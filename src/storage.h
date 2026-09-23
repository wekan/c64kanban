#ifndef KB_STORAGE_H
#define KB_STORAGE_H
#include "kanban.h"

#define KB_DISK_OK 0
#define KB_DISK_MISSING 1
#define KB_DISK_IO 2
#define KB_DISK_INVALID 3
#define KB_DISK_NAME 4
#define KB_DISK_VERIFY 5
#define KB_PAYLOAD_SIZE 7364
#define KB_HEADER_SIZE 12
#define KB_FORMAT_VERSION 2

/* Backend operations have a single open stream. close must report disk errors,
 * including errors reported only after a C64 sequential file is closed. */
int disk_open(const char *name, Byte writing);
int disk_read(void *buffer, unsigned int size);
int disk_write(const void *buffer, unsigned int size);
int disk_close(void);
int disk_remove(const char *name);
extern Byte disk_device;
extern char disk_detail[40];

unsigned int kb_crc(const Byte *bytes, unsigned int length, unsigned int crc);
int kb_load(KbData *db, const char *base);
int kb_save(KbData *db, const char *base);
const char *kb_disk_message(int result);
#endif
