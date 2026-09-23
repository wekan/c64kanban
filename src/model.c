#include <string.h>
#include "kanban.h"

static int text_valid(const char *text, Byte limit, Byte empty)
{
    Byte i, nonspace;
    nonspace = 0;
    for (i = 0; i <= limit; ++i) {
        if (!text[i]) return empty || nonspace;
        if ((Byte)text[i] < 32 || (Byte)text[i] > 95) return 0;
        if (text[i] != ' ') nonspace = 1;
    }
    return 0;
}

int kb_set_text(char *dest, const char *source, Byte limit, Byte allow_empty)
{
    if (!text_valid(source, limit, allow_empty)) return KB_INVALID;
    /* Copy first, then clear padding; also supports editing in place. */
    memmove(dest, source, strlen(source) + 1);
    memset(dest + strlen(dest), 0, limit + 1 - strlen(dest));
    return 0;
}

KbNamed *kb_pool(KbData *db, Byte kind, Byte *capacity)
{
    switch (kind) {
        case KB_BOARD: *capacity = KB_BOARDS; return db->boards;
        case KB_LANE: *capacity = KB_LANES; return db->lanes;
        case KB_LIST: *capacity = KB_LISTS; return db->lists;
        case KB_CHECKLIST: *capacity = KB_CHECKLISTS; return db->checklists;
    }
    *capacity = 0;
    return db->boards;
}

int kb_named_at(KbData *db, Byte kind, Byte parent, Byte position)
{
    Byte i, capacity;
    KbNamed *pool;
    pool = kb_pool(db, kind, &capacity);
    for (i = 0; i < capacity; ++i)
        if (pool[i].used && pool[i].parent == parent) {
            if (!position) return i;
            --position;
        }
    return KB_INVALID;
}

int kb_add_named(KbData *db, Byte kind, Byte parent, const char *title)
{
    Byte i, capacity;
    KbNamed *pool;
    if (!text_valid(title, KB_TITLE, 0)) return KB_INVALID;
    if (kind == KB_BOARD) {
        if (parent) return KB_INVALID;
    } else if (kind == KB_CHECKLIST) {
        if (parent >= KB_CARDS || !db->cards[parent].used) return KB_INVALID;
    } else if (kind == KB_LANE || kind == KB_LIST) {
        if (parent >= KB_BOARDS || !db->boards[parent].used) return KB_INVALID;
    } else return KB_INVALID;
    pool = kb_pool(db, kind, &capacity);
    for (i = 0; i < capacity; ++i) if (!pool[i].used) {
        memset(&pool[i], 0, sizeof(KbNamed));
        pool[i].used = 1;
        pool[i].parent = parent;
        kb_set_text(pool[i].title, title, KB_TITLE, 0);
        return i;
    }
    return KB_FULL;
}

int kb_add_board(KbData *db, const char *title)
{
    Byte i, lanes, lists;
    int id;
    lanes = lists = 0;
    for (i = 0; i < KB_LANES; ++i) if (!db->lanes[i].used) ++lanes;
    for (i = 0; i < KB_LISTS; ++i) if (!db->lists[i].used) ++lists;
    if (!lanes || lists < 3) return KB_FULL;
    id = kb_add_named(db, KB_BOARD, 0, title);
    if (id < 0) return id;
    kb_add_named(db, KB_LANE, (Byte)id, "GENERAL");
    kb_add_named(db, KB_LIST, (Byte)id, "TODO");
    kb_add_named(db, KB_LIST, (Byte)id, "DOING");
    kb_add_named(db, KB_LIST, (Byte)id, "DONE");
    return id;
}

int kb_rename(KbData *db, Byte kind, Byte id, const char *title)
{
    Byte capacity;
    KbNamed *pool;
    pool = kb_pool(db, kind, &capacity);
    if (id >= capacity || !pool[id].used) return KB_INVALID;
    return kb_set_text(pool[id].title, title, KB_TITLE, 0);
}

static int cell_valid(KbData *db, Byte board, Byte lane, Byte list)
{
    return board < KB_BOARDS && db->boards[board].used &&
        lane < KB_LANES && db->lanes[lane].used && db->lanes[lane].parent == board &&
        list < KB_LISTS && db->lists[list].used && db->lists[list].parent == board;
}

