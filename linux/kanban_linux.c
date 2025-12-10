#include <stdio.h>

#define MAX_TASK_LEN 30

// ANSI Värikoodit
#define RESET       "\x1b[0m"
#define TODO_COLOR  "\x1b[31m"  // Punainen (Tehtävät)
#define DOING_COLOR "\x1b[33m"  // Keltainen (Kesken)
#define DONE_COLOR  "\x1b[32m"  // Vihreä (Valmis)
#define HEADER_BG   "\x1b[44m"  // Sininen tausta otsikoille
#define HEADER_FG   "\x1b[37m"  // Valkoinen etuala

void print_separator(void) {
    printf("+------------------------------+------------------------------+------------------------------+\n");
}

void print_header(void) {
    // Käytetään tausta- ja etualaväriä otsikoille
    printf(HEADER_BG HEADER_FG "| %-28s %-2s | %-28s %-2s | %-28s %-2s |" RESET "\n",
           "TEHTÄVÄT", "", "KESKEN", "", "VALMIS", "");
}

void print_task(const char *todo, const char *doing, const char *done) {
    
    // Alustetaan jokainen sarakepohja tyhjällä merkkijonolla ja oletusväreillä
    printf("| " TODO_COLOR "%-30s" RESET
           "| " DOING_COLOR "%-30s" RESET
           "| " DONE_COLOR "%-30s" RESET "|\n", 
           todo, doing, done);
}

int main(void) {
    
    printf("\n");
    print_separator();
    print_header();
    print_separator();
    
    // Esimerkkitehtäviä
    print_task("1. Suunnittele UI", "2. Koodaa perusrakenne", "3. Asenna ympäristö");
    print_task("4. Kirjoita testit", "5. Testaa ominaisuudet", "");
    print_task("6. Refaktoroi koodi", "", "");
    print_task("", "", "7. Julkaise Beta");

    print_separator();
    printf("\n");
    
    return 0;
}
