#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kanban.h"

static KbData db, before;
static void basics(void)
{
    int card, second, checklist, item, b, u, l;
    Byte done, total;
    kb_init(&db);
    assert(kb_validate(&db));
    assert(kb_delete_named(&db, KB_BOARD, 0) == KB_LAST);
    assert(kb_delete_named(&db, KB_LANE, 0) == KB_LAST);
    assert(kb_add_card(&db, 0, 0, 0, "   ") == KB_INVALID);
    assert(kb_add_card(&db, 0, 0, 0, "TOO LONG FOR A CARD TITLE!") == KB_INVALID);
    assert(kb_add_card(&db, 0, 0, 0, "BAD\nTEXT") == KB_INVALID);
    card = kb_add_card(&db, 0, 0, 0, "FIRST");
    second = kb_add_card(&db, 0, 0, 0, "SECOND");
    assert(card == 0 && second == 1);
    assert(kb_reorder_card(&db, 0, -1) == KB_INVALID);
    assert(!kb_reorder_card(&db, 1, -1));
    assert(kb_card_at(&db, 0, 0, 0) == 1);
    assert(!kb_reorder_card(&db, 1, 1));
    checklist = kb_add_named(&db, KB_CHECKLIST, 0, "RELEASE");
    item = kb_add_item(&db, (Byte)checklist, "BUILD");
    db.items[item].done = 1;
    assert(kb_add_item(&db, (Byte)checklist, "TEST") == 1);
    checklist = kb_add_named(&db, KB_CHECKLIST, 0, "DOCUMENTATION");
    assert(kb_add_item(&db, (Byte)checklist, "README") == 2);
    kb_progress(&db, 0, &done, &total);
    assert(done == 1 && total == 3);
    assert(!kb_set_text(db.cards[0].description, "A COMPLETE DESCRIPTION", KB_DESCRIPTION, 1));
    db.cards[0].priority = 2;
    db.cards[0].done = 1;
    b = kb_add_board(&db, "SECOND BOARD");
    assert(b == 1);
    u = kb_named_at(&db, KB_LANE, (Byte)b, 0);
    l = kb_named_at(&db, KB_LIST, (Byte)b, 1);
    memcpy(&before, &db, sizeof db);
    assert(kb_move_card(&db, 0, 0, (Byte)u, 0) == KB_INVALID);
    assert(!memcmp(&before, &db, sizeof db));
    assert(!kb_move_card(&db, 0, (Byte)b, (Byte)u, (Byte)l));
    assert(db.cards[1].rank == 0);
    assert(!strcmp(db.cards[0].description, "A COMPLETE DESCRIPTION"));
    kb_progress(&db, 0, &done, &total);
    assert(total == 3 && done == 1);
    assert(kb_validate(&db));
    assert(!kb_delete_named(&db, KB_BOARD, (Byte)b));
    assert(!db.cards[0].used && db.cards[1].used);
    assert(!db.items[0].used && !db.items[1].used && !db.items[2].used);
    assert(!db.checklists[0].used && !db.checklists[1].used);
    assert(kb_validate(&db));
    assert(kb_add_card(&db, 0, 0, 0, "REUSED") == 0);
    assert(db.cards[0].rank == 1 && !db.cards[0].description[0]);
    assert(!kb_delete_card(&db, 1));
    assert(db.cards[0].rank == 0);
    assert(!kb_delete_named(&db, KB_LIST, 0));
    assert(!db.cards[0].used && kb_validate(&db));
}
static void limits(void)
{
    int i, id;
    kb_init(&db);
    for (i = 1; i < KB_BOARDS; ++i) assert(kb_add_board(&db, "BOARD") == i);
    memcpy(&before, &db, sizeof db);
    assert(kb_add_board(&db, "OVERFLOW") == KB_FULL);
    assert(!memcmp(&before, &db, sizeof db));
    for (i = 0; i < KB_CARDS; ++i) assert(kb_add_card(&db, 0, 0, 0, "CARD") == i);
    assert(kb_add_card(&db, 0, 0, 0, "OVERFLOW") == KB_FULL);
    for (i = 0; i < KB_CHECKLISTS; ++i)
        assert(kb_add_named(&db, KB_CHECKLIST, 0, "CHECKLIST") == i);
    assert(kb_add_named(&db, KB_CHECKLIST, 0, "OVERFLOW") == KB_FULL);
    for (i = 0; i < KB_ITEMS; ++i) assert(kb_add_item(&db, 0, "ITEM") == i);
    assert(kb_add_item(&db, 0, "OVERFLOW") == KB_FULL);
    while ((id = kb_add_named(&db, KB_LANE, 0, "LANE")) >= 0) {}
    assert(id == KB_FULL);
    while ((id = kb_add_named(&db, KB_LIST, 0, "LIST")) >= 0) {}
    assert(id == KB_FULL && kb_validate(&db));
    assert(!kb_delete_named(&db, KB_CHECKLIST, 0));
    assert(kb_add_item(&db, 1, "REUSED") == 0);
    assert(!kb_delete_named(&db, KB_LANE, 0));
    assert(kb_validate(&db));
    for (i = 0; i < KB_CARDS; ++i) assert(!db.cards[i].used);
    /* Board creation must be atomic when there are too few list slots. */
    kb_init(&db);
    while (kb_add_named(&db, KB_LIST, 0, "LIST") >= 0) {}
    memcpy(&before, &db, sizeof db);
    assert(kb_add_board(&db, "NO LIST SPACE") == KB_FULL);
    assert(!memcmp(&before, &db, sizeof db));
}
static void validation(void)
{
    kb_init(&db);
    assert(kb_add_card(&db, 0, 0, 0, "CARD") == 0);
    assert(kb_add_card(&db, 0, 0, 0, "OTHER") == 1);
    memcpy(&before, &db, sizeof db);
    db.cards[1].rank = 0; assert(!kb_validate(&db)); db = before;
    db.cards[1].rank = 3; assert(!kb_validate(&db)); db = before;
    db.cards[1].lane = 254; assert(!kb_validate(&db)); db = before;
    db.cards[1].priority = 3; assert(!kb_validate(&db)); db = before;
    db.cards[1].done = 2; assert(!kb_validate(&db)); db = before;
    db.boards[0].parent = 1; assert(!kb_validate(&db)); db = before;
    db.lanes[0].used = 0; assert(!kb_validate(&db)); db = before;
    memset(db.cards[0].title, 'A', sizeof db.cards[0].title);
    assert(!kb_validate(&db)); db = before;
    db.items[0].used = 1; db.items[0].parent = 255;
    assert(!kb_validate(&db));
}
static void random_operations(void)
{
    int n, op;
    Byte id;
    kb_init(&db);
    assert(kb_add_named(&db, KB_LANE, 0, "SECOND LANE") == 1);
    srand(64);
    for (n = 0; n < 10000; ++n) {
        id = (Byte)(rand() % KB_CARDS);
        op = rand() % 7;
        switch (op) {
            case 0: kb_add_card(&db, 0, (Byte)(rand() % 2), (Byte)(rand() % 3), "TASK"); break;
            case 1: kb_delete_card(&db, id); break;
            case 2: kb_move_card(&db, id, 0, (Byte)(rand() % 2), (Byte)(rand() % 3)); break;
            case 3: kb_reorder_card(&db, id, rand() % 2 ? -1 : 1); break;
            case 4: kb_add_named(&db, KB_CHECKLIST, id, "CHECK"); break;
            case 5: kb_add_item(&db, (Byte)(rand() % KB_CHECKLISTS), "ITEM"); break;
            case 6: kb_delete_named(&db, KB_CHECKLIST, (Byte)(rand() % KB_CHECKLISTS)); break;
        }
        assert(kb_validate(&db));
    }
}
int main(void)
{
    basics(); limits(); validation(); random_operations();
    puts("model: CRUD, movement, cascades, capacity, validation + 10000 operations passed");
    return 0;
}
