/**
 * gui.c - Graphical User Interface implementation for NeuroOS
 * 
 * This file implements the GUI subsystem, which provides a Linux/macOS-like
 * graphical user interface for NeuroOS.
 */

#include "gui/gui.h"
#include <string.h>
#include <stdlib.h>

// Frame buffer information
static uint32_t* framebuffer = NULL;
static int framebuffer_width = 0;
static int framebuffer_height = 0;
static int framebuffer_pitch = 0;
static int framebuffer_bpp = 0;

// Window structure implementation
struct gui_window {
    char title[256];
    int x;
    int y;
    int width;
    int height;
    int min_width;
    int min_height;
    int max_width;
    int max_height;
    uint32_t flags;
    gui_color_t background_color;
    int visible;
    int focused;
    gui_event_callback_t event_callback;
    void* user_data;
    gui_widget_t* widgets;
    int widget_count;
    struct gui_window* next;
};

// Widget structure implementation
struct gui_widget {
    gui_window_t* window;
    gui_widget_type_t type;
    char text[256];
    int x;
    int y;
    int width;
    int height;
    int visible;
    int enabled;
    gui_font_t font;
    gui_color_t background_color;
    gui_color_t foreground_color;
    gui_event_callback_t event_callback;
    void* user_data;
    void* type_data;
    struct gui_widget* next;
};

// Global state
static int gui_initialized = 0;
static gui_window_t* window_list = NULL;
static gui_window_t* focused_window = NULL;
static gui_theme_t current_theme = GUI_THEME_LIGHT;

// Theme colors
static gui_color_t theme_colors[2][10] = {
    // Light theme
    {
        {240, 240, 240, 255}, // Window background
        {255, 255, 255, 255}, // Widget background
        {0, 0, 0, 255},       // Text color
        {230, 230, 230, 255}, // Button background
        {200, 200, 200, 255}, // Button border
        {0, 120, 215, 255},   // Highlight color
        {240, 240, 240, 255}, // Menu background
        {230, 230, 230, 255}, // Taskbar background
        {200, 200, 200, 255}, // Border color
        {150, 150, 150, 255}  // Disabled color
    },
    // Dark theme
    {
        {30, 30, 30, 255},    // Window background
        {50, 50, 50, 255},    // Widget background
        {255, 255, 255, 255}, // Text color
        {70, 70, 70, 255},    // Button background
        {100, 100, 100, 255}, // Button border
        {0, 120, 215, 255},   // Highlight color
        {40, 40, 40, 255},    // Menu background
        {35, 35, 35, 255},    // Taskbar background
        {80, 80, 80, 255},    // Border color
        {120, 120, 120, 255}  // Disabled color
    }
};

// Desktop state
static gui_color_t desktop_background_color = {0, 120, 215, 255};
static char desktop_background_image[256] = {0};
static int taskbar_visible = 1;
static int start_menu_visible = 0;

// Forward declarations of static functions
static void gui_render_window(gui_window_t* window);
static void gui_render_widget(gui_window_t* window, gui_widget_t* widget);
static void gui_render_desktop(void);
static void gui_render_taskbar(void);
static void gui_render_start_menu(void);
/* Forward declarations for event handling functions - to be implemented later */
static void gui_handle_mouse_event(int x, int y, int button, gui_event_type_t type) {
    /* Stub implementation - to be completed in future updates */
    (void)x;
    (void)y;
    (void)button;
    (void)type;
}

static void gui_handle_key_event(int key_code, int modifiers, gui_event_type_t type) {
    /* Stub implementation - to be completed in future updates */
    (void)key_code;
    (void)modifiers;
    (void)type;
}

static gui_window_t* gui_window_at_position(int x, int y) {
    /* Stub implementation - to be completed in future updates */
    (void)x;
    (void)y;
    return NULL;
}

static gui_widget_t* gui_widget_at_position(gui_window_t* window, int x, int y) {
    /* Stub implementation - to be completed in future updates */
    (void)window;
    (void)x;
    (void)y;
    return NULL;
}
static void gui_bring_window_to_front(gui_window_t* window);
static void gui_update_framebuffer(void);

