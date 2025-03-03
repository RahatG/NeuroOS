/**
 * console.c - Console implementation for NeuroOS
 * 
 * This file implements the console subsystem, which is responsible for
 * displaying text on the screen.
 */

#include "include/console.h"
#include <stdarg.h>

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA color attribute byte
#define VGA_COLOR(fg, bg) (((bg) << 4) | (fg))

// Console state
static struct {
    int x;                  // Cursor X position
    int y;                  // Cursor Y position
    console_color_t fg;     // Foreground color
    console_color_t bg;     // Background color
    uint16_t* buffer;       // VGA text buffer
    int cursor_enabled;     // Whether the cursor is enabled
} console;

/**
 * Set the VGA cursor position
 * 
 * @param x: X position (column)
 * @param y: Y position (row)
 */
static void vga_set_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    // Send the high byte
    __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)((pos >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
    
    // Send the low byte
    __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));
}

/**
 * Enable or disable the VGA cursor
 * 
 * @param enabled: Whether the cursor should be visible
 */
static void vga_set_cursor_enabled(int enabled) {
    if (enabled) {
        // Enable the cursor with a standard size (start=14, end=15)
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0A), "Nd"((uint16_t)0x3D4));
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D5));
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)0x3D4));
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D5));
    } else {
        // Disable the cursor by setting the start position beyond the end
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x0A), "Nd"((uint16_t)0x3D4));
        __asm__ volatile("outb %b0, %w1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x3D5));
    }
}

/**
 * Scroll the console up by one line
 */
static void console_scroll(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            console.buffer[y * VGA_WIDTH + x] = console.buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    uint8_t attribute = VGA_COLOR(console.fg, console.bg);
    uint16_t blank = ' ' | (attribute << 8);
    for (int x = 0; x < VGA_WIDTH; x++) {
        console.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
    
    // Move the cursor up
    console.y--;
}

/**
 * Initialize the console subsystem
 */
void console_init(void) {
    // Initialize console state
    console.x = 0;
    console.y = 0;
    console.fg = CONSOLE_COLOR_LIGHT_GREY;
    console.bg = CONSOLE_COLOR_BLACK;
    console.buffer = (uint16_t*)VGA_MEMORY;
    console.cursor_enabled = 1;
    
    // Clear the screen
    console_clear();
    
    // Enable the cursor
    vga_set_cursor_enabled(console.cursor_enabled);
}

/**
 * Clear the console screen
 */
void console_clear(void) {
    // Clear the screen with the current colors
    uint8_t attribute = VGA_COLOR(console.fg, console.bg);
    uint16_t blank = ' ' | (attribute << 8);
    
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            console.buffer[y * VGA_WIDTH + x] = blank;
        }
    }
    
    // Reset cursor position
    console.x = 0;
    console.y = 0;
    vga_set_cursor(console.x, console.y);
}

/**
 * Write a character to the console
 * 
 * @param c: The character to write
 */
void console_write_char(char c) {
    // Handle special characters
    if (c == '\n') {
        // Newline
        console.x = 0;
        console.y++;
    } else if (c == '\r') {
        // Carriage return
        console.x = 0;
    } else if (c == '\t') {
        // Tab (4 spaces)
        console.x = (console.x + 4) & ~3;
    } else if (c == '\b') {
        // Backspace
        if (console.x > 0) {
            console.x--;
            // Clear the character
            uint8_t attribute = VGA_COLOR(console.fg, console.bg);
            console.buffer[console.y * VGA_WIDTH + console.x] = ' ' | (attribute << 8);
        }
    } else {
        // Regular character
        uint8_t attribute = VGA_COLOR(console.fg, console.bg);
        console.buffer[console.y * VGA_WIDTH + console.x] = c | (attribute << 8);
        console.x++;
    }
    
    // Handle line wrapping
    if (console.x >= VGA_WIDTH) {
        console.x = 0;
        console.y++;
    }
    
    // Handle scrolling
    if (console.y >= VGA_HEIGHT) {
        console_scroll();
    }
    
    // Update cursor position
    vga_set_cursor(console.x, console.y);
}

/**
 * Write a string to the console
 * 
 * @param str: The string to write
 */
