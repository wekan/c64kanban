#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main kanban_program_main
#include "../kanban.c"
#undef main

static char display[25][40];
static const unsigned char *script;
static unsigned int steps;
void screen_init(void) { screen_clear(); }
void screen_done(void) {}
void screen_clear(void) { memset(display, ' ', sizeof display); }
void screen_text(Byte x, Byte y, const char *text, Byte width, Byte color, Byte reverse)
{
    Byte i;
    (void)color; (void)reverse;
    assert(y < 25 && x + width <= 39);
    for (i = 0; i < width; ++i) {
        display[y][x + i] = *text ? *text++ : ' ';
        assert(display[y][x + i] >= 32 && display[y][x + i] <= 95);
    }
}
int screen_key(void)
{
    int c;
    assert(script && *script && ++steps < 2000);
    c = *script++;
    if (c == 21) return KEY_CLEAR;
    if (c == 17) return KEY_DOWN;
    return c;
}
static void keys(const char *s) { script = (const unsigned char *)s; steps = 0; }
static void consumed(void) { assert(!*script); }
static void workflow(void)
{
    int b, u, t, id;
    Byte done, total;
    board = lane = list_position = card_position = dirty = 0;
    kb_init(&data);
    keys("NPROJECT\r\r"); b = choose(KB_BOARD, 0, 1); consumed();
    assert(b == 1 && !strcmp(data.boards[b].title, "PROJECT"));
    keys("NBLUE TEAM\r\r"); u = choose(KB_LANE, (Byte)b, 1); consumed();
    keys("NREVIEW\r\r"); t = choose(KB_LIST, (Byte)b, 1); consumed();
    id = kb_add_card(&data, 0, 0, 0, "OLD");
    keys("E\025NEW CARD\rDALL DETAILS\rPP KNPRELAUNCH\r\rNBUILD\r NTEST\r\033\033\033");
    card_detail((Byte)id); consumed();
    assert(!strcmp(data.cards[id].title, "NEW CARD"));
    assert(!strcmp(data.cards[id].description, "ALL DETAILS"));
    assert(data.cards[id].priority == 2 && data.cards[id].done == 1);
    kb_progress(&data, (Byte)id, &done, &total);
    assert(done == 1 && total == 2);
    /* Move through the real board/lane/list destination selectors. */
    keys("M\021\r\021\r\021\021\021\r\033");
    card_detail((Byte)id); consumed();
    assert(data.cards[id].board == b && data.cards[id].lane == u && data.cards[id].list == t);
    assert(kb_validate(&data));
    keys("E\025CANCELLED\033\033"); card_detail((Byte)id); consumed();
    assert(!strcmp(data.cards[id].title, "NEW CARD"));
    keys("XX\033"); card_detail((Byte)id); consumed();
    assert(data.cards[id].used);
    keys("XY"); card_detail((Byte)id); consumed();
    assert(!data.cards[id].used && !data.items[0].used && !data.checklists[0].used);
    /* Long input never overwrites storage, and clear/backspace work. */
    keys("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789\r");
    assert(input("TITLE", "", KB_TITLE, 0)); consumed();
    assert(strlen(edit) == KB_TITLE);
    keys("ABC\bD\r"); assert(input("TITLE", "", KB_TITLE, 0)); consumed();
    assert(!strcmp(edit, "ABD"));
    /* Scrolling and deleting the current board leave valid selection. */
    board = (Byte)b; lane = (Byte)u; list_position = 3;
    for (id = 0; id < 8; ++id) assert(kb_add_card(&data, board, lane, (Byte)t, "SCROLL CARD") >= 0);
    card_position = 7; selection_valid(); draw_board();
    sprintf(line, tr(T_SELECTION), 8, 4);
    assert(!memcmp(display[20], line, strlen(line)));
    assert(!kb_delete_named(&data, KB_BOARD, board));
    selection_valid(); draw_board();
    assert(board == 0 && lane == 0 && kb_validate(&data));

}

int main(void)
{
    static KbData before;
    ui_language = LANG_EN; workflow();
    ui_language = LANG_FI; workflow();
    ui_language = LANG_EN;
    before = data;
    dirty = 0;
    keys("K\033"); settings(); consumed();
    assert(ui_language == LANG_FI && dirty);
    assert(!memcmp(display[0], "ASETUKSET", 9));
    assert(!memcmp(&before, &data, sizeof data));
    assert(!strcmp(language_name(), "SUOMI"));
    keys("Y"); assert(confirm(tr(T_DELETE_CARD))); consumed();
    keys("N"); assert(!confirm(tr(T_DELETE_CARD))); consumed();
    keys("K\033"); settings(); consumed();
    assert(ui_language == LANG_EN && !strcmp(language_name(), "ENGLISH"));
    assert(!memcmp(display[0], "SETTINGS", 8));
    assert(!memcmp(&before, &data, sizeof data));
    puts("UI: full workflow in EN/FI, language switching, unchanged user text passed");
    return 0;
}
