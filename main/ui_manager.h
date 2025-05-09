#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h" // For lv_obj_t, lv_scr_load_anim_t, lv_task_t

// Define a function pointer type for functions that create screens
typedef lv_obj_t* (*screen_create_func_t)(void);

/**
 * @brief Initializes the UI, starting with the splash screen.
 * @param task Pointer to the LVGL task (can be NULL if not used by the function).
 */
void ui_init(lv_task_t *task);

/**
 * @brief Transitions to a new screen using a specified animation.
 *
 * This function will call the provided screen_creator function to build the new screen
 * and then use lv_scr_load_anim to display it, automatically deleting the old screen.
 *
 * @param target_screen_creator Pointer to the function that creates the target screen.
 * @param anim_type The type of screen load animation (e.g., LV_SCR_LOAD_ANIM_MOVE_LEFT).
 * @param anim_time Duration of the animation in milliseconds.
 * @param anim_delay Delay before the animation starts in milliseconds.
 * @param auto_delete_old Typically true, to delete the previous screen.
 */
void ui_load_screen_animated(screen_create_func_t target_screen_creator,
                             lv_scr_load_anim_t anim_type,
                             uint32_t anim_time,
                             uint32_t anim_delay,
                             bool auto_delete_old);

// You might also want to declare your individual screen creation functions here
// if they need to be called from event handlers that are defined outside ui_manager.cpp,
// or keep them static if ui_load_screen_animated is always called from within ui_manager.cpp.
// For now, let's assume they can be static in ui_manager.cpp.

#endif // UI_MANAGER_H