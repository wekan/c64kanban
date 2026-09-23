#ifndef KB_I18N_H
#define KB_I18N_H
#include "kanban.h"
#define LANG_EN 0
#define LANG_FI 1
#define LANG_COUNT 2
extern Byte ui_language;
enum {
#define KB_TEXT(id, en, fi) T_ ## id,
#include "strings.def"
#undef KB_TEXT
    T_COUNT
};
const char *tr(Byte id);
const char *language_name(void);
#endif