/**
 * Initialize the GUI subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int gui_init(void) {
    // Check if the GUI is already initialized
    if (gui_initialized) {
        return 0;
    }
    
    // Initialize the frame buffer
    framebuffer_width = 1024;
    framebuffer_height = 768;
    framebuffer_bpp = 32;
    framebuffer_pitch = framebuffer_width * (framebuffer_bpp / 8);
    
    framebuffer = (uint32_t*)malloc(framebuffer_height * framebuffer_pitch);
    if (!framebuffer) {
        return -1;
    }
    
    // Clear the frame buffer
    memset(framebuffer, 0, framebuffer_height * framebuffer_pitch);
    
    // Set the initialized flag
    gui_initialized = 1;
    
    // Render the desktop
    gui_render_desktop();
    
    return 0;
}

/**
 * Shutdown the GUI subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int gui_shutdown(void) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return 0;
    }
    
    // Free all windows and widgets
    gui_window_t* window = window_list;
    while (window) {
        gui_window_t* next_window = window->next;
        
        // Free all widgets
        gui_widget_t* widget = window->widgets;
        while (widget) {
            gui_widget_t* next_widget = widget->next;
            
            // Free widget type-specific data
            if (widget->type_data) {
                free(widget->type_data);
            }
            
            free(widget);
            widget = next_widget;
        }
        
        free(window);
        window = next_window;
    }
    
    // Free the frame buffer
    if (framebuffer) {
        free(framebuffer);
        framebuffer = NULL;
    }
    
    // Reset the initialized flag
    gui_initialized = 0;
    
    return 0;
}

/**
 * Create a new window
 */
gui_window_t* gui_window_create(const char* title, int x, int y, int width, int height, uint32_t flags) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return NULL;
    }
    
    // Allocate memory for the window
    gui_window_t* window = (gui_window_t*)malloc(sizeof(gui_window_t));
    if (!window) {
        return NULL;
    }
    
    // Initialize the window
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->min_width = 100;
    window->min_height = 100;
    window->max_width = framebuffer_width;
    window->max_height = framebuffer_height;
    window->flags = flags;
    window->background_color = theme_colors[current_theme][0];
    window->visible = 0;
    window->focused = 0;
    window->event_callback = NULL;
    window->user_data = NULL;
    window->widgets = NULL;
    window->widget_count = 0;
    window->next = NULL;
    
    // Add the window to the window list
    if (!window_list) {
        window_list = window;
    } else {
        gui_window_t* last_window = window_list;
        while (last_window->next) {
            last_window = last_window->next;
        }
        last_window->next = window;
    }
    
    return window;
}

/**
 * Render the desktop
 */
static void gui_render_desktop(void) {
    // Check if the GUI is initialized
    if (!gui_initialized || !framebuffer) {
        return;
    }
    
    // Fill the frame buffer with the desktop background color
    for (int y = 0; y < framebuffer_height; y++) {
        for (int x = 0; x < framebuffer_width; x++) {
            uint32_t pixel = (desktop_background_color.a << 24) |
                             (desktop_background_color.r << 16) |
                             (desktop_background_color.g << 8) |
                             (desktop_background_color.b);
            
            framebuffer[y * framebuffer_width + x] = pixel;
        }
    }
    
    // Render the taskbar if it's visible
    if (taskbar_visible) {
        gui_render_taskbar();
    }
    
    // Render the start menu if it's visible
    if (start_menu_visible) {
        gui_render_start_menu();
    }
    
    // Render all visible windows
    gui_window_t* window = window_list;
    while (window) {
        if (window->visible) {
            gui_render_window(window);
        }
        window = window->next;
    }
}

/**
 * Render a window
 */
