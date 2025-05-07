#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <vector>
#include <string>
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


/**
 * @brief Initializes the SD card and mounts the filesystem.
 *
 * Initializes SPI bus, SD card, mounts FAT filesystem, and creates mutex.
 * Call once at startup.
 *
 * @return ESP_OK on success, or an esp_err_t error code on failure.
 */
esp_err_t sd_init();

/**
 * @brief Uninitializes the SD card, unmounts the filesystem, and frees resources.
 *
 * Unmounts FAT, frees SPI bus, deletes mutex. Ensure no tasks are accessing
 * the SD card when calling.
 */
void sd_uninit();

/**
 * @brief Reads a text file from the SD card line by line into a std::vector.
 *
 * Reads the specified file, splitting it into lines. Uses C++ streams and
 * std::vector for automatic memory management.
 *
 * @param filename The name of the file to read (relative to the mount point).
 * @param[out] lines_out A std::vector<std::string> to store the lines read from the file.
 * The vector will be cleared before reading.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_ARG if filename is NULL or invalid path generated.
 * @return ESP_FAIL if the SD card is not initialized, mutex cannot be taken,
 * the file cannot be opened, or a read error occurs.
 * @return Other esp_err_t codes for underlying filesystem or stream errors.
 */
esp_err_t sd_read_lines(const char* filename, std::vector<std::string>& lines_out);

/**
 * @brief Reads a text file into a provided buffer for display purposes.
 * 
 * Designed specifically for direct use with LVGL labels or other UI elements.
 * Always provides a valid C-string in the buffer - either the file content or an error message.
 *
 * @param filename The name of the file to read (relative to the mount point).
 * @param buffer Pointer to a buffer where the content will be stored.
 * @param buffer_size Size of the provided buffer.
 * @param default_text Optional text to return if file can't be read (defaults to error message).
 * @return ESP_OK if file was read successfully, appropriate error code otherwise.
 */
esp_err_t sd_read_file_to_buffer(const char* filename, char* buffer, size_t buffer_size, 
    const char* default_text = nullptr);


/**
 * @brief Register the SD card VFS driver with LVGL's filesystem interface.
 *
 * Call this after sd_init() to enable LVGL to load images/files directly
 * from the SD card using the VFS layer via custom callbacks.
 * Requires LV_USE_FS_IF to be enabled in lv_conf.h.
 * Files can then be loaded using path "S:/filename.png" (or other assigned letter)
 * in LVGL functions.
 *
 * @return ESP_OK on success, ESP_FAIL if LV_USE_FS_IF is disabled or SD not initialized.
 */
esp_err_t sd_register_with_lvgl();


#endif // SD_MANAGER_H
