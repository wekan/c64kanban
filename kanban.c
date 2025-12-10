#include <stdio.h>
#include <conio.h>   // cputsxy, clrscr, textcolor, cgetc
#include <c64.h>     // COLOR_RED, COLOR_BLUE jne.

// --- GLOBAALIT MÄÄRITTELYT ---

// Määritellään Color RAM osoitin suoraan char-tyypiksi.
const char* COLOR_RAM_START = (const char*)0xD800;

// VIC-II rekisteri taustavärille (0xD021)
#define VIC_BG_COLOR (*(char*)0xD021)

#define SCREEN_WIDTH 40

// --- TÄRKEÄ KORJAUS: Määritellään rivimuuttuja globaaliksi ---
int current_row = 1; 
// -----------------------------------------------------------------

// --- FUNKTIOIDEN MÄÄRITTELYT ---

// Asettaa tietyn sarakkeen osan värin suoraan Värimuistiin
void set_color_direct(int x, int y, int length, unsigned char c64_color) {
    int offset = y * SCREEN_WIDTH + x;  
    int i;
    
    // Kirjoitetaan väriarvo suoraan Color RAMiin käyttämällä osoitinlaskentaa.
    for (i = 0; i < length; i++) {
        *( (char*)COLOR_RAM_START + offset + i ) = c64_color;
    }
}

void print_separator_row(int row) {
    cputsxy(0, row, "+-----------+-----------+-----------+");
}

void print_header_row(int row) {
    // 1. Tulosta tekstit (oletusväri)
    cputsxy(2, row, "TEHTAVAT");
    cputsxy(14, row, "KESKEN");
    cputsxy(26, row, "VALMIS");
    
    // 2. Aseta värit POKE:lla tekstin päälle
    set_color_direct(2, row, 8, COLOR_CYAN); 
    set_color_direct(14, row, 6, COLOR_CYAN); 
    set_color_direct(26, row, 6, COLOR_CYAN); 
}

void print_task_row(int row, const char *todo, const char *doing, const char *done) {
    
    // Erotinviivat (oletusväri)
    cputsxy(1, row, "|"); 
    cputsxy(12, row, "|"); 
    cputsxy(24, row, "|"); 
    cputsxy(36, row, "|"); 
    
    // 1. Tehtävät (TODO) - Tulostus
    cputsxy(2, row, todo);
    set_color_direct(2, row, 10, COLOR_RED); 

    // 2. Kesken (DOING) - Tulostus
    cputsxy(14, row, doing);
    set_color_direct(14, row, 10, COLOR_YELLOW);
    
    // 3. Valmis (DONE) - Tulostus
    cputsxy(26, row, done);
    set_color_direct(26, row, 10, COLOR_GREEN); 
}

int main(void) {
    
    // Asetetaan näytön taustaväri Siniseksi.
    VIC_BG_COLOR = COLOR_BLUE; 
    
    clrscr(); 
    textcolor(COLOR_WHITE);

    // Huom! current_row on nyt määritelty globaalisti, joten se ALKAA riviltä 1.
    // Emme määrittele sitä enää tässä.

    // Otsikko-osa
    print_separator_row(current_row++);
    print_header_row(current_row++);
    print_separator_row(current_row++);
    
    // Tehtävien rivit
    print_task_row(current_row++, "A. UI Desi", "B. Koodi", "C. Testi");
    print_task_row(current_row++, "D. Refa", "E. Kesken", "");
    print_task_row(current_row++, "F. Idea", "", "G. Tehty");
    
    // Viimeinen erotin
    print_separator_row(current_row++);
    
    // Lopetusviesti
    cputsxy(0, 24, "Paina mita tahansa nappainta...");
    
    cgetc(); 
    
    return 0;
}
