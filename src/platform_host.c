#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include "platform.h"
static struct termios original;
static int raw;
void screen_init(void)
{
    struct termios mode;
    raw = isatty(STDIN_FILENO) && !tcgetattr(STDIN_FILENO, &original);
    if (raw) {
        mode = original;
        mode.c_lflag &= (tcflag_t)~(ICANON | ECHO);
        mode.c_cc[VMIN] = 1;
        mode.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &mode);
    }
    atexit(screen_done);
    fputs("\033[?25l", stdout);
}
void screen_done(void)
{
    if (raw) { tcsetattr(STDIN_FILENO, TCSANOW, &original); raw = 0; }
    fputs("\033[0m\033[?25h\033[26;1H", stdout);
    fflush(stdout);
}
void screen_clear(void) { fputs("\033[0m\033[2J\033[H", stdout); }
void screen_text(Byte x, Byte y, const char *text, Byte width, Byte color, Byte reverse)
{
    int ansi;
    ansi = 37;
    if (color == UI_RED) ansi = 31;
    if (color == UI_GREEN) ansi = 32;
    if (color == UI_YELLOW) ansi = 33;
    if (color == UI_CYAN) ansi = 36;
    printf("\033[%u;%uH\033[0;%d%sm", y + 1, x + 1, ansi, reverse ? ";7" : "");
    while (width-- && x++ < 39) {
        if (*text) putchar(*text++); else putchar(' ');
    }
}
static int byte_in(void)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) exit(0);
    return c;
}
int screen_key(void)
{
    int c;
    fd_set set;
    struct timeval timeout;
    fflush(stdout);
    c = byte_in();
    if (c == 27) {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        timeout.tv_sec = 0;
        timeout.tv_usec = 80000;
        if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
            c = byte_in();
            if (c == '[') {
                c = byte_in();
                if (c == 'A') return KEY_UP;
                if (c == 'B') return KEY_DOWN;
                if (c == 'C') return KEY_RIGHT;
                if (c == 'D') return KEY_LEFT;
            }
        }
        return 27;
    }
    if (c == 10) return 13;
    if (c == 127) return 8;
    if (c == 21) return KEY_CLEAR;
    if (c >= 'a' && c <= 'z') c -= 32;
    return c;
}
