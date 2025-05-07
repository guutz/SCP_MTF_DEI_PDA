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

#endif