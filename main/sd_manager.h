#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "esp_err.h"
#include "lvgl.h" // For lv_obj_t

/**
 * @brief Initializes the SD card and mounts the filesystem.
 * @return ESP_OK on success, or an esp_err_t error code on failure.
 */
void sd_init(lv_task_t*);

/**
 * @brief Registers the SD card driver with LVGL's filesystem interface.
 * Assigns 'S' as the drive letter.
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
void sd_register_with_lvgl();

/**
 * @brief Reads a text file from the SD card and displays it on an LVGL label.
 *
 * Uses LVGL filesystem functions. Handles file opening, size determination (v7 compatible),
 * reading, and setting the label text.
 *
 * @param label Pointer to the LVGL label object.
 * @param base_filename Name of the file on the SD card (e.g., "text.txt").
 * @param drive_letter The drive letter registered with LVGL (e.g., 'S').
 * @param default_error_text Text to display on the label if an error occurs.
 * @return ESP_OK if successful (even if file is empty), ESP_FAIL on critical errors.
 */
esp_err_t lvgl_display_text_from_sd_file(lv_obj_t *label, const char *base_filename);

#endif // SD_MANAGER_H