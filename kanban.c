#include "src/i18n.h"
#include <stdio.h>
#include <string.h>
#include "src/kanban.h"
#include "src/platform.h"
#include "src/storage.h"

static KbData data;
static Byte board, lane, list_position, card_position, dirty;
static char base[13] = "KANBAN";
static char line[80], edit[KB_DESCRIPTION + 1], message[40];

static void row(Byte y, const char *text)
{
    screen_text(0, y, text, 39, UI_WHITE, 0);
}
static void title(const char *text)
{
    screen_clear();
    screen_text(0, 0, text, 39, UI_CYAN, 1);
}
static void notify(const char *text)
{
    strncpy(message, text, 39);
    message[39] = 0;
}
static void changed(int result)
{
    if (result >= 0) { dirty = 1; notify(tr(T_CHANGED)); }
    else if (result == KB_FULL) notify(tr(T_FULL));
    else if (result == KB_LAST) notify(tr(T_LAST));
    else notify(tr(T_INVALID));
}
static int confirm(const char *prompt)
{
    row(21, prompt);
    row(22, tr(T_CONFIRM));
    return screen_key() == 'Y';
}
static int input(const char *prompt, const char *old, Byte limit, Byte empty)
{
    Byte length, offset;
    int key;
    strcpy(edit, old);
    length = (Byte)strlen(edit);
    for (;;) {
        row(18, prompt);
        offset = length > 36 ? length - 36 : 0;
        screen_text(0, 19, edit + offset, 38, UI_YELLOW, 0);
        screen_text(length - offset, 19, "_", 1, UI_YELLOW, 1);
        row(20, tr(T_INPUT_ACCEPT));
        row(21, tr(T_INPUT_ERASE));
        key = screen_key();
        if (key == 27) return 0;
        if (key == 13) {
            if (!kb_set_text(edit, edit, limit, empty)) return 1;
            row(22, tr(T_NONBLANK));
        } else if (key == 8 && length) edit[--length] = 0;
        else if (key == KEY_CLEAR) { length = 0; edit[0] = 0; }
        else if (key >= 32 && key <= 95 && length < limit) {
            edit[length++] = (char)key;
            edit[length] = 0;
        }
    }
}
static const char *kind_title(Byte kind)
{
    switch (kind) {
        case KB_BOARD: return tr(T_BOARDS);
        case KB_LANE: return tr(T_LANES);
        case KB_LIST: return tr(T_LISTS);
    }
    return tr(T_CHECKLISTS);
}