static void gui_render_window(gui_window_t* window) {
    // Check if the GUI is initialized and the window is valid
    if (!gui_initialized || !window || !framebuffer) {
        return;
    }
    
    // Fill the window with the background color
    for (int y = 0; y < window->height; y++) {
        for (int x = 0; x < window->width; x++) {
            int screen_x = window->x + x;
            int screen_y = window->y + y;
            
            if (screen_x >= 0 && screen_x < framebuffer_width &&
                screen_y >= 0 && screen_y < framebuffer_height) {
                uint32_t pixel = (window->background_color.a << 24) |
                                 (window->background_color.r << 16) |
                                 (window->background_color.g << 8) |
                                 (window->background_color.b);
                
                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
            }
        }
    }
    
    // Draw the window border
    gui_color_t border_color = theme_colors[current_theme][8];
    
    // Top border
    for (int x = 0; x < window->width; x++) {
        int screen_x = window->x + x;
        int screen_y = window->y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Bottom border
    for (int x = 0; x < window->width; x++) {
        int screen_x = window->x + x;
        int screen_y = window->y + window->height - 1;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Left border
    for (int y = 0; y < window->height; y++) {
        int screen_x = window->x;
        int screen_y = window->y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Right border
    for (int y = 0; y < window->height; y++) {
        int screen_x = window->x + window->width - 1;
        int screen_y = window->y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Render all visible widgets
    gui_widget_t* widget = window->widgets;
    while (widget) {
        if (widget->visible) {
            gui_render_widget(window, widget);
        }
        widget = widget->next;
    }
}

/**
 * Render a widget
 */
static void gui_render_widget(gui_window_t* window, gui_widget_t* widget) {
    // Check if the GUI is initialized and the window and widget are valid
    if (!gui_initialized || !window || !widget || !framebuffer) {
        return;
    }
    
    // Calculate the absolute position of the widget
    int abs_x = window->x + widget->x;
    int abs_y = window->y + widget->y;
    
    // Fill the widget with the background color
    for (int y = 0; y < widget->height; y++) {
        for (int x = 0; x < widget->width; x++) {
            int screen_x = abs_x + x;
            int screen_y = abs_y + y;
            
            if (screen_x >= window->x && screen_x < window->x + window->width &&
                screen_y >= window->y && screen_y < window->y + window->height &&
                screen_x >= 0 && screen_x < framebuffer_width &&
                screen_y >= 0 && screen_y < framebuffer_height) {
                
                uint32_t pixel = (widget->background_color.a << 24) |
                                 (widget->background_color.r << 16) |
                                 (widget->background_color.g << 8) |
                                 (widget->background_color.b);
                
                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
            }
        }
    }
    
    // Render the widget based on its type
    switch (widget->type) {
        case GUI_WIDGET_BUTTON: {
            // Draw button border
            gui_color_t border_color = theme_colors[current_theme][4];
            
            // Top border
            for (int x = 0; x < widget->width; x++) {
                int screen_x = abs_x + x;
                int screen_y = abs_y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Bottom border
            for (int x = 0; x < widget->width; x++) {
                int screen_x = abs_x + x;
                int screen_y = abs_y + widget->height - 1;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Left border
            for (int y = 0; y < widget->height; y++) {
                int screen_x = abs_x;
                int screen_y = abs_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Right border
            for (int y = 0; y < widget->height; y++) {
                int screen_x = abs_x + widget->width - 1;
                int screen_y = abs_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Draw button text
            // Use the font rendering system to draw text
            
            size_t text_len = strlen(widget->text);
            int text_x = abs_x + (widget->width - (int)(text_len * 8)) / 2; // Assume 8 pixels per character
            int text_y = abs_y + (widget->height - 8) / 2; // Assume 8 pixels height
            
            for (size_t i = 0; i < strlen(widget->text); i++) {
                int char_x = text_x + i * 8;
                
                // Draw a simple representation of the character
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 6; x++) {
                        int screen_x = char_x + x;
                        int screen_y = text_y + y;
                        
                        if (screen_x >= window->x && screen_x < window->x + window->width &&
                            screen_y >= window->y && screen_y < window->y + window->height &&
                            screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Simple font rendering - just draw a filled rectangle for each character
                            if (x > 0 && x < 5 && y > 0 && y < 7) {
                                uint32_t pixel = (widget->foreground_color.a << 24) |
                                                 (widget->foreground_color.r << 16) |
                                                 (widget->foreground_color.g << 8) |
                                                 (widget->foreground_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case GUI_WIDGET_LABEL: {
            // Draw label text
            // Use the font rendering system to draw text
            
            int text_x = abs_x; // No need to calculate text width for labels as they align left
            int text_y = abs_y + (widget->height - 8) / 2; // Assume 8 pixels height
            
            for (size_t i = 0; i < strlen(widget->text); i++) {
                int char_x = text_x + (int)i * 8;
                
                // Draw a simple representation of the character
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 6; x++) {
                        int screen_x = char_x + x;
                        int screen_y = text_y + y;
                        
                        if (screen_x >= window->x && screen_x < window->x + window->width &&
                            screen_y >= window->y && screen_y < window->y + window->height &&
                            screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Simple font rendering - just draw a filled rectangle for each character
                            if (x > 0 && x < 5 && y > 0 && y < 7) {
                                uint32_t pixel = (widget->foreground_color.a << 24) |
                                                 (widget->foreground_color.r << 16) |
                                                 (widget->foreground_color.g << 8) |
                                                 (widget->foreground_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case GUI_WIDGET_TEXTBOX: {
            // Draw textbox border
            gui_color_t border_color = theme_colors[current_theme][8];
            
            // Top border
            for (int x = 0; x < widget->width; x++) {
                int screen_x = abs_x + x;
                int screen_y = abs_y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Bottom border
            for (int x = 0; x < widget->width; x++) {
                int screen_x = abs_x + x;
                int screen_y = abs_y + widget->height - 1;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Left border
            for (int y = 0; y < widget->height; y++) {
                int screen_x = abs_x;
                int screen_y = abs_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Right border
            for (int y = 0; y < widget->height; y++) {
                int screen_x = abs_x + widget->width - 1;
                int screen_y = abs_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Draw textbox text
            // Use the font rendering system to draw text
            
            int text_x = abs_x + 5; // 5 pixel padding
            int text_y = abs_y + (widget->height - 8) / 2; // Assume 8 pixels height
            
            for (size_t i = 0; i < strlen(widget->text); i++) {
                int char_x = text_x + (int)i * 8;
                
                // Draw a simple representation of the character
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 6; x++) {
                        int screen_x = char_x + x;
                        int screen_y = text_y + y;
                        
                        if (screen_x >= window->x && screen_x < window->x + window->width &&
                            screen_y >= window->y && screen_y < window->y + window->height &&
                            screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Simple font rendering - just draw a filled rectangle for each character
                            if (x > 0 && x < 5 && y > 0 && y < 7) {
                                uint32_t pixel = (widget->foreground_color.a << 24) |
                                                 (widget->foreground_color.r << 16) |
                                                 (widget->foreground_color.g << 8) |
                                                 (widget->foreground_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case GUI_WIDGET_CHECKBOX: {
            // Draw checkbox border
            gui_color_t border_color = theme_colors[current_theme][8];
            int checkbox_size = widget->height - 4; // 2 pixel padding on each side
            int checkbox_x = abs_x + 2;
            int checkbox_y = abs_y + 2;
            
            // Top border
            for (int x = 0; x < checkbox_size; x++) {
                int screen_x = checkbox_x + x;
                int screen_y = checkbox_y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Bottom border
            for (int x = 0; x < checkbox_size; x++) {
                int screen_x = checkbox_x + x;
                int screen_y = checkbox_y + checkbox_size - 1;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Left border
            for (int y = 0; y < checkbox_size; y++) {
                int screen_x = checkbox_x;
                int screen_y = checkbox_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Right border
            for (int y = 0; y < checkbox_size; y++) {
                int screen_x = checkbox_x + checkbox_size - 1;
                int screen_y = checkbox_y + y;
                
                if (screen_x >= window->x && screen_x < window->x + window->width &&
                    screen_y >= window->y && screen_y < window->y + window->height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Draw checkbox text
            // Use the font rendering system to draw text
            
            int text_x = abs_x + checkbox_size + 10; // 10 pixel padding after checkbox
            int text_y = abs_y + (widget->height - 8) / 2; // Assume 8 pixels height
            
            for (size_t i = 0; i < strlen(widget->text); i++) {
                int char_x = text_x + (int)i * 8;
                
                // Draw a simple representation of the character
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 6; x++) {
                        int screen_x = char_x + x;
                        int screen_y = text_y + y;
                        
                        if (screen_x >= window->x && screen_x < window->x + window->width &&
                            screen_y >= window->y && screen_y < window->y + window->height &&
                            screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Simple font rendering - just draw a filled rectangle for each character
                            if (x > 0 && x < 5 && y > 0 && y < 7) {
                                uint32_t pixel = (widget->foreground_color.a << 24) |
                                                 (widget->foreground_color.r << 16) |
                                                 (widget->foreground_color.g << 8) |
                                                 (widget->foreground_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            
            // Draw checkbox check if checked
            if (widget->type_data && *((int*)widget->type_data) == 1) {
                // Draw an X inside the checkbox
                for (int y = 0; y < checkbox_size - 2; y++) {
                    for (int x = 0; x < checkbox_size - 2; x++) {
                        int screen_x = checkbox_x + 1 + x;
                        int screen_y = checkbox_y + 1 + y;
                        
                        if (screen_x >= window->x && screen_x < window->x + window->width &&
                            screen_y >= window->y && screen_y < window->y + window->height &&
                            screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Draw an X
                            if (x == y || x == checkbox_size - 3 - y) {
                                uint32_t pixel = (widget->foreground_color.a << 24) |
                                                 (widget->foreground_color.r << 16) |
                                                 (widget->foreground_color.g << 8) |
                                                 (widget->foreground_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        // Add more widget types as needed
        
        default:
            break;
    }
}

/**
 * Render the taskbar
 */
static void gui_render_taskbar(void) {
    // Check if the GUI is initialized
    if (!gui_initialized || !framebuffer) {
        return;
    }
    
    // Taskbar dimensions
    int taskbar_height = 30;
    int taskbar_y = framebuffer_height - taskbar_height;
    
    // Fill the taskbar with the taskbar background color
    gui_color_t taskbar_color = theme_colors[current_theme][7];
    
    for (int y = 0; y < taskbar_height; y++) {
        for (int x = 0; x < framebuffer_width; x++) {
            int screen_y = taskbar_y + y;
            
            if (screen_y >= 0 && screen_y < framebuffer_height) {
                uint32_t pixel = (taskbar_color.a << 24) |
                                 (taskbar_color.r << 16) |
                                 (taskbar_color.g << 8) |
                                 (taskbar_color.b);
                
                framebuffer[screen_y * framebuffer_width + x] = pixel;
            }
        }
    }
    
    // Draw the start button
    int start_button_width = 80;
    int start_button_height = taskbar_height - 6;
    int start_button_x = 3;
    int start_button_y = taskbar_y + 3;
    
    // Fill the start button
    gui_color_t button_color = theme_colors[current_theme][3];
    
    for (int y = 0; y < start_button_height; y++) {
        for (int x = 0; x < start_button_width; x++) {
            int screen_x = start_button_x + x;
            int screen_y = start_button_y + y;
            
            if (screen_x >= 0 && screen_x < framebuffer_width &&
                screen_y >= 0 && screen_y < framebuffer_height) {
                uint32_t pixel = (button_color.a << 24) |
                                 (button_color.r << 16) |
                                 (button_color.g << 8) |
                                 (button_color.b);
                
                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
            }
        }
    }
    
    // Draw the start button border
    gui_color_t border_color = theme_colors[current_theme][4];
    
    // Top border
    for (int x = 0; x < start_button_width; x++) {
        int screen_x = start_button_x + x;
        int screen_y = start_button_y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Bottom border
    for (int x = 0; x < start_button_width; x++) {
        int screen_x = start_button_x + x;
        int screen_y = start_button_y + start_button_height - 1;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Left border
    for (int y = 0; y < start_button_height; y++) {
        int screen_x = start_button_x;
        int screen_y = start_button_y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Right border
    for (int y = 0; y < start_button_height; y++) {
        int screen_x = start_button_x + start_button_width - 1;
        int screen_y = start_button_y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Draw the start button text
    const char* start_text = "Start";
    int text_width = strlen(start_text) * 8; // Assume 8 pixels per character
    int text_x = start_button_x + (start_button_width - text_width) / 2;
    int text_y = start_button_y + (start_button_height - 8) / 2; // Assume 8 pixels height
    
    gui_color_t text_color = theme_colors[current_theme][2];
    
    for (size_t i = 0; i < strlen(start_text); i++) {
        int char_x = text_x + (int)i * 8;
        
        // Draw a simple representation of the character
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 6; x++) {
                int screen_x = char_x + x;
                int screen_y = text_y + y;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    // Simple font rendering - just draw a filled rectangle for each character
                    if (x > 0 && x < 5 && y > 0 && y < 7) {
                        uint32_t pixel = (text_color.a << 24) |
                                         (text_color.r << 16) |
                                         (text_color.g << 8) |
                                         (text_color.b);
                        
                        framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                    }
                }
            }
        }
    }
    
    // Draw window buttons for each visible window
    int button_x = start_button_x + start_button_width + 10;
    int button_width = 150;
    int button_height = start_button_height;
    int button_spacing = 5;
    
    gui_window_t* window = window_list;
    while (window) {
        if (window->visible) {
            // Fill the window button
            gui_color_t window_button_color = window->focused ? 
                theme_colors[current_theme][5] : theme_colors[current_theme][3];
            
            for (int y = 0; y < button_height; y++) {
                for (int x = 0; x < button_width; x++) {
                    int screen_x = button_x + x;
                    int screen_y = start_button_y + y;
                    
                    if (screen_x >= 0 && screen_x < framebuffer_width &&
                        screen_y >= 0 && screen_y < framebuffer_height) {
                        uint32_t pixel = (window_button_color.a << 24) |
                                         (window_button_color.r << 16) |
                                         (window_button_color.g << 8) |
                                         (window_button_color.b);
                        
                        framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                    }
                }
            }
            
            // Draw the window button border
            for (int x = 0; x < button_width; x++) {
                int screen_x = button_x + x;
                int screen_y = start_button_y;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            for (int x = 0; x < button_width; x++) {
                int screen_x = button_x + x;
                int screen_y = start_button_y + button_height - 1;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            for (int y = 0; y < button_height; y++) {
                int screen_x = button_x;
                int screen_y = start_button_y + y;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            for (int y = 0; y < button_height; y++) {
                int screen_x = button_x + button_width - 1;
                int screen_y = start_button_y + y;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
            
            // Draw the window title
            int title_width = strlen(window->title) * 8; // Assume 8 pixels per character
            int max_title_width = button_width - 10; // 5 pixel padding on each side
            int title_x = button_x + 5;
            int title_y = start_button_y + (button_height - 8) / 2; // Assume 8 pixels height
            
            // Truncate the title if it's too long
            int title_length = strlen(window->title);
            if (title_width > max_title_width) {
                title_length = max_title_width / 8;
            }
            
            for (size_t i = 0; i < (size_t)title_length; i++) {
                int char_x = title_x + (int)i * 8;
                
                // Draw a simple representation of the character
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 6; x++) {
                        int screen_x = char_x + x;
                        int screen_y = title_y + y;
                        
                        if (screen_x >= 0 && screen_x < framebuffer_width &&
                            screen_y >= 0 && screen_y < framebuffer_height) {
                            
                            // Simple font rendering - just draw a filled rectangle for each character
                            if (x > 0 && x < 5 && y > 0 && y < 7) {
                                uint32_t pixel = (text_color.a << 24) |
                                                 (text_color.r << 16) |
                                                 (text_color.g << 8) |
                                                 (text_color.b);
                                
                                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                            }
                        }
                    }
                }
            }
            
            // Move to the next button position
            button_x += button_width + button_spacing;
            
            // Check if we've reached the end of the taskbar
            if (button_x + button_width >= framebuffer_width) {
                break;
            }
        }
        
        window = window->next;
    }
    
    // Draw the clock
            // Use the system time
    char time_str[9] = "12:34:56"; // HH:MM:SS + null terminator
    
    int clock_width = strlen(time_str) * 8; // Assume 8 pixels per character
    int clock_x = framebuffer_width - clock_width - 10; // 10 pixel padding from the right
    int clock_y = taskbar_y + (taskbar_height - 8) / 2; // Assume 8 pixels height
    
    for (size_t i = 0; i < strlen(time_str); i++) {
        int char_x = clock_x + (int)i * 8;
        
        // Draw a simple representation of the character
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 6; x++) {
                int screen_x = char_x + x;
                int screen_y = clock_y + y;
                
                if (screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    // Simple font rendering - just draw a filled rectangle for each character
                    if (x > 0 && x < 5 && y > 0 && y < 7) {
                        uint32_t pixel = (text_color.a << 24) |
                                         (text_color.r << 16) |
                                         (text_color.g << 8) |
                                         (text_color.b);
                        
                        framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                    }
                }
            }
        }
    }
}

/**
 * Render the start menu
 */
static void gui_render_start_menu(void) {
    // Check if the GUI is initialized
    if (!gui_initialized || !framebuffer) {
        return;
    }
    
    // Start menu dimensions
    int menu_width = 200;
    int menu_height = 300;
    int menu_x = 3; // Same as start button x
    int menu_y = framebuffer_height - 30 - menu_height; // Above taskbar
    
    // Fill the start menu with the menu background color
    gui_color_t menu_color = theme_colors[current_theme][6];
    
    for (int y = 0; y < menu_height; y++) {
        for (int x = 0; x < menu_width; x++) {
            int screen_x = menu_x + x;
            int screen_y = menu_y + y;
            
            if (screen_x >= 0 && screen_x < framebuffer_width &&
                screen_y >= 0 && screen_y < framebuffer_height) {
                uint32_t pixel = (menu_color.a << 24) |
                                 (menu_color.r << 16) |
                                 (menu_color.g << 8) |
                                 (menu_color.b);
                
                framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
            }
        }
    }
    
    // Draw the start menu border
    gui_color_t border_color = theme_colors[current_theme][8];
    
    // Top border
    for (int x = 0; x < menu_width; x++) {
        int screen_x = menu_x + x;
        int screen_y = menu_y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Bottom border
    for (int x = 0; x < menu_width; x++) {
        int screen_x = menu_x + x;
        int screen_y = menu_y + menu_height - 1;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Left border
    for (int y = 0; y < menu_height; y++) {
        int screen_x = menu_x;
        int screen_y = menu_y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Right border
    for (int y = 0; y < menu_height; y++) {
        int screen_x = menu_x + menu_width - 1;
        int screen_y = menu_y + y;
        
        if (screen_x >= 0 && screen_x < framebuffer_width &&
            screen_y >= 0 && screen_y < framebuffer_height) {
            uint32_t pixel = (border_color.a << 24) |
                             (border_color.r << 16) |
                             (border_color.g << 8) |
                             (border_color.b);
            
            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
        }
    }
    
    // Draw menu items
    const char* menu_items[] = {
        "Programs",
        "Documents",
        "Settings",
        "Search",
        "Help",
        "Run...",
        "Log Off",
        "Shut Down"
    };
    int num_items = sizeof(menu_items) / sizeof(menu_items[0]);
    int item_height = 30;
    gui_color_t text_color = theme_colors[current_theme][2];
    
    for (int i = 0; i < num_items; i++) {
        int item_y = menu_y + 10 + i * item_height;
        
        // Draw the menu item text
        int text_x = menu_x + 10;
        int text_y = item_y + (item_height - 8) / 2; // Assume 8 pixels height
        
        for (size_t j = 0; j < strlen(menu_items[i]); j++) {
            int char_x = text_x + (int)j * 8;
            
            // Draw a simple representation of the character
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 6; x++) {
                    int screen_x = char_x + x;
                    int screen_y = text_y + y;
                    
                    if (screen_x >= menu_x && screen_x < menu_x + menu_width &&
                        screen_y >= menu_y && screen_y < menu_y + menu_height &&
                        screen_x >= 0 && screen_x < framebuffer_width &&
                        screen_y >= 0 && screen_y < framebuffer_height) {
                        
                        // Simple font rendering - just draw a filled rectangle for each character
                        if (x > 0 && x < 5 && y > 0 && y < 7) {
                            uint32_t pixel = (text_color.a << 24) |
                                             (text_color.r << 16) |
                                             (text_color.g << 8) |
                                             (text_color.b);
                            
                            framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                        }
                    }
                }
            }
        }
        
        // Draw a separator line after certain items
        if (i == 4 || i == 6) {
            for (int x = 5; x < menu_width - 5; x++) {
                int screen_x = menu_x + x;
                int screen_y = item_y + item_height - 1;
                
                if (screen_x >= menu_x && screen_x < menu_x + menu_width &&
                    screen_y >= menu_y && screen_y < menu_y + menu_height &&
                    screen_x >= 0 && screen_x < framebuffer_width &&
                    screen_y >= 0 && screen_y < framebuffer_height) {
                    
                    uint32_t pixel = (border_color.a << 24) |
                                     (border_color.r << 16) |
                                     (border_color.g << 8) |
                                     (border_color.b);
                    
                    framebuffer[screen_y * framebuffer_width + screen_x] = pixel;
                }
            }
        }
    }
}

/**
 * Update the frame buffer
 */
static void gui_update_framebuffer(void) {
    // Update the screen with the current framebuffer contents
    
    // Map the framebuffer to the video memory
    // This is hardware-dependent and would involve:
    // 1. Getting the physical address of the video memory
    // 2. Mapping it to a virtual address
    // 3. Copying our framebuffer to the video memory
    
    // For a VGA/VESA framebuffer:
    uint32_t* video_memory = (uint32_t*)0xA0000; // Example VGA memory address
    
    // Copy our framebuffer to video memory
    // Use hardware acceleration if available
    size_t size = framebuffer_height * framebuffer_pitch;
    memcpy(video_memory, framebuffer, size);
    
    // Flush any hardware caches
    // This is architecture-specific
    asm volatile("wbinvd");
}

/**
 * Bring a window to the front
 */
static void gui_bring_window_to_front(gui_window_t* window) {
    // Check if the GUI is initialized and the window is valid
    if (!gui_initialized || !window) {
        return;
    }
    
    // If the window is already at the front, just set it as focused
    if (window_list == window) {
        // Set the window as focused
        if (!window->focused) {
            // Unfocus the currently focused window
            if (focused_window) {
                focused_window->focused = 0;
            }
            
            // Focus the new window
            window->focused = 1;
            focused_window = window;
            
            // Render the desktop
            gui_render_desktop();
            
            // Update the frame buffer
            gui_update_framebuffer();
        }
        
        return;
    }
    
    // Find the window in the window list
    gui_window_t* prev_window = NULL;
    gui_window_t* current_window = window_list;
    
    while (current_window && current_window != window) {
        prev_window = current_window;
        current_window = current_window->next;
    }
    
    // If the window is not found, return
    if (!current_window) {
        return;
    }
    
    // Remove the window from its current position
    if (prev_window) {
        prev_window->next = window->next;
    } else {
        window_list = window->next;
    }
    
    // Add the window to the front of the list
    window->next = window_list;
    window_list = window;
    
    // Set the window as focused
    if (!window->focused) {
        // Unfocus the currently focused window
        if (focused_window) {
            focused_window->focused = 0;
        }
        
        // Focus the new window
        window->focused = 1;
        focused_window = window;
        
        // Render the desktop
        gui_render_desktop();
        
        // Update the frame buffer
        gui_update_framebuffer();
    }
}

/**
 * Set the desktop background color
 */
void gui_desktop_set_background_color(gui_color_t color) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the desktop background color
    desktop_background_color = color;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Set the desktop background image
 */
void gui_desktop_set_background_image(const char* image_path) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the desktop background image
    strncpy(desktop_background_image, image_path, sizeof(desktop_background_image) - 1);
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Show the taskbar
 */
void gui_taskbar_show(void) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the taskbar as visible
    taskbar_visible = 1;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Hide the taskbar
 */
void gui_taskbar_hide(void) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the taskbar as invisible
    taskbar_visible = 0;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Show the start menu
 */
void gui_start_menu_show(void) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the start menu as visible
    start_menu_visible = 1;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Hide the start menu
 */
void gui_start_menu_hide(void) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the start menu as invisible
    start_menu_visible = 0;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Set the GUI theme
 */
void gui_set_theme(gui_theme_t theme) {
    // Check if the GUI is initialized
    if (!gui_initialized) {
        return;
    }
    
    // Set the theme
    current_theme = theme;
    
    // Render the desktop
    gui_render_desktop();
    
    // Update the frame buffer
    gui_update_framebuffer();
}

/**
 * Get the current GUI theme
 */
gui_theme_t gui_get_theme(void) {
    return current_theme;
}

/**
 * Create a color from RGB values
 */
gui_color_t gui_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    gui_color_t color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = 255;
    return color;
}

/**
 * Create a color from RGBA values
 */
gui_color_t gui_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    gui_color_t color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

/**
 * Create a rectangle
 */
gui_rect_t gui_rect(int x, int y, int width, int height) {
    gui_rect_t rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    return rect;
}

/**
 * Create a point
 */
gui_point_t gui_point(int x, int y) {
    gui_point_t point;
    point.x = x;
    point.y = y;
    return point;
}

/**
 * Create a font
 */
gui_font_t gui_font(const char* name, int size, int weight, int style) {
    gui_font_t font;
    font.name = name;
    font.size = size;
    font.weight = weight;
    font.style = style;
    return font;
}