void console_write(const char* str) {
    while (*str) {
        console_write_char(*str++);
    }
}

/**
 * Write a string to the console with the specified color
 * 
 * @param str: The string to write
 * @param color: The color to use
 */
void console_write_color(const char* str, console_color_t color) {
    // Save the current color
    console_color_t old_fg = console.fg;
    
    // Set the new color
    console.fg = color;
    
    // Write the string
    console_write(str);
    
    // Restore the old color
    console.fg = old_fg;
}

/**
 * Format and print a number to the console
 * 
 * @param value: The value to print
 * @param base: The base to use (e.g., 10 for decimal, 16 for hex)
 * @param width: Minimum field width
 * @param pad_char: Character to use for padding
 * @param is_signed: Whether the value is signed
 */
static void console_print_number(unsigned long value, int base, int width, char pad_char, int is_signed) {
    // Handle negative numbers
    if (is_signed && (long)value < 0) {
        console_write_char('-');
        value = -(long)value;
    }
    
    // Convert the number to a string (in reverse)
    char buffer[32];
    int i = 0;
    
    if (value == 0) {
        buffer[i++] = '0';
    } else {
        while (value > 0) {
            int digit = value % base;
            buffer[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
            value /= base;
        }
    }
    
    // Pad to the specified width
    while (i < width) {
        buffer[i++] = pad_char;
    }
    
    // Print the number (in correct order)
    while (i > 0) {
        console_write_char(buffer[--i]);
    }
}

/**
 * Write a formatted string to the console
 * 
 * @param format: The format string
 * @param ...: The arguments to format
 */
void console_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    while (*format) {
        if (*format == '%') {
            format++;
            
            // Handle format specifiers
            int width = 0;
            char pad_char = ' ';
            
            // Check for zero padding
            if (*format == '0') {
                pad_char = '0';
                format++;
            }
            
            // Parse width
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
            
            // Handle format specifiers
            switch (*format) {
                case 'c': {
                    // Character
                    char c = (char)va_arg(args, int);
                    console_write_char(c);
                    break;
                }
                case 's': {
                    // String
                    const char* s = va_arg(args, const char*);
                    if (s == NULL) {
                        console_write("(null)");
                    } else {
                        console_write(s);
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    // Signed decimal
                    int value = va_arg(args, int);
                    console_print_number(value, 10, width, pad_char, 1);
                    break;
                }
                case 'u': {
                    // Unsigned decimal
                    unsigned int value = va_arg(args, unsigned int);
                    console_print_number(value, 10, width, pad_char, 0);
                    break;
                }
                case 'x': {
                    // Hexadecimal
                    unsigned int value = va_arg(args, unsigned int);
                    console_print_number(value, 16, width, pad_char, 0);
                    break;
                }
                case 'p': {
                    // Pointer
                    void* value = va_arg(args, void*);
                    console_write("0x");
                    console_print_number((unsigned long)value, 16, width, pad_char, 0);
                    break;
                }
                case '%': {
                    // Literal '%'
                    console_write_char('%');
                    break;
                }
                default: {
                    // Unknown format specifier
                    console_write_char('%');
                    console_write_char(*format);
                    break;
                }
            }
        } else {
            // Regular character
            console_write_char(*format);
        }
        
        format++;
    }
    
    va_end(args);
}

/**
 * Set the console cursor position
 * 
 * @param x: The x coordinate (column)
 * @param y: The y coordinate (row)
 */
void console_set_cursor(int x, int y) {
    // Clamp coordinates to valid range
    if (x < 0) x = 0;
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    
    // Update cursor position
    console.x = x;
    console.y = y;
    vga_set_cursor(x, y);
}

/**
 * Get the console cursor position
 * 
 * @param x: Pointer to store the x coordinate (column)
 * @param y: Pointer to store the y coordinate (row)
 */
void console_get_cursor(int* x, int* y) {
    if (x) *x = console.x;
    if (y) *y = console.y;
}

/**
 * Enable or disable the console cursor
 * 
 * @param enabled: Whether the cursor should be visible
 */
void console_set_cursor_enabled(int enabled) {
    console.cursor_enabled = enabled;
    vga_set_cursor_enabled(enabled);
}
