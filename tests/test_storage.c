#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "i18n.h"
#define SIZE (KB_HEADER_SIZE + KB_PAYLOAD_SIZE)
static Byte files[2][SIZE + 1], backup[SIZE + 1], present[2];
static unsigned int lengths[2], position;
static int slot, mode, fail_after = -1, fail_close, fail_open, fail_remove, corrupt_write;
static KbData db, loaded, old;
Byte disk_device = 8;
char disk_detail[40];
int disk_open(const char *name, Byte writing)
{
    if (fail_open) return KB_DISK_IO;
    slot = name[strlen(name) - 1] == 'B';
    position = 0; mode = writing;
    if (!writing && !present[slot]) return KB_DISK_MISSING;
    if (writing) { lengths[slot] = 0; present[slot] = 1; }
    return 0;
}
int disk_read(void *buffer, unsigned int size)
{
    if (size > lengths[slot] - position) size = lengths[slot] - position;
    memcpy(buffer, files[slot] + position, size);
    position += size;
    return (int)size;
}
int disk_write(const void *buffer, unsigned int size)
{
    if (fail_after >= 0) {
        if (position >= (unsigned int)fail_after) return -1;
        if (size > (unsigned int)fail_after - position) size = fail_after - position;
    }
    assert(position + size <= SIZE);
    memcpy(files[slot] + position, buffer, size);
    position += size; lengths[slot] = position;
    return (int)size;
}
int disk_close(void)
{
    if (mode && corrupt_write && lengths[slot]) files[slot][lengths[slot] - 1] ^= 1;
    return mode && fail_close ? KB_DISK_IO : 0;
}
int disk_remove(const char *name)
{
    int s;
    if (fail_remove) return KB_DISK_IO;
    s = name[strlen(name) - 1] == 'B';
    present[s] = 0; lengths[s] = 0;
    return 0;
}
static void reset(void)
{
    memset(files, 0, sizeof files);
    memset(present, 0, sizeof present);
    memset(lengths, 0, sizeof lengths);
    fail_after = -1;
    fail_open = fail_close = fail_remove = corrupt_write = 0;
    ui_language = LANG_EN;
    kb_init(&db);
    kb_add_card(&db, 0, 0, 0, "PERSISTED CARD");
    kb_add_named(&db, KB_CHECKLIST, 0, "CHECKLIST");
    kb_add_item(&db, 0, "FIRST ITEM");
    db.items[0].done = 1;
    kb_set_text(db.cards[0].description, "C64 DISK ROUND TRIP", KB_DESCRIPTION, 1);
    db.cards[0].priority = 2;
}
static void crc_file(int s)
{
    unsigned int crc;
    crc = kb_crc(files[s], 10, 0xffffU);
    crc = kb_crc(files[s] + KB_HEADER_SIZE, KB_PAYLOAD_SIZE, crc);
    files[s][10] = (Byte)crc; files[s][11] = (Byte)(crc >> 8);
}
static void normal(void)
{
    reset(); old = db;
    assert(kb_crc((const Byte *)"123456789", 9, 0xffffU) == 0x29b1);
    loaded = db;
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_MISSING);
    assert(!memcmp(&loaded, &db, sizeof db));
    assert(kb_save(&db, "BAD,NAME") == KB_DISK_NAME);
    assert(kb_save(&db, "") == KB_DISK_NAME);
    assert(kb_save(&db, "1234567890123") == KB_DISK_NAME);
    assert(!kb_save(&db, "KANBAN"));
    assert(lengths[0] == SIZE && !present[1]);
    assert(!memcmp(files[0], "C64K\2\0\1\0", 8));
    memset(&loaded, 0, sizeof loaded);
    assert(!kb_load(&loaded, "KANBAN"));
    assert(!memcmp(&loaded, &db, sizeof db));
    kb_rename(&db, KB_BOARD, 0, "SECOND GENERATION");
    assert(!kb_save(&db, "KANBAN"));
    assert(present[1] && files[1][6] == 2);
    assert(!kb_load(&loaded, "KANBAN") && !memcmp(&loaded, &db, sizeof db));
    /* Corrupt newest payload, then recover the previous verified snapshot. */
    files[1][SIZE - 1] ^= 1;
    assert(!kb_load(&loaded, "KANBAN") && !memcmp(&loaded, &old, sizeof old));
    assert(!kb_save(&db, "KANBAN"));
    assert(!kb_load(&loaded, "KANBAN") && !memcmp(&loaded, &db, sizeof db));
    /* Wrapped sequence 0 is newer than 65535. Header is checksummed too. */
    files[0][6] = files[0][7] = 255; crc_file(0);
    files[1][6] = files[1][7] = 0; crc_file(1);
    assert(!kb_load(&loaded, "KANBAN") && !memcmp(&loaded, &db, sizeof db));
    assert(!kb_save(&db, "KANBAN") && files[0][6] == 1);
}
static void malformed(void)
{
    int n;
    reset(); assert(!kb_save(&db, "KANBAN"));
    memcpy(backup, files[0], SIZE);
    loaded = old = db;
    for (n = 0; n < SIZE; n += 97) {
        lengths[0] = (unsigned int)n;
        assert(kb_load(&loaded, "KANBAN") != 0);
        assert(!memcmp(&loaded, &old, sizeof old));
    }
    lengths[0] = SIZE + 1;
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_INVALID);
    lengths[0] = SIZE;
    files[0][4] = 3; crc_file(0);
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_INVALID);
    memcpy(files[0], backup, SIZE);
    /* Valid checksum, invalid parent reference. */
    files[0][KB_HEADER_SIZE + KB_BOARDS * 27 + 1] = 255;
    crc_file(0);
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_INVALID);
    assert(!memcmp(&loaded, &old, sizeof old));
    memcpy(files[0], backup, SIZE);
    /* CRC detects corruption of sequence as well as user data. */
    files[0][6] ^= 1;
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_INVALID);
}
static void interrupted(void)
{
    int n;
    for (n = 0; n < SIZE; n += 127) {
        reset(); old = db;
        assert(!kb_save(&db, "KANBAN"));
        memcpy(backup, files[0], SIZE);
        kb_rename(&db, KB_BOARD, 0, "NEW DATA");
        fail_after = n;
        assert(kb_save(&db, "KANBAN") == KB_DISK_IO);
        assert(!memcmp(backup, files[0], SIZE));
        fail_after = -1;
        assert(!kb_load(&loaded, "KANBAN"));
        assert(!memcmp(&loaded, &old, sizeof old));
        assert(!kb_save(&db, "KANBAN"));
        assert(!kb_load(&loaded, "KANBAN") && !memcmp(&loaded, &db, sizeof db));
    }
    reset(); assert(!kb_save(&db, "KANBAN"));
    memcpy(backup, files[0], SIZE);
    fail_open = 1; assert(kb_save(&db, "KANBAN") == KB_DISK_IO); fail_open = 0;
    fail_remove = 1; assert(kb_save(&db, "KANBAN") == KB_DISK_IO); fail_remove = 0;
    fail_close = 1; assert(kb_save(&db, "KANBAN") == KB_DISK_IO); fail_close = 0;
    assert(!memcmp(backup, files[0], SIZE));
    present[1] = 0;
    corrupt_write = 1;
    assert(kb_save(&db, "KANBAN") == KB_DISK_VERIFY);
    assert(!memcmp(backup, files[0], SIZE));
}
static void languages(void)
{
    reset();
    assert(!kb_save(&db, "KANBAN"));
    /* Read legacy version-1 English files without changing any user text. */
    files[0][4] = 1; crc_file(0);
    ui_language = LANG_FI;
    assert(!kb_load(&loaded, "KANBAN") && ui_language == LANG_EN);
    assert(!memcmp(&loaded, &db, sizeof db));
    /* Saving must not import the scanned slot's language into the UI. */
    ui_language = LANG_FI;
    assert(!kb_save(&db, "KANBAN") && ui_language == LANG_FI);
    assert(files[1][4] == KB_FORMAT_VERSION && files[1][5] == LANG_FI);
    ui_language = LANG_EN;
    assert(!kb_load(&loaded, "KANBAN") && ui_language == LANG_FI);
    assert(!memcmp(&loaded, &db, sizeof db));
    /* CRC protects language; fallback restores language and data together. */
    files[1][5] = LANG_EN;
    assert(!kb_load(&loaded, "KANBAN") && ui_language == LANG_EN);
    ui_language = LANG_FI;
    assert(!kb_save(&db, "KANBAN"));
    files[0][5] = 1; crc_file(0); /* Not valid in legacy v1. */
    files[1][5] = LANG_COUNT; crc_file(1); /* Unsupported language. */
    old = loaded;
    ui_language = LANG_EN;
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_INVALID);
    assert(ui_language == LANG_EN && !memcmp(&loaded, &old, sizeof old));
    ui_language = LANG_FI;
    fail_open = 1;
    assert(kb_save(&db, "KANBAN") == KB_DISK_IO && ui_language == LANG_FI);
    assert(kb_load(&loaded, "KANBAN") == KB_DISK_IO && ui_language == LANG_FI);
    fail_open = 0;
    ui_language = LANG_COUNT;
    assert(kb_save(&db, "KANBAN") == KB_DISK_INVALID);
    ui_language = LANG_EN;
}
int main(void)
{
    normal(); malformed(); interrupted(); languages();
    puts("storage: round trips, CRC, recovery, truncation, write faults, rollover, language + v1 migration passed");
    return 0;
}
