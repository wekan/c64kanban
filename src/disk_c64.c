#include "i18n.h"
#include <cbm.h>
#include <string.h>
#include "storage.h"

Byte disk_device = 8;
char disk_detail[40];
static char command[24];
static Byte opened;
static Byte eof;

static int status(void)
{
    int count;
    count = cbm_read(15, disk_detail, 39);
    if (count < 2) {
        strcpy(disk_detail, tr(T_DRIVE_READY));
        return KB_DISK_IO;
    }
    disk_detail[count] = 0;
    if (disk_detail[0] == '0' &&
        (disk_detail[1] == '0' || disk_detail[1] == '1')) return 0;
    if (disk_detail[0] == '6' && disk_detail[1] == '2') return KB_DISK_MISSING;
    return KB_DISK_IO;
}

static int command_open(const char *cmd)
{
    if (cbm_open(15, disk_device, 15, cmd)) {
        cbm_close(15);
        strcpy(disk_detail, tr(T_DRIVE_MISSING));
        return KB_DISK_IO;
    }
    return 0;
}

int disk_open(const char *name, Byte writing)
{
    int result;
    opened = 0;
    eof = 0;
    disk_detail[0] = 0;
    if (command_open("")) return KB_DISK_IO;
    /* Consume power-on status before the actual OPEN operation. */
    status();
    strcpy(command, "0:");
    strcat(command, name);
    strcat(command, writing ? ",S,W" : ",S,R");
    result = cbm_open(2, disk_device, 2, command);
    if (result) result = KB_DISK_IO;
    else result = status();
    if (result) { cbm_close(2); cbm_close(15); }
    else opened = 1;
    return result;
}

int disk_read(void *buffer, unsigned int size)
{
    int result;
    Byte st;
    if (eof) return 0;
    result = cbm_read(2, buffer, size);
    st = cbm_k_readst();
    if (st & 0xbf) return -1;
    eof = (st & 0x40) != 0;
    return result;
}

int disk_write(const void *buffer, unsigned int size)
{
    int result;
    result = cbm_write(2, buffer, size);
    /* cc65's last-byte write is not always followed by READST. */
    if (cbm_k_readst()) result = -1;
    cbm_k_clrch();
    return result;
}

int disk_close(void)
{
    int result;
    result = 0;
    if (opened) {
        cbm_close(2);
        result = status();
        cbm_close(15);
        opened = 0;
    }
    return result;
}

int disk_remove(const char *name)
{
    int result;
    strcpy(command, "S0:");
    strcat(command, name);
    result = command_open(command);
    if (result) return result;
    result = status();
    cbm_close(15);
    return result;
}
