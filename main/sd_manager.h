#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "esp_err.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "menu_structures.h" // Include menu structures

#define MOUNT_POINT "/sdcard"

// Expose the mutex for SD operations
extern SemaphoreHandle_t s_sd_mutex;

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
 * 
 * @note This function loads the entire file into memory, so it's
 * only suitable for small files. For large files, use lvgl_stream_text_from_sd_file instead.
 */
esp_err_t lvgl_display_text_from_sd_file(lv_obj_t *label, const char *lvgl_path_with_drive);

/**
 * @brief Sets up streaming for a large text file from SD to an LVGL label.
 * 
 * @param label The LVGL label to display text in
 * @param page The LVGL page containing the label (for scroll events)
 * @param lvgl_path_with_drive Path to the file (e.g., "S:/path/to/file.txt")
 * @return ESP_OK if successful, error code otherwise
 * 
 * @note This function sets up a TextStreamer that will load text in chunks
 * as the user scrolls through the content. It's suitable for large files
 * that would not fit in memory all at once.
 */
esp_err_t lvgl_stream_text_from_sd_file(lv_obj_t *label, lv_obj_t *page, const char *lvgl_path_with_drive);

/**
 * @brief Parses a menu definition text file from the SD card.
 * Populates the G_MenuScreens global map with parsed definitions.
 * @param file_path_on_sd Path to the menu definition file (e.g., "S:/menu.txt").
 * @return true if parsing was successful, false otherwise.
 */
bool parse_menu_definition_file(const char* file_path_on_sd);


#endif // SD_MANAGER_H