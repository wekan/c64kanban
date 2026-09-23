#include "i18n.h"
Byte ui_language = LANG_EN;
static const char * const english[] = {
#define KB_TEXT(id, en, fi) en,
#include "strings.def"
#undef KB_TEXT
};
static const char * const finnish[] = {
#define KB_TEXT(id, en, fi) fi,
#include "strings.def"
#undef KB_TEXT
};
const char *tr(Byte id)
{
    if (id >= T_COUNT) return "?";
    return ui_language == LANG_FI ? finnish[id] : english[id];
}
const char *language_name(void)
{
    return ui_language == LANG_FI ? "SUOMI" : "ENGLISH";
}
