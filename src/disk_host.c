#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "storage.h"

Byte disk_device = 8;
char disk_detail[40];
static FILE *stream;

int disk_open(const char *name, Byte writing)
{
    disk_detail[0] = 0;
    stream = fopen(name, writing ? "wb" : "rb");
    if (stream) return 0;
    strncpy(disk_detail, strerror(errno), 39);
    disk_detail[39] = 0;
    return errno == ENOENT && !writing ? KB_DISK_MISSING : KB_DISK_IO;
}
int disk_read(void *buffer, unsigned int size)
{
    size_t count;
    count = fread(buffer, 1, size, stream);
    return ferror(stream) ? -1 : (int)count;
}
int disk_write(const void *buffer, unsigned int size)
{
    return (int)fwrite(buffer, 1, size, stream);
}
int disk_close(void)
{
    int result;
    result = fclose(stream);
    stream = NULL;
    return result ? KB_DISK_IO : 0;
}
int disk_remove(const char *name)
{
    if (!remove(name)) return 0;
    return errno == ENOENT ? KB_DISK_MISSING : KB_DISK_IO;
}
