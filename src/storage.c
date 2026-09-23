#include "i18n.h"
#include <string.h>
#include "storage.h"

#ifdef __CC65__
#pragma warn (const-comparison, push, off)
#endif
typedef char named_size_check[1 / (sizeof(KbNamed) == 27)];
typedef char card_size_check[1 / (sizeof(KbCard) == 93)];
typedef char item_size_check[1 / (sizeof(KbItem) == 28)];
typedef char data_size_check[1 / (sizeof(KbData) == KB_PAYLOAD_SIZE)];
#ifdef __CC65__
#pragma warn (const-comparison, pop)
#endif

/* Staging makes invalid/truncated loads non-destructive, even with 64K RAM. */
static KbData staging;
static Byte header[KB_HEADER_SIZE];
static char filename[17];

unsigned int kb_crc(const Byte *bytes, unsigned int length, unsigned int crc)
{
    Byte bit;
    while (length--) {
        crc ^= (unsigned int)*bytes++ << 8;
        for (bit = 0; bit < 8; ++bit)
            crc = ((crc & 0x8000U) ? (crc << 1) ^ 0x1021U : crc << 1) & 0xffffU;
    }
    return crc;
}

static unsigned int word_at(Byte offset)
{
    return header[offset] | ((unsigned int)header[offset + 1] << 8);
}

static void put_word(Byte offset, unsigned int value)
{
    header[offset] = (Byte)value;
    header[offset + 1] = (Byte)(value >> 8);
}

static int valid_name(const char *base)
{
    Byte i;
    for (i = 0; i <= 12; ++i) {
        if (!base[i]) return i > 0;
        if (!((base[i] >= 'A' && base[i] <= 'Z') ||
              (base[i] >= '0' && base[i] <= '9') || base[i] == '-')) return 0;
    }
    return 0;
}

static void name_slot(const char *base, Byte slot)
{
    strcpy(filename, base);
    strcat(filename, slot ? "-B" : "-A");
}

static int transfer(Byte writing, Byte *bytes, unsigned int size)
{
    unsigned int count;
    int result;
    while (size) {
        count = size > 128 ? 128 : size;
        result = writing ? disk_write(bytes, count) : disk_read(bytes, count);
        if (result != (int)count)
            return !writing && result >= 0 ? KB_DISK_INVALID : KB_DISK_IO;
        bytes += count;
        size -= count;
    }
    return 0;
}

static int read_slot(const char *base, Byte slot, unsigned int *generation, Byte *language)
{
    int result, closed;
    unsigned int crc;
    Byte extra;
    name_slot(base, slot);
    result = disk_open(filename, 0);
    if (result) return result;
    result = transfer(0, header, KB_HEADER_SIZE);
    if (!result && (memcmp(header, "C64K", 4) ||
        !((header[4] == 1 && header[5] == 0) ||
          (header[4] == KB_FORMAT_VERSION && header[5] < LANG_COUNT)) ||
        word_at(8) != KB_PAYLOAD_SIZE)) result = KB_DISK_INVALID;
    if (!result) result = transfer(0, (Byte *)&staging, KB_PAYLOAD_SIZE);
    if (!result && disk_read(&extra, 1) != 0) result = KB_DISK_INVALID;
    closed = disk_close();
    if (!result) result = closed;
    if (result) return result;
    crc = kb_crc(header, 10, 0xffffU);
    crc = kb_crc((const Byte *)&staging, KB_PAYLOAD_SIZE, crc);
    if (crc != word_at(10) || !kb_validate(&staging)) return KB_DISK_INVALID;
    *generation = word_at(6);
    *language = header[5];
    return 0;
}

static int newer(unsigned int a, unsigned int b)
{
    unsigned int difference;
    difference = (a - b) & 0xffffU;
    return difference != 0 && difference < 0x8000U;
}

/* A missing/corrupt slot can be recovered from the other slot. An I/O error
 * when saving is different: do not guess which disk copy is safe to replace. */
int kb_load(KbData *db, const char *base)
{
    int a, b;
    unsigned int ga, gb;
    Byte language;
    if (!valid_name(base)) return KB_DISK_NAME;
    ga = gb = 0;
    a = read_slot(base, 0, &ga, &language);
    if (!a) {
        memcpy(db, &staging, sizeof(KbData));
        ui_language = language;
    }
    b = read_slot(base, 1, &gb, &language);
    if (!b && (a || newer(gb, ga))) {
        memcpy(db, &staging, sizeof(KbData));
        ui_language = language;
    }
    if (!a || !b) return 0;
    if (a == KB_DISK_MISSING && b == KB_DISK_MISSING) return KB_DISK_MISSING;
    return a != KB_DISK_MISSING ? a : b;
}

int kb_save(KbData *db, const char *base)
{
    int a, b, result, closed;
    Byte slot, language;
    unsigned int ga, gb, generation, verified, crc;
    if (!valid_name(base)) return KB_DISK_NAME;
    if (ui_language >= LANG_COUNT || !kb_validate(db)) return KB_DISK_INVALID;
    ga = gb = 0;
    a = read_slot(base, 0, &ga, &language);
    b = read_slot(base, 1, &gb, &language);
    if (a == KB_DISK_IO || b == KB_DISK_IO) return KB_DISK_IO;
    if (!a && (b || !newer(gb, ga))) { slot = 1; generation = ga + 1; }
    else if (!b) { slot = 0; generation = gb + 1; }
    else { slot = 0; generation = 1; }
    generation &= 0xffffU;
    name_slot(base, slot);
    result = disk_remove(filename);
    if (result && result != KB_DISK_MISSING) return result;
    result = disk_open(filename, 1);
    if (result) return result;
    memcpy(header, "C64K", 4);
    header[4] = KB_FORMAT_VERSION;
    header[5] = ui_language;
    put_word(6, generation);
    put_word(8, KB_PAYLOAD_SIZE);
    crc = kb_crc(header, 10, 0xffffU);
    crc = kb_crc((const Byte *)db, KB_PAYLOAD_SIZE, crc);
    put_word(10, crc);
    result = transfer(1, header, KB_HEADER_SIZE);
    if (!result) result = transfer(1, (Byte *)db, KB_PAYLOAD_SIZE);
    closed = disk_close();
    if (!result) result = closed;
    if (result) return result;
    result = read_slot(base, slot, &verified, &language);
    if (result || verified != generation || language != ui_language ||
        memcmp(db, &staging, sizeof(KbData)))
        return KB_DISK_VERIFY;
    return 0;
}

const char *kb_disk_message(int result)
{
    switch (result) {
        case KB_DISK_OK: return tr(T_DISK_OK);
        case KB_DISK_MISSING: return tr(T_DISK_MISSING);
        case KB_DISK_INVALID: return tr(T_DISK_INVALID);
        case KB_DISK_NAME: return tr(T_DISK_NAME);
        case KB_DISK_VERIFY: return tr(T_DISK_VERIFY);
    }
    return tr(T_DISK_IO);
}
