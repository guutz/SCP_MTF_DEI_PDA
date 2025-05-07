#include "sd_manager.h"

#include <string>       // For std::string
#include <vector>       // For std::vector
#include <fstream>      // For std::ifstream
#include <cstring>      // For strcmp, strlen etc. (though less needed with std::string)
#include <cstdio>       // For snprintf

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_check.h"

#include "lvgl.h"

static const char *TAG = "sd_manager_cpp";

#define MOUNT_POINT "/sdcard"

#define PIN_NUM_MISO  GPIO_NUM_19
#define PIN_NUM_MOSI  GPIO_NUM_23
#define PIN_NUM_CLK   GPIO_NUM_18
#define PIN_NUM_CS    GPIO_NUM_5

static sdmmc_card_t* s_card = NULL;

esp_err_t sd_init() {
    // Prevent double initialization
    if (s_card != NULL) {
        ESP_LOGW(TAG, "SD card already initialized.");
        return ESP_OK;
    }

    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;

    ESP_LOGI(TAG, "Initializing SD card.");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    ret = spi_bus_initialize(VSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = VSPI_HOST;

    ESP_LOGI(TAG, "Mounting filesystem at %s", MOUNT_POINT);
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem (%s). Check card/connections.", esp_err_to_name(ret));
        spi_bus_free(VSPI_HOST);
        s_card = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "Filesystem mounted successfully.");
    sdmmc_card_print_info(stdout, s_card);

    // No need to create or use SD mutex
    // No need for explicit LVGL registration - it's handled by LV_FS_STDIO

    return ESP_OK;
}
