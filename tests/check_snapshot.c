#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "i18n.h"
static KbData data;
int main(void)
{
    Byte done, total;
    assert(!kb_load(&data, "KANBAN"));
    assert(ui_language == LANG_EN);
    assert(!strcmp(data.cards[0].title, "SHIP C64"));
    assert(!strcmp(data.cards[0].description, "PERSIST AFTER REBOOT"));
    assert(data.cards[0].priority == 2);
    assert(!strcmp(data.cards[1].title, "SECOND CARD"));
    kb_progress(&data, 0, &done, &total);
    assert(done == 1 && total == 1);
    assert(!strcmp(data.items[0].title, "TEST DISK") && data.items[0].done);
    puts("C64-produced snapshots load unchanged through the native host backend");
    return 0;
}
