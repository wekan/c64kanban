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

// Full-width separator (single border spanning all lists)
void print_full_separator_row(int row) {
    cputsxy(0, row, "+---------------------------------------+");
}

// Board title centered across screen, colored green
void print_board_title_row(int row) {
    const char *title = "PROJECT SPACE TRAVEL";
    int title_len = 20; // "PROJECT SPACE TRAVEL" length
    int x = (SCREEN_WIDTH - title_len) / 2; // center horizontally (SCREEN_WIDTH is 40)
    cputsxy(x, row, title);
    set_color_direct(x, row, title_len, COLOR_GREEN);
}

// Swimlane row that spans the full width with borders and centered swimlane title in red
void print_swimlane_row(int row) {
    const char *swim = "TEAM RED";
    int swim_len = 8; // "TEAM RED" length
    // Inner width between the two '|' is 37 (columns 1..37)
    int inner_start = 1;
    int inner_width = 37;
    int x = inner_start + (inner_width - swim_len) / 2;

    // Draw vertical borders at left and right of the full-width swimlane (columns 0 and 38)
    cputsxy(0, row, "|");
    cputsxy(38, row, "|");

    cputsxy(x, row, swim);
    set_color_direct(x, row, swim_len, COLOR_RED);
}

void print_header_row(int row) {
    // Draw vertical separators / borders around the list titles so titles have left/right borders
    cputsxy(1, row, "|"); 
    cputsxy(12, row, "|"); 
    cputsxy(24, row, "|"); 
    cputsxy(36, row, "|"); 

    // 1. Print the texts (default color)
    cputsxy(2, row, "TASKS");
    // Move "IN PROGRESS" one character left (was 14 -> now 13)
    cputsxy(13, row, "IN PROGRESS");
    cputsxy(26, row, "DONE");
    
    // 2. Set colors in the Color RAM over the text
    set_color_direct(2, row, 8, COLOR_CYAN); 
    set_color_direct(13, row, 11, COLOR_CYAN); 
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

    // Top full border and board title + swimlane
    print_full_separator_row(current_row++);
    print_board_title_row(current_row++);
    print_full_separator_row(current_row++);
    print_swimlane_row(current_row++);
    print_full_separator_row(current_row++);

    // Header area (list titles with borders between them)
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
