#ifndef KANBAN_H
#define KANBAN_H
#include "ascii.h"

#define KB_BOARDS 4
#define KB_LANES 12
#define KB_LISTS 16
#define KB_CARDS 40
#define KB_CHECKLISTS 20
#define KB_ITEMS 80
#define KB_TITLE 24
#define KB_DESCRIPTION 60
#define KB_NONE 255

#define KB_BOARD 0
#define KB_LANE 1
#define KB_LIST 2
#define KB_CHECKLIST 3
#define KB_FULL (-1)
#define KB_INVALID (-2)
#define KB_LAST (-3)

typedef unsigned char Byte;
/* Only byte fields: the version 1/2 disk payload is identical on both targets.
 * Size assertions in storage.c reject a compiler with incompatible padding. */
typedef struct { Byte used, parent; char title[KB_TITLE + 1]; } KbNamed;
typedef struct {
    Byte used, board, lane, list, rank, done, priority;
    char title[KB_TITLE + 1];
    char description[KB_DESCRIPTION + 1];
} KbCard;
typedef struct {
    Byte used, parent, done;
    char title[KB_TITLE + 1];
} KbItem;
typedef struct {
    KbNamed boards[KB_BOARDS], lanes[KB_LANES], lists[KB_LISTS];
    KbCard cards[KB_CARDS];
    KbNamed checklists[KB_CHECKLISTS];
    KbItem items[KB_ITEMS];
} KbData;

KbNamed *kb_pool(KbData *db, Byte kind, Byte *capacity);
int kb_named_at(KbData *db, Byte kind, Byte parent, Byte position);
int kb_add_named(KbData *db, Byte kind, Byte parent, const char *title);
int kb_add_board(KbData *db, const char *title);
int kb_delete_named(KbData *db, Byte kind, Byte id);
int kb_rename(KbData *db, Byte kind, Byte id, const char *title);
int kb_add_card(KbData *db, Byte board, Byte lane, Byte list, const char *title);
int kb_card_at(KbData *db, Byte lane, Byte list, Byte position);
int kb_move_card(KbData *db, Byte id, Byte board, Byte lane, Byte list);
int kb_reorder_card(KbData *db, Byte id, int direction);
int kb_delete_card(KbData *db, Byte id);
int kb_add_item(KbData *db, Byte checklist, const char *title);
int kb_item_at(KbData *db, Byte checklist, Byte position);
int kb_delete_item(KbData *db, Byte id);
void kb_progress(KbData *db, Byte card, Byte *done, Byte *total);
int kb_set_text(char *dest, const char *source, Byte limit, Byte allow_empty);
int kb_validate(KbData *db);
void kb_init(KbData *db);
#endif
