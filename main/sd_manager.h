#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "esp_err.h"
#include "lvgl.h"
#include "menu_structures.h" // Include menu structures

/**
 * @brief Initializes the SD card and mounts the filesystem.
 */
void sd_init(lv_task_t*);

/**
 * @brief Registers the SD card driver with LVGL's filesystem interface.
 */
void sd_register_with_lvgl();

/**
 * @brief Reads a text file from SD and displays it on an LVGL label.
 */
esp_err_t lvgl_display_text_from_sd_file(lv_obj_t *label, const char *lvgl_path_with_drive);


/**
 * @brief Parses a menu definition text file from the SD card.
 * Populates the G_MenuScreens global map with parsed definitions.
 * @param file_path_on_sd Path to the menu definition file (e.g., "S:/menu.txt").
 * @return true if parsing was successful, false otherwise.
 */
bool parse_menu_definition_file(const char* file_path_on_sd);


#endif // SD_MANAGER_H