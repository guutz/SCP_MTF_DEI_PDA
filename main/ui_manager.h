#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"
#include "menu_structures.h" // For MenuScreenDefinition, G_MenuScreens etc.

extern volatile bool g_wifi_intentional_stop;

// Define a function pointer type for functions that create screens.
// These creator functions will typically use global state (like G_TargetMenuNameForCreation)
// set prior to their invocation.
typedef lv_obj_t* (*screen_create_func_t)(void);

// --- Global Style Declarations ---
// These styles are initialized by ui_styles_init() and can be used by UI elements.
extern lv_style_t style_default_screen_bg;
extern lv_style_t style_default_label;
extern lv_style_t style_default_button;

// --- Public Function Declarations ---

/**
 * @brief Initializes the UI styles and prepares the initial UI state.
 * This should be called once at startup. It also triggers parsing of menu definitions
 * and sets up the splash screen.
 * @param task Pointer to the LVGL task (can be NULL if not used by the function,
 * as is the case in the current implementation).
 */
void ui_init(lv_task_t *task);

/**
 * @brief Initializes the global default styles for UI elements.
 * Called by ui_init().
 */
void ui_styles_init(void);

/**
 * @brief Loads the screen specified by G_TargetMenuNameForCreation using the provided
 * screen_creator_func.
 * * The G_TargetMenuNameForCreation and G_InvokingParentMenuName global variables
 * should be set appropriately before calling this function.
 *
 * @param anim_type The type of animation to use for the screen transition.
 * @param anim_time The duration of the animation in milliseconds.
 * @param anim_delay The delay before the animation starts in milliseconds.
 * @param auto_delete_old If true, the old screen will be deleted after the animation.
 * @param screen_creator_func A function pointer (screen_create_func_t) that will be
 * called to create the new screen object. This function
 * is expected to use G_TargetMenuNameForCreation and
 * G_InvokingParentMenuName to determine which screen to create
 * and what its context is.
 */
void ui_load_active_target_screen(lv_scr_load_anim_t anim_type,
                                  uint32_t anim_time,
                                  uint32_t anim_delay,
                                  bool auto_delete_old,
                                  screen_create_func_t screen_creator_func);


void example_predefined_function_1(void);
void example_predefined_function_2(void);

/**
 * @brief Trigger OTA update from the menu
 * This function is registered with G_PredefinedFunctions to allow
 * the OTA update to be triggered from a menu button
 */
void trigger_ota_update_from_menu(void);

/**
 * @brief Toggle WiFi on or off from the menu
 * This function is registered with G_PredefinedFunctions to allow
 * WiFi to be toggled from a menu button
 */
void toggle_wifi_from_menu(void);

/**
 * @brief Display WiFi status information
 * Shows current WiFi connection status, IP address, and signal strength
 */
void show_wifi_status_from_menu(void);

#endif // UI_MANAGER_H
