#ifndef KB_PLATFORM_H
#define KB_PLATFORM_H
#include "kanban.h"
#define KEY_UP 256
#define KEY_DOWN 257
#define KEY_LEFT 258
#define KEY_RIGHT 259
#define KEY_CLEAR 260
#define UI_WHITE 1
#define UI_RED 2
#define UI_GREEN 5
#define UI_YELLOW 7
#define UI_CYAN 3
void screen_init(void);
void screen_done(void);
void screen_clear(void);
void screen_text(Byte x, Byte y, const char *text, Byte width, Byte color, Byte reverse);
int screen_key(void);
#endif