/* All collections scroll. IDs remain stable when another entry is deleted. */
static int choose(Byte kind, Byte parent, Byte manage)
{
    Byte position, start, r, cap;
    int id, key, selected, created;
    KbNamed *pool;
    position = 0;
    for (;;) {
        pool = kb_pool(&data, kind, &cap);
        while (position && kb_named_at(&data, kind, parent, position) < 0) --position;
        selected = kb_named_at(&data, kind, parent, position);
        title(kind_title(kind));
        start = (position / 12) * 12;
        for (r = 0; r < 12; ++r) {
            id = kb_named_at(&data, kind, parent, start + r);
            if (id < 0) break;
            sprintf(line, "%2u %s", start + r + 1, pool[id].title);
            screen_text(0, 2 + r, line, 39, UI_WHITE, start + r == position);
        }
        if (selected < 0) row(3, tr(T_EMPTY));
        row(15, tr(T_SELECT_OPEN));
        row(16, manage ? tr(T_MANAGE) : tr(T_DESTINATION));
        row(17, tr(T_BACK));
        row(24, message);
        key = screen_key();
        if (key == 27) return -1;
        if (key == KEY_UP && position) --position;
        if (key == KEY_DOWN && kb_named_at(&data, kind, parent, position + 1) >= 0)
            ++position;
        if (key == 13 && selected >= 0) return selected;
        if (manage && key == 'N' && input(tr(T_NEW_TITLE), "", KB_TITLE, 0)) {
            created = kind == KB_BOARD ? kb_add_board(&data, edit) :
                kb_add_named(&data, kind, parent, edit);
            changed(created);
            if (created >= 0) {
                position = 0;
                while (kb_named_at(&data, kind, parent, position) != created) ++position;
            }
        }
        if (manage && selected >= 0 && key == 'E' &&
            input(tr(T_RENAME), pool[selected].title, KB_TITLE, 0))
            changed(kb_rename(&data, kind, (Byte)selected, edit));
        if (manage && selected >= 0 && key == 'X' && confirm(tr(T_DELETE_ENTRY)))
            changed(kb_delete_named(&data, kind, (Byte)selected));
    }
}
static void items(Byte checklist)
{
    Byte position, start, r;
    int id, selected, key, created;
    position = 0;
    for (;;) {
        while (position && kb_item_at(&data, checklist, position) < 0) --position;
        selected = kb_item_at(&data, checklist, position);
        title(data.checklists[checklist].title);
        start = (position / 12) * 12;
        for (r = 0; r < 12; ++r) {
            id = kb_item_at(&data, checklist, start + r);
            if (id < 0) break;
            sprintf(line, "[%c] %s", data.items[id].done ? 'X' : ' ', data.items[id].title);
            screen_text(0, 2 + r, line, 39, data.items[id].done ? UI_GREEN : UI_WHITE,
                start + r == position);
        }
        if (selected < 0) row(3, tr(T_EMPTY_ITEMS));
        row(15, tr(T_SELECT_TOGGLE));
        row(16, tr(T_MANAGE_ITEMS));
        row(17, tr(T_BACK));
        row(24, message);
        key = screen_key();
        if (key == 27) return;
        if (key == KEY_UP && position) --position;
        if (key == KEY_DOWN && kb_item_at(&data, checklist, position + 1) >= 0) ++position;
        if (key == 'N' && input(tr(T_NEW_ITEM), "", KB_TITLE, 0)) {
            created = kb_add_item(&data, checklist, edit);
            changed(created);
            if (created >= 0) {
                position = 0;
                while (kb_item_at(&data, checklist, position) != created) ++position;
            }
        }
        if (selected >= 0) {
            if (key == ' ' || key == 13) { data.items[selected].done ^= 1; changed(0); }
            if (key == 'E' && input(tr(T_EDIT_ITEM), data.items[selected].title, KB_TITLE, 0))
                changed(kb_set_text(data.items[selected].title, edit, KB_TITLE, 0));
            if (key == 'X' && confirm(tr(T_DELETE_ITEM)))
                changed(kb_delete_item(&data, (Byte)selected));
        }
    }
}
static void move_card(Byte id)
{
    int b, u, t;
    b = choose(KB_BOARD, 0, 0);
    if (b < 0) return;
    u = choose(KB_LANE, (Byte)b, 0);
    if (u < 0) return;
    t = choose(KB_LIST, (Byte)b, 0);
    if (t >= 0) changed(kb_move_card(&data, id, (Byte)b, (Byte)u, (Byte)t));
}
static void card_detail(Byte id)
{
    KbCard *card;
    Byte done, total;
    int key, checklist;
    card = &data.cards[id];
    for (;;) {
        title(card->title);
        sprintf(line, "%s / %s", data.boards[card->board].title,
            data.lanes[card->lane].title);
        row(2, line);
        row(3, data.lists[card->list].title);
        row(5, tr(T_DESCRIPTION));
        screen_text(0, 6, card->description, 36, UI_WHITE, 0);
        if (strlen(card->description) > 36) row(7, card->description + 36);
        sprintf(line, tr(T_CARD_STATUS),
            card->priority == 2 ? tr(T_HIGH) : card->priority == 1 ? tr(T_MID) : tr(T_LOW),
            card->done ? tr(T_DONE) : tr(T_OPEN));
        row(9, line);
        kb_progress(&data, id, &done, &total);
        sprintf(line, tr(T_PROGRESS), done, total);
        row(11, line);
        row(13, tr(T_CARD_EDIT));
        row(14, tr(T_CARD_ACTIONS));
        row(15, tr(T_CARD_DELETE_BACK));
        row(24, message);
        key = screen_key();
        if (key == 27) return;
        if (key == 'E' && input(tr(T_CARD_TITLE), card->title, KB_TITLE, 0))
            changed(kb_set_text(card->title, edit, KB_TITLE, 0));
        if (key == 'D' && input(tr(T_EDIT_DESCRIPTION), card->description, KB_DESCRIPTION, 1))
            changed(kb_set_text(card->description, edit, KB_DESCRIPTION, 1));
        if (key == 'P') { card->priority = (card->priority + 1) % 3; changed(0); }
        if (key == ' ') { card->done ^= 1; changed(0); }
        if (key == 'M') move_card(id);
        if (key == 'K') {
            for (;;) {
                checklist = choose(KB_CHECKLIST, id, 1);
                if (checklist < 0) break;
                items((Byte)checklist);
            }
        }
        if (key == 'X' && confirm(tr(T_DELETE_CARD))) {
            changed(kb_delete_card(&data, id));
            return;
        }
    }
}
static void selection_valid(void)
{
    int list;
    if (!data.boards[board].used) board = (Byte)kb_named_at(&data, KB_BOARD, 0, 0);
    if (!data.lanes[lane].used || data.lanes[lane].parent != board)
        lane = (Byte)kb_named_at(&data, KB_LANE, board, 0);
    while (list_position && kb_named_at(&data, KB_LIST, board, list_position) < 0)
        --list_position;
    list = kb_named_at(&data, KB_LIST, board, list_position);
    while (card_position && kb_card_at(&data, lane, (Byte)list, card_position) < 0)
        --card_position;
}
static void draw_board(void)
{
    Byte col, r, start, page, done, total, active, color;
    int list, id;
    sprintf(line, "%c %s", dirty ? '*' : ' ', data.boards[board].title);
    title(line);
    sprintf(line, tr(T_LANE_LABEL), data.lanes[lane].title);
    screen_text(0, 1, line, 39, UI_YELLOW, 0);
    start = (list_position / 3) * 3;
    page = (card_position / 5) * 5;
    for (col = 0; col < 3; ++col) {
        list = kb_named_at(&data, KB_LIST, board, start + col);
        if (list < 0) break;
        active = start + col == list_position;
        screen_text(col * 13, 3, data.lists[list].title, 12, UI_CYAN, active);
        for (r = 0; r < 5; ++r) {
            id = kb_card_at(&data, lane, (Byte)list, page + r);
            if (id < 0) break;
            color = data.cards[id].done ? UI_GREEN :
                data.cards[id].priority == 2 ? UI_RED : UI_YELLOW;
            sprintf(line, "%u %s", r + 1, data.cards[id].title);
            screen_text(col * 13, 5 + r * 3, line, 12, color,
                active && page + r == card_position);
            kb_progress(&data, (Byte)id, &done, &total);
            sprintf(line, "%c %u/%u%s", data.cards[id].done ? 'X' : '-', done, total,
                data.cards[id].priority == 2 ? " !" : "");
            screen_text(col * 13, 6 + r * 3, line, 12, color, 0);
        }
    }
    sprintf(line, tr(T_SELECTION),
        card_position + 1, list_position + 1);
    row(20, line);
    row(21, tr(T_BOARD_CARDS));
    row(22, tr(T_BOARD_MANAGERS));
    row(23, tr(T_BOARD_FILES));
    row(24, message);
}
static void help(void)
{
    title(tr(T_HELP_TITLE));
    row(2, tr(T_HELP_LISTS));
    row(3, tr(T_HELP_CARDS));
    row(4, tr(T_HELP_NUMBERS));
    row(5, tr(T_HELP_ORDER));
    row(7, tr(T_HELP_MANAGERS));
    row(8, tr(T_HELP_EDIT));
    row(9, tr(T_HELP_RETURN));
    row(11, tr(T_HELP_CARD_ACTIONS));
    row(12, tr(T_HELP_SPACE));
    row(14, tr(T_HELP_SAVE));
    row(15, tr(T_HELP_LOAD));
    row(16, tr(T_HELP_SETTINGS));
    row(18, tr(T_LIMIT_BOARDS));
    row(19, tr(T_LIMIT_CARDS));
    row(20, tr(T_LIMIT_SHARED));
    row(22, tr(T_HELP_DISK));
    row(24, tr(T_ANY_BACK));
    screen_key();
}
static void settings(void)
{
    int key;
    for (;;) {
        title(tr(T_SETTINGS_TITLE));
        sprintf(line, tr(T_LANGUAGE_LABEL), language_name());
        row(2, line);
        sprintf(line, tr(T_CURRENT_FILE), base, disk_device);
        row(4, line);
        row(6, tr(T_NAME_RULE));
        row(7, tr(T_TWO_FILES));
        row(9, tr(T_DRIVE_KEYS));
        row(10, tr(T_LANGUAGE_KEY));
        row(11, tr(T_NAME_BACK));
        row(14, tr(T_LANGUAGE_SAVED));
        row(24, message);
        key = screen_key();
        if (key == 27) break;
        if (key == '8' || key == '9') disk_device = (Byte)(key - '0');
        if (key == '0' || key == '1') disk_device = (Byte)(key - '0' + 10);
        if (key == 'N' && input(tr(T_DATA_NAME), base, 12, 0)) strcpy(base, edit);
        if (key == 'K') {
            ui_language = ui_language == LANG_EN ? LANG_FI : LANG_EN;
            changed(0);
        }
    }
    sprintf(line, tr(T_FILE_NOTICE), base, disk_device);
    notify(line);
}
static int save(void)
{
    int result;
    row(24, tr(T_SAVING));
    result = kb_save(&data, base);
    if (!result) { dirty = 0; notify(tr(T_SAVED)); }
    else notify(kb_disk_message(result));
    return result;
}
int main(void)
{
    int key, list, selected, result;
    Byte position;
    screen_init();
    kb_init(&data);
    row(0, "C64 KANBAN");
    row(2, tr(T_STARTUP_LOAD));
    result = kb_load(&data, base);
    dirty = result != 0;
    if (result) notify(kb_disk_message(result));
    else notify(tr(T_STARTUP_LOADED));
    for (;;) {
        selection_valid();
        list = kb_named_at(&data, KB_LIST, board, list_position);
        selected = kb_card_at(&data, lane, (Byte)list, card_position);
        draw_board();
        key = screen_key();
        if (key == KEY_LEFT && list_position) { --list_position; card_position = 0; }
        if (key == KEY_RIGHT && kb_named_at(&data, KB_LIST, board, list_position + 1) >= 0) {
            ++list_position; card_position = 0;
        }
        if (key == KEY_UP && card_position) --card_position;
        if (key == KEY_DOWN && kb_card_at(&data, lane, (Byte)list, card_position + 1) >= 0)
            ++card_position;
        if (key >= '1' && key <= '5') {
            position = (card_position / 5) * 5 + key - '1';
            selected = kb_card_at(&data, lane, (Byte)list, position);
            if (selected >= 0) { card_position = position; card_detail((Byte)selected); }
        }
        if (key == 13 && selected >= 0) card_detail((Byte)selected);
        if ((key == '+' || key == '-') && selected >= 0) {
            result = kb_reorder_card(&data, (Byte)selected, key == '+' ? 1 : -1);
            if (!result) { card_position = data.cards[selected].rank; changed(0); }
        }
        if (key == 'N' && input(tr(T_NEW_CARD), "", KB_TITLE, 0)) {
            result = kb_add_card(&data, board, lane, (Byte)list, edit);
            changed(result);
            if (result >= 0) card_position = data.cards[result].rank;
        }
        if (key == 'B') {
            result = choose(KB_BOARD, 0, 1);
            if (result >= 0) { board = (Byte)result; list_position = card_position = 0; }
        }
        if (key == 'U') {
            result = choose(KB_LANE, board, 1);
            if (result >= 0) { lane = (Byte)result; card_position = 0; }
        }
        if (key == 'T') {
            result = choose(KB_LIST, board, 1);
            if (result >= 0) {
                list_position = 0;
                while (kb_named_at(&data, KB_LIST, board, list_position) != result)
                    ++list_position;
                card_position = 0;
            }
        }
        if (key == 'H') help();
        if (key == 'F') settings();
        if (key == 'S') save();
        if (key == 'L' && (!dirty || confirm(tr(T_REPLACE)))) {
            row(24, tr(T_LOADING));
            result = kb_load(&data, base);
            if (!result) { dirty = 0; board = lane = list_position = card_position = 0; }
            if (result) notify(kb_disk_message(result));
            else notify(tr(T_LOADED));
        }
        if (key == 'Q') {
            if (!dirty) break;
            row(21, tr(T_QUIT_UNSAVED));
            row(22, tr(T_QUIT_CANCEL));
            key = screen_key();
            if (key == 'D' || (key == 'S' && !save())) break;
        }
    }
    screen_done();
    return 0;
}
