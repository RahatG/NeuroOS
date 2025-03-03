/**
 * gui.h - Graphical User Interface for NeuroOS
 * 
 * This file defines the interface for the GUI subsystem, which provides
 * a Linux/macOS-like graphical user interface for NeuroOS.
 */

#ifndef NEUROOS_GUI_H
#define NEUROOS_GUI_H

#include <stdint.h>
#include <stddef.h>

// Color definitions
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} gui_color_t;

// Rectangle structure
typedef struct {
    int x;
    int y;
    int width;
    int height;
} gui_rect_t;

// Point structure
typedef struct {
    int x;
    int y;
} gui_point_t;

// Font structure
typedef struct {
    const char* name;
    int size;
    int weight;
    int style;
} gui_font_t;

// Window structure
typedef struct gui_window gui_window_t;

// Widget structure
typedef struct gui_widget gui_widget_t;

// Event types
typedef enum {
    GUI_EVENT_MOUSE_MOVE,
    GUI_EVENT_MOUSE_DOWN,
    GUI_EVENT_MOUSE_UP,
    GUI_EVENT_KEY_DOWN,
    GUI_EVENT_KEY_UP,
    GUI_EVENT_WINDOW_CLOSE,
    GUI_EVENT_WINDOW_RESIZE,
    GUI_EVENT_WINDOW_MOVE,
    GUI_EVENT_WINDOW_FOCUS,
    GUI_EVENT_WINDOW_BLUR
} gui_event_type_t;

// Event structure
typedef struct {
    gui_event_type_t type;
    gui_window_t* window;
    gui_widget_t* widget;
    union {
        struct {
            int x;
            int y;
            int button;
        } mouse;
        struct {
            int key_code;
            int modifiers;
        } key;
        struct {
            int width;
            int height;
        } size;
        struct {
            int x;
            int y;
        } position;
    } data;
} gui_event_t;

// Event callback function
typedef void (*gui_event_callback_t)(gui_event_t* event, void* user_data);

// Window flags
typedef enum {
    GUI_WINDOW_FLAG_RESIZABLE = 1 << 0,
    GUI_WINDOW_FLAG_BORDERLESS = 1 << 1,
    GUI_WINDOW_FLAG_FULLSCREEN = 1 << 2,
    GUI_WINDOW_FLAG_ALWAYS_ON_TOP = 1 << 3,
    GUI_WINDOW_FLAG_TRANSPARENT = 1 << 4
} gui_window_flags_t;

// Widget types
typedef enum {
    GUI_WIDGET_BUTTON,
    GUI_WIDGET_LABEL,
    GUI_WIDGET_TEXTBOX,
    GUI_WIDGET_CHECKBOX,
    GUI_WIDGET_RADIO,
    GUI_WIDGET_SLIDER,
    GUI_WIDGET_PROGRESS,
    GUI_WIDGET_COMBOBOX,
    GUI_WIDGET_LISTBOX,
    GUI_WIDGET_MENU,
    GUI_WIDGET_TOOLBAR,
    GUI_WIDGET_STATUSBAR,
    GUI_WIDGET_TABCONTROL,
    GUI_WIDGET_TREEVIEW,
    GUI_WIDGET_CUSTOM
} gui_widget_type_t;

// GUI initialization and shutdown
int gui_init(void);
int gui_shutdown(void);

// Window management
gui_window_t* gui_window_create(const char* title, int x, int y, int width, int height, uint32_t flags);
void gui_window_destroy(gui_window_t* window);
void gui_window_show(gui_window_t* window);
void gui_window_hide(gui_window_t* window);
void gui_window_set_title(gui_window_t* window, const char* title);
void gui_window_set_position(gui_window_t* window, int x, int y);
void gui_window_set_size(gui_window_t* window, int width, int height);
void gui_window_set_min_size(gui_window_t* window, int width, int height);
void gui_window_set_max_size(gui_window_t* window, int width, int height);
void gui_window_set_background_color(gui_window_t* window, gui_color_t color);
void gui_window_set_event_callback(gui_window_t* window, gui_event_callback_t callback, void* user_data);
void gui_window_invalidate(gui_window_t* window);
void gui_window_update(gui_window_t* window);

// Widget management
gui_widget_t* gui_widget_create(gui_window_t* window, gui_widget_type_t type, const char* text, int x, int y, int width, int height);
void gui_widget_destroy(gui_widget_t* widget);
void gui_widget_set_text(gui_widget_t* widget, const char* text);
void gui_widget_set_position(gui_widget_t* widget, int x, int y);
void gui_widget_set_size(gui_widget_t* widget, int width, int height);
void gui_widget_set_visible(gui_widget_t* widget, int visible);
void gui_widget_set_enabled(gui_widget_t* widget, int enabled);
void gui_widget_set_font(gui_widget_t* widget, gui_font_t font);
void gui_widget_set_background_color(gui_widget_t* widget, gui_color_t color);
void gui_widget_set_foreground_color(gui_widget_t* widget, gui_color_t color);
void gui_widget_set_event_callback(gui_widget_t* widget, gui_event_callback_t callback, void* user_data);

// Drawing functions
void gui_draw_line(gui_window_t* window, int x1, int y1, int x2, int y2, gui_color_t color);
void gui_draw_rectangle(gui_window_t* window, gui_rect_t rect, gui_color_t color);
void gui_fill_rectangle(gui_window_t* window, gui_rect_t rect, gui_color_t color);
void gui_draw_ellipse(gui_window_t* window, int x, int y, int width, int height, gui_color_t color);
void gui_fill_ellipse(gui_window_t* window, int x, int y, int width, int height, gui_color_t color);
void gui_draw_text(gui_window_t* window, const char* text, int x, int y, gui_font_t font, gui_color_t color);
void gui_draw_image(gui_window_t* window, const void* image_data, int x, int y, int width, int height);

// Event handling
void gui_process_events(void);
void gui_wait_events(void);
void gui_post_event(gui_event_t* event);

// Utility functions
gui_color_t gui_color_rgb(uint8_t r, uint8_t g, uint8_t b);
gui_color_t gui_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
gui_rect_t gui_rect(int x, int y, int width, int height);
gui_point_t gui_point(int x, int y);
gui_font_t gui_font(const char* name, int size, int weight, int style);

// Desktop environment functions
void gui_desktop_set_background_color(gui_color_t color);
void gui_desktop_set_background_image(const char* image_path);
void gui_taskbar_show(void);
void gui_taskbar_hide(void);
void gui_start_menu_show(void);
void gui_start_menu_hide(void);
void gui_notification_show(const char* title, const char* message, int timeout_ms);

// Theme management
typedef enum {
    GUI_THEME_LIGHT,
    GUI_THEME_DARK,
    GUI_THEME_CUSTOM
} gui_theme_t;

void gui_set_theme(gui_theme_t theme);
gui_theme_t gui_get_theme(void);
void gui_load_theme_from_file(const char* theme_file);
void gui_save_theme_to_file(const char* theme_file);

#endif // NEUROOS_GUI_H
