#ifndef SETUP_H
#define SETUP_H

#include "mcp23008.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h"

#include "xasin/mqtt/Handler.h"

#define LV_TICK_PERIOD_MS 1

#define WIFI_STATION_SSID "Dabney Lounge"
#define WIFI_STATION_PASSWD "dabneyvictory"


// --- Terminal Style Definitions ---

// Font
#define TERMINAL_FONT &lv_font_firacode_14
LV_FONT_DECLARE(lv_font_firacode_14);

// Colors
#define TERMINAL_COLOR_BACKGROUND lv_color_hex(0x000000)      // Black background
#define TERMINAL_COLOR_FOREGROUND lv_color_hex(0x00FF00)      // Green text (classic terminal)
#define TERMINAL_COLOR_FOREGROUND_ALT lv_color_hex(0xFFFFFF)  // White text (alternative if green is too much)
#define TERMINAL_COLOR_BUTTON_TEXT TERMINAL_COLOR_FOREGROUND // Green text for buttons
#define TERMINAL_COLOR_BUTTON_BORDER TERMINAL_COLOR_BACKGROUND
#define TERMINAL_COLOR_BUTTON_PRESSED_BG lv_color_hex(0x222222)    // Dark grey for pressed button background
#define TERMINAL_COLOR_BUTTON_PRESSED_TEXT lv_color_hex(0x88FF88) // Lighter green for pressed button text
#define TERMINAL_COLOR_BUTTON_PRESSED_BORDER TERMINAL_COLOR_BACKGROUND

// Spacing & Padding (for a tighter feel)
#define TERMINAL_PADDING_HORIZONTAL 5            // Reduced from 20
#define TERMINAL_PADDING_VERTICAL_TITLE_TOP 5    // Reduced from 10
#define TERMINAL_PADDING_VERTICAL_AFTER_TITLE 8  // Reduced from 15
#define TERMINAL_ITEM_SPACING 2                  // Reduced from 5
#define TERMINAL_BUTTON_HEIGHT 16                // Reduced from 30, adjust based on font size
#define TERMINAL_LABEL_LINE_SPACE 0              // Reduced from 2 (no extra line space)
#define TERMINAL_BUTTON_RADIUS 0                 // Square corners for buttons
#define TERMINAL_BUTTON_BORDER_WIDTH 0           // Add a 1px border to buttons
#define TERMINAL_BUTTON_INNER_PADDING 0          // Small padding inside the button border
#define TERMINAL_BUTTON_BG_OPA LV_OPA_TRANSP     // Transparent background for buttons (border will define shape)

// --- Screen Transition Animations ---
#define SCREEN_ANIMATION_TYPE_FORWARD LV_SCR_LOAD_ANIM_NONE
#define SCREEN_ANIMATION_TYPE_BACKWARD LV_SCR_LOAD_ANIM_NONE
#define SCREEN_ANIMATION_DURATION 0

// --- Global Style Objects ---
extern lv_style_t style_default_screen_bg;
extern lv_style_t style_default_label; // General purpose label style (e.g., for titles, static text)
extern lv_style_t style_default_button;
extern lv_style_t style_transparent_container; // For label containers
extern lv_style_t style_focused_button; // Style for focused buttons

extern SemaphoreHandle_t xGuiSemaphore;

#ifdef __cplusplus
extern "C" {
#endif


void display_touch_init(void);
void time_overlay_init(void);
void lvgl_full_init(void);
void lvgl_tick_timer_setup(void);

/**
 * @brief Initializes the global default styles for UI elements.
 * Called by ui_init().
 */
void ui_styles_init(void);

#ifdef __cplusplus
}
#endif

#endif // SETUP_H