static Byte card_count(KbData *db, Byte lane, Byte list)
{
    Byte i, count;
    count = 0;
    for (i = 0; i < KB_CARDS; ++i)
        if (db->cards[i].used && db->cards[i].lane == lane &&
            db->cards[i].list == list) ++count;
    return count;
}

int kb_card_at(KbData *db, Byte lane, Byte list, Byte position)
{
    Byte i;
    for (i = 0; i < KB_CARDS; ++i)
        if (db->cards[i].used && db->cards[i].lane == lane &&
            db->cards[i].list == list && db->cards[i].rank == position) return i;
    return KB_INVALID;
}

int kb_add_card(KbData *db, Byte board, Byte lane, Byte list, const char *title)
{
    Byte i;
    KbCard *card;
    if (!cell_valid(db, board, lane, list) || !text_valid(title, KB_TITLE, 0))
        return KB_INVALID;
    for (i = 0; i < KB_CARDS; ++i) if (!db->cards[i].used) {
        card = &db->cards[i];
        memset(card, 0, sizeof(KbCard));
        card->rank = card_count(db, lane, list);
        card->used = 1;
        card->board = board;
        card->lane = lane;
        card->list = list;
        kb_set_text(card->title, title, KB_TITLE, 0);
        return i;
    }
    return KB_FULL;
}

static void close_rank(KbData *db, Byte id)
{
    Byte i;
    KbCard *card;
    card = &db->cards[id];
    for (i = 0; i < KB_CARDS; ++i)
        if (db->cards[i].used && db->cards[i].lane == card->lane &&
            db->cards[i].list == card->list && db->cards[i].rank > card->rank)
            --db->cards[i].rank;
}

int kb_move_card(KbData *db, Byte id, Byte board, Byte lane, Byte list)
{
    KbCard *card;
    if (id >= KB_CARDS || !db->cards[id].used || !cell_valid(db, board, lane, list))
        return KB_INVALID;
    card = &db->cards[id];
    if (card->lane == lane && card->list == list) return 0;
    close_rank(db, id);
    card->rank = card_count(db, lane, list);
    card->board = board;
    card->lane = lane;
    card->list = list;
    return 0;
}

int kb_reorder_card(KbData *db, Byte id, int direction)
{
    int neighbor, rank;
    KbCard *card;
    if (id >= KB_CARDS || !db->cards[id].used ||
        (direction != -1 && direction != 1)) return KB_INVALID;
    card = &db->cards[id];
    rank = card->rank + direction;
    if (rank < 0) return KB_INVALID;
    neighbor = kb_card_at(db, card->lane, card->list, (Byte)rank);
    if (neighbor < 0) return KB_INVALID;
    db->cards[neighbor].rank = card->rank;
    card->rank = (Byte)rank;
    return 0;
}

int kb_delete_item(KbData *db, Byte id)
{
    if (id >= KB_ITEMS || !db->items[id].used) return KB_INVALID;
    memset(&db->items[id], 0, sizeof(KbItem));
    return 0;
}

int kb_delete_card(KbData *db, Byte id)
{
    Byte i;
    if (id >= KB_CARDS || !db->cards[id].used) return KB_INVALID;
    for (i = 0; i < KB_CHECKLISTS; ++i)
        if (db->checklists[i].used && db->checklists[i].parent == id)
            kb_delete_named(db, KB_CHECKLIST, i);
    close_rank(db, id);
    memset(&db->cards[id], 0, sizeof(KbCard));
    return 0;
}

int kb_delete_named(KbData *db, Byte kind, Byte id)
{
    Byte capacity, i;
    KbNamed *pool;
    pool = kb_pool(db, kind, &capacity);
    if (id >= capacity || !pool[id].used) return KB_INVALID;
    if (kind != KB_CHECKLIST && kb_named_at(db, kind, pool[id].parent, 1) < 0)
        return KB_LAST;
    if (kind == KB_CHECKLIST) {
        for (i = 0; i < KB_ITEMS; ++i)
            if (db->items[i].used && db->items[i].parent == id) kb_delete_item(db, i);
    } else {
        for (i = 0; i < KB_CARDS; ++i) if (db->cards[i].used &&
            ((kind == KB_BOARD && db->cards[i].board == id) ||
             (kind == KB_LANE && db->cards[i].lane == id) ||
             (kind == KB_LIST && db->cards[i].list == id))) kb_delete_card(db, i);
        if (kind == KB_BOARD) {
            for (i = 0; i < KB_LANES; ++i)
                if (db->lanes[i].used && db->lanes[i].parent == id)
                    memset(&db->lanes[i], 0, sizeof(KbNamed));
            for (i = 0; i < KB_LISTS; ++i)
                if (db->lists[i].used && db->lists[i].parent == id)
                    memset(&db->lists[i], 0, sizeof(KbNamed));
        }
    }
    memset(&pool[id], 0, sizeof(KbNamed));
    return 0;
}

