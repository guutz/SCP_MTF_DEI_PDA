#ifndef SD_RAW_ACCESS_H
#define SD_RAW_ACCESS_H

#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opens a file on the SD card.
 * 
 * @param path_suffix Path to the file relative to the SD card mount point (e.g., "data/myfile.txt").
 * @param mode File access mode (e.g., "r", "w", "rb", "wb").
 * @return FILE* pointer on success, NULL on failure.
 */
FILE* sd_raw_fopen(const char* path_suffix, const char* mode);

/**
 * @brief Reads data from an opened file.
 * 
 * @param ptr Pointer to the buffer to store read data.
 * @param size Size of each element to read.
 * @param count Number of elements to read.
 * @param stream FILE pointer to the opened file.
 * @return Number of elements successfully read.
 */
size_t sd_raw_fread(void* ptr, size_t size, size_t count, FILE* stream);

/**
 * @brief Writes data to an opened file.
 * 
 * @param ptr Pointer to the data to be written.
 * @param size Size of each element to write.
 * @param count Number of elements to write.
 * @param stream FILE pointer to the opened file.
 * @return Number of elements successfully written.
 */
size_t sd_raw_fwrite(const void* ptr, size_t size, size_t count, FILE* stream);

/**
 * @brief Seeks to a specific position in an opened file.
 * 
 * @param stream FILE pointer to the opened file.
 * @param offset Offset from the position specified by 'whence'.
 * @param whence Position from where offset is added (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return 0 on success, non-zero on failure.
 */
int sd_raw_fseek(FILE* stream, long offset, int whence);

/**
 * @brief Gets the current position in an opened file.
 * 
 * @param stream FILE pointer to the opened file.
 * @return Current file position, or -1L on error.
 */
long sd_raw_ftell(FILE* stream);

/**
 * @brief Closes an opened file.
 * 
 * @param stream FILE pointer to the opened file.
 * @return 0 on success, EOF on failure.
 */
int sd_raw_fclose(FILE* stream);

/**
 * @brief Deletes a file from the SD card.
 * 
 * @param path_suffix Path to the file relative to the SD card mount point.
 * @return 0 on success, non-zero on failure.
 */
int sd_raw_remove(const char* path_suffix);

/**
 * @brief Renames or moves a file on the SD card.
 * 
 * @param old_path_suffix Current path to the file relative to the mount point.
 * @param new_path_suffix New path for the file relative to the mount point.
 * @return 0 on success, non-zero on failure.
 */
int sd_raw_rename(const char* old_path_suffix, const char* new_path_suffix);

/**
 * @brief Checks if a file exists on the SD card.
 * 
 * @param path_suffix Path to the file relative to the SD card mount point.
 * @return true if the file exists, false otherwise.
 */
bool sd_raw_file_exists(const char* path_suffix);

/**
 * @brief Gets the size of a file on the SD card.
 * 
 * @param path_suffix Path to the file relative to the SD card mount point.
 * @return File size in bytes, or -1L on error.
 */
long sd_raw_get_file_size(const char* path_suffix);

#ifdef __cplusplus
}
#endif

#endif // SD_RAW_ACCESS_H
