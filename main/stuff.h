#ifndef STUFF_H
#define STUFF_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h" // Needed for lv_font_t and lv_color_t

// Declare the fonts you intend to use
// LV_FONT_DECLARE(lv_font_firacode_8);
// LV_FONT_DECLARE(lv_font_firacode_16); // Keep if you might switch back or use it elsewhere
LV_FONT_DECLARE(lv_font_firacode_12);

// --- Terminal Style Definitions ---

// Font
#define TERMINAL_FONT &lv_font_firacode_12

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
#define TERMINAL_ITEM_SPACING 3                  // Reduced from 5
#define TERMINAL_BUTTON_HEIGHT 12                // Reduced from 30, adjust based on font size
#define TERMINAL_LABEL_LINE_SPACE 1              // Reduced from 2 (no extra line space)
#define TERMINAL_BUTTON_RADIUS 0                 // Square corners for buttons
#define TERMINAL_BUTTON_BORDER_WIDTH 0           // Add a 1px border to buttons
#define TERMINAL_BUTTON_INNER_PADDING 0          // Small padding inside the button border
#define TERMINAL_BUTTON_BG_OPA LV_OPA_TRANSP     // Transparent background for buttons (border will define shape)

extern SemaphoreHandle_t xGuiSemaphore;

#endif // STUFF_H