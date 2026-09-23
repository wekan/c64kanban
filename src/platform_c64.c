#include <conio.h>
#include <cbm.h>
#include "platform.h"

void screen_init(void)
{
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    cbm_k_bsout(142); /* Uppercase / graphics character set. */
    cursor(0);
    screen_clear();
}
void screen_done(void)
{
    revers(0);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
}
void screen_clear(void) { clrscr(); }
void screen_text(Byte x, Byte y, const char *text, Byte width, Byte color, Byte reverse)
{
    Byte c;
    gotoxy(x, y);
    textcolor(color);
    revers(reverse);
    while (width-- && x++ < 39) {
        c = (Byte)*text;
        if (c) ++text; else c = ' ';
        if (c >= 97 && c <= 122) c -= 32;
        cputc(c);
    }
    revers(0);
}
int screen_key(void)
{
    Byte c;
    c = (Byte)cgetc();
    switch (c) {
        case 3: return 27;
        case 13: return 13;
        case 20: return 8;
        case 145: return KEY_UP;
        case 17: return KEY_DOWN;
        case 157: return KEY_LEFT;
        case 29: return KEY_RIGHT;
        case 147: return KEY_CLEAR;
    }
    if (c >= 193 && c <= 218) c -= 128;
    return c;
}
