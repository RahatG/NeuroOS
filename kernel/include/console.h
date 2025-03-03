/**
 * console.h - Console interface for NeuroOS
 * 
 * This file defines the interface for the console subsystem, which is
 * responsible for displaying text on the screen.
 */

#ifndef NEUROOS_CONSOLE_H
#define NEUROOS_CONSOLE_H

#include <stdint.h>
#include <stddef.h>

// Console colors
typedef enum {
    CONSOLE_COLOR_BLACK = 0,
    CONSOLE_COLOR_BLUE = 1,
    CONSOLE_COLOR_GREEN = 2,
    CONSOLE_COLOR_CYAN = 3,
    CONSOLE_COLOR_RED = 4,
    CONSOLE_COLOR_MAGENTA = 5,
    CONSOLE_COLOR_BROWN = 6,
    CONSOLE_COLOR_LIGHT_GREY = 7,
    CONSOLE_COLOR_DARK_GREY = 8,
    CONSOLE_COLOR_LIGHT_BLUE = 9,
    CONSOLE_COLOR_LIGHT_GREEN = 10,
    CONSOLE_COLOR_LIGHT_CYAN = 11,
    CONSOLE_COLOR_LIGHT_RED = 12,
    CONSOLE_COLOR_LIGHT_MAGENTA = 13,
    CONSOLE_COLOR_LIGHT_BROWN = 14,
    CONSOLE_COLOR_WHITE = 15,
    
    // Aliases for common colors
    CONSOLE_COLOR_BRIGHT_BLUE = CONSOLE_COLOR_LIGHT_BLUE,
    CONSOLE_COLOR_BRIGHT_GREEN = CONSOLE_COLOR_LIGHT_GREEN,
    CONSOLE_COLOR_BRIGHT_CYAN = CONSOLE_COLOR_LIGHT_CYAN,
    CONSOLE_COLOR_BRIGHT_RED = CONSOLE_COLOR_LIGHT_RED,
    CONSOLE_COLOR_BRIGHT_MAGENTA = CONSOLE_COLOR_LIGHT_MAGENTA,
    CONSOLE_COLOR_YELLOW = CONSOLE_COLOR_LIGHT_BROWN
} console_color_t;

/**
 * Initialize the console subsystem
 * 
 * This function initializes the console subsystem and clears the screen.
 */
void console_init(void);

/**
 * Clear the console screen
 * 
 * This function clears the console screen and resets the cursor position.
 */
void console_clear(void);

/**
 * Write a string to the console
 * 
 * @param str: The string to write
 */
void console_write(const char* str);

/**
 * Write a character to the console
 * 
 * @param c: The character to write
 */
void console_write_char(char c);

/**
 * Write a string to the console with the specified color
 * 
 * @param str: The string to write
 * @param color: The color to use
 */
void console_write_color(const char* str, console_color_t color);

/**
 * Write a formatted string to the console
 * 
 * @param format: The format string
 * @param ...: The arguments to format
 */
void console_printf(const char* format, ...);

/**
 * Set the console cursor position
 * 
 * @param x: The x coordinate (column)
 * @param y: The y coordinate (row)
 */
void console_set_cursor(int x, int y);

/**
 * Get the console cursor position
 * 
 * @param x: Pointer to store the x coordinate (column)
 * @param y: Pointer to store the y coordinate (row)
 */
void console_get_cursor(int* x, int* y);

/**
 * Enable or disable the console cursor
 * 
 * @param enabled: Whether the cursor should be visible
 */
void console_set_cursor_enabled(int enabled);

#endif /* NEUROOS_CONSOLE_H */