int kb_add_item(KbData *db, Byte checklist, const char *title)
{
    Byte i;
    if (checklist >= KB_CHECKLISTS || !db->checklists[checklist].used ||
        !text_valid(title, KB_TITLE, 0)) return KB_INVALID;
    for (i = 0; i < KB_ITEMS; ++i) if (!db->items[i].used) {
        memset(&db->items[i], 0, sizeof(KbItem));
        db->items[i].used = 1;
        db->items[i].parent = checklist;
        kb_set_text(db->items[i].title, title, KB_TITLE, 0);
        return i;
    }
    return KB_FULL;
}

int kb_item_at(KbData *db, Byte checklist, Byte position)
{
    Byte i;
    for (i = 0; i < KB_ITEMS; ++i)
        if (db->items[i].used && db->items[i].parent == checklist) {
            if (!position) return i;
            --position;
        }
    return KB_INVALID;
}

void kb_progress(KbData *db, Byte card, Byte *done, Byte *total)
{
    Byte i;
    *done = *total = 0;
    for (i = 0; i < KB_ITEMS; ++i) if (db->items[i].used &&
        db->checklists[db->items[i].parent].parent == card) {
        ++*total;
        if (db->items[i].done) ++*done;
    }
}

int kb_validate(KbData *db)
{
    Byte kind, cap, i, j;
    KbNamed *pool;
    KbCard *card;
    if (kb_named_at(db, KB_BOARD, 0, 0) < 0) return 0;
    for (kind = 0; kind <= KB_CHECKLIST; ++kind) {
        pool = kb_pool(db, kind, &cap);
        for (i = 0; i < cap; ++i) {
            if (pool[i].used > 1) return 0;
            if (!pool[i].used) continue;
            if (!text_valid(pool[i].title, KB_TITLE, 0)) return 0;
            j = pool[i].parent;
            if (kind == KB_BOARD) {
                if (j || kb_named_at(db, KB_LANE, i, 0) < 0 ||
                    kb_named_at(db, KB_LIST, i, 0) < 0) return 0;
            } else if (kind == KB_CHECKLIST) {
                if (j >= KB_CARDS || !db->cards[j].used) return 0;
            } else if (j >= KB_BOARDS || !db->boards[j].used) return 0;
        }
    }
    for (i = 0; i < KB_CARDS; ++i) {
        card = &db->cards[i];
        if (card->used > 1) return 0;
        if (!card->used) continue;
        if (!cell_valid(db, card->board, card->lane, card->list) ||
            card->done > 1 || card->priority > 2 ||
            !text_valid(card->title, KB_TITLE, 0) ||
            !text_valid(card->description, KB_DESCRIPTION, 1) ||
            card->rank >= card_count(db, card->lane, card->list)) return 0;
        for (j = i + 1; j < KB_CARDS; ++j)
            if (db->cards[j].used && db->cards[j].lane == card->lane &&
                db->cards[j].list == card->list && db->cards[j].rank == card->rank)
                return 0;
    }
    for (i = 0; i < KB_ITEMS; ++i) {
        if (db->items[i].used > 1) return 0;
        if (!db->items[i].used) continue;
        j = db->items[i].parent;
        if (j >= KB_CHECKLISTS || !db->checklists[j].used || db->items[i].done > 1 ||
            !text_valid(db->items[i].title, KB_TITLE, 0)) return 0;
    }
    return 1;
}

void kb_init(KbData *db)
{
    memset(db, 0, sizeof(KbData));
    kb_add_board(db, "MY BOARD");
}
