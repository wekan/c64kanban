#include <stdio.h>
#include <conio.h>   // cputsxy, clrscr, textcolor, cgetc
#include <c64.h>     // COLOR_RED, COLOR_BLUE jne.

// --- GLOBAL DEFINITIONS ---

// Define the Color RAM pointer directly as a char pointer.
const char* COLOR_RAM_START = (const char*)0xD800;

// VIC-II register for background color (0xD021)
#define VIC_BG_COLOR (*(char*)0xD021)

#define SCREEN_WIDTH 40

// --- IMPORTANT FIX: Define the row variable as global ---
int current_row = 1; 
// -----------------------------------------------------------------

// --- FUNCTION DEFINITIONS ---

// Sets the color for a part of a column directly into Color RAM
void set_color_direct(int x, int y, int length, unsigned char c64_color) {
    int offset = y * SCREEN_WIDTH + x;  
    int i;
    
    // Write the color value directly into the Color RAM using pointer arithmetic.
    for (i = 0; i < length; i++) {
        *( (char*)COLOR_RAM_START + offset + i ) = c64_color;
    }
}

void print_separator_row(int row) {
    cputsxy(0, row, "+-----------+-----------+-----------+");
}

void print_header_row(int row) {
    // 1. Print the texts (default color)
    cputsxy(2, row, "TASKS");
    cputsxy(14, row, "IN PROGRESS");
    cputsxy(26, row, "DONE");
    
    // 2. Set colors in the Color RAM over the text
    set_color_direct(2, row, 8, COLOR_CYAN); 
    set_color_direct(14, row, 10, COLOR_CYAN); 
    set_color_direct(26, row, 4, COLOR_CYAN); 
}

void print_task_row(int row, const char *todo, const char *doing, const char *done) {
    
    // Separators (default color)
    cputsxy(1, row, "|"); 
    cputsxy(12, row, "|"); 
    cputsxy(24, row, "|"); 
    cputsxy(36, row, "|"); 
    
    // 1. Tasks (TODO) - Print
    cputsxy(2, row, todo);
    set_color_direct(2, row, 10, COLOR_RED); 

    // 2. In Progress (DOING) - Print
    cputsxy(14, row, doing);
    set_color_direct(14, row, 10, COLOR_YELLOW);
    
    // 3. Done (DONE) - Print
    cputsxy(26, row, done);
    set_color_direct(26, row, 10, COLOR_GREEN); 
}

int main(void) {
    
    // Set the screen background color to blue.
    VIC_BG_COLOR = COLOR_BLUE; 
    
    clrscr(); 
    textcolor(COLOR_WHITE);

    // Note: current_row is now defined globally, so it STARTS at row 1.
    // We don't define it here anymore.

    // Header area
    print_separator_row(current_row++);
    print_header_row(current_row++);
    print_separator_row(current_row++);
    
    // Task rows
    print_task_row(current_row++, "A. UI Desi", "B. Code", "C. Test");
    print_task_row(current_row++, "D. Refac", "E. In Prog", "");
    print_task_row(current_row++, "F. Idea", "", "G. Done");
    
    // Final separator
    print_separator_row(current_row++);
    
    // Exit message
    cputsxy(0, 24, "Press any key...");
    
    cgetc(); 
    
    return 0;
}
