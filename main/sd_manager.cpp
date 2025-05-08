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

static SemaphoreHandle_t s_sd_mutex = NULL;
static sdmmc_card_t* s_card = NULL;

static bool build_full_path(const char* filename, char* full_path_buffer, size_t buffer_size) {
    int required_len = snprintf(full_path_buffer, buffer_size, "%s/%s", MOUNT_POINT, filename);
    if (required_len < 0 || static_cast<size_t>(required_len) >= buffer_size) {
        ESP_LOGE(TAG, "Failed to build full path or path too long for buffer.");
        return false;
    }
    return true;
}

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

    s_sd_mutex = xSemaphoreCreateMutex();
    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SD card mutex!");
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
        spi_bus_free(VSPI_HOST);
        s_card = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SD card mutex created.");

    sd_register_with_lvgl();

    return ESP_OK;
}

void sd_uninit() {
    if (s_card == NULL) {
        ESP_LOGI(TAG, "SD card not initialized or already uninitialized.");
        return;
    }

    if (s_sd_mutex != NULL) {
        if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "Unmounting filesystem...");

            esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to unmount filesystem: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "Filesystem unmounted.");
            }
            s_card = NULL;

            ret = spi_bus_free(VSPI_HOST);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "SPI bus freed.");
            }

            xSemaphoreGive(s_sd_mutex); 

            vSemaphoreDelete(s_sd_mutex);
            s_sd_mutex = NULL;
            ESP_LOGI(TAG, "SD card mutex deleted.");

        } else {
            ESP_LOGE(TAG, "Failed to take mutex for uninitialization. Cleanup aborted.");
        }
    } else {
        ESP_LOGW(TAG, "Mutex not found during uninit. Attempting cleanup without lock.");
        esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
         if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to unmount filesystem: %s", esp_err_to_name(ret));
         else ESP_LOGI(TAG, "Filesystem unmounted.");
        s_card = NULL;

        ret = spi_bus_free(VSPI_HOST);
         if (ret != ESP_OK) ESP_LOGE(TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
         else ESP_LOGI(TAG, "SPI bus freed.");
    }
}

/**
 * @brief Reads a text file from the SD card line by line into a std::vector (C++ Implementation).
 */
esp_err_t sd_read_lines(const char* filename, std::vector<std::string>& lines_out) {
    // --- Input validation ---
    if (!filename) {
        ESP_LOGE(TAG, "sd_read_lines: Invalid argument (NULL filename).");
        return ESP_ERR_INVALID_ARG;
    }
    lines_out.clear();

    if (s_sd_mutex == NULL || s_card == NULL) {
        ESP_LOGE(TAG, "SD card not initialized or mutex not created.");
        return ESP_FAIL;
    }

    std::ifstream file_stream;
    esp_err_t result = ESP_FAIL; 

    // --- Take Mutex ---
    if (xSemaphoreTake(s_sd_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Mutex taken, reading lines from: %s", filename);

    // --- Build full path ---
    char full_path[128]; 
    if (!build_full_path(filename, full_path, sizeof(full_path))) {
        xSemaphoreGive(s_sd_mutex); 
        return ESP_ERR_INVALID_ARG; 
    }

    // --- Open file ---
    file_stream.open(full_path);
    if (!file_stream.is_open()) {
        ESP_LOGE(TAG, "Failed to open file: %s", full_path);
        result = ESP_FAIL;
    } else {
        std::string line;
        // --- Read line by line ---
        try {
            while (std::getline(file_stream, line)) {
                // Optional: remove trailing '\r' if dealing with Windows line endings
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                lines_out.push_back(std::move(line));
            }

            if (file_stream.bad()) { 
                 ESP_LOGE(TAG, "Error reading file: %s", full_path);
                 result = ESP_FAIL;
                 lines_out.clear();
            } else if (!file_stream.eof()) { 
                 ESP_LOGE(TAG, "Error reading file: %s", full_path);
                 result = ESP_FAIL;
                 lines_out.clear();
            }
             else {
                result = ESP_OK;
                ESP_LOGI(TAG, "Successfully read %zu lines from %s", lines_out.size(), filename);
            }
        } catch (const std::exception& e) {
             ESP_LOGE(TAG, "Exception during file read: %s", e.what());
             result = ESP_FAIL;
             lines_out.clear();
        } catch (...) {
             ESP_LOGE(TAG, "Unknown exception during file read.");
             result = ESP_FAIL;
             lines_out.clear();
        }

        file_stream.close();
    }

    xSemaphoreGive(s_sd_mutex);

    return result;
}

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
esp_err_t sd_read_file_to_buffer(const char *filename, char *buffer, size_t buffer_size,
                                 const char *default_text)
{
    // Validate inputs
    if (!filename || !buffer || buffer_size == 0)
    {
        if (buffer && buffer_size > 0)
        {
            const char *err_msg = "Error: Invalid parameters";
            strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
        }
        return ESP_ERR_INVALID_ARG;
    }

    // Ensure buffer is always null-terminated
    buffer[buffer_size - 1] = '\0';

    if (s_sd_mutex == NULL || s_card == NULL)
    {
        const char *err_msg = "Error: SD card not initialized";
        strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
        return ESP_FAIL;
    }

    esp_err_t result = ESP_FAIL;

    // Take Mutex
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        const char *err_msg = "Error: Could not access SD card";
        strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
        return ESP_FAIL;
    }

    // Build full path
    char full_path[128];
    if (!build_full_path(filename, full_path, sizeof(full_path)))
    {
        const char *err_msg = "Error: Invalid path";
        strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
        xSemaphoreGive(s_sd_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    // Open file
    std::ifstream file_stream(full_path, std::ios::binary | std::ios::ate); // Open at end for size
    if (!file_stream.is_open())
    {
        ESP_LOGE(TAG, "Failed to open file: %s", full_path);
        const char *err_msg = "Error: File not found";
        strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
        xSemaphoreGive(s_sd_mutex);
        return ESP_FAIL;
    }

    try
    {
        // Get file size
        std::streamsize file_size = file_stream.tellg();
        file_stream.seekg(0, std::ios::beg);

        // Check if buffer is large enough
        if (static_cast<size_t>(file_size) >= buffer_size)
        {
            ESP_LOGW(TAG, "Buffer too small for file: %s (%lld bytes needed, %zu available)",
                     full_path, (long long)file_size, buffer_size);
            file_size = buffer_size - 1; // Truncate to fit buffer (minus null terminator)
        }

        // Read directly into buffer
        if (file_stream.read(buffer, file_size))
        {
            // Null-terminate the string at the end of the read data
            buffer[file_size] = '\0';
            ESP_LOGI(TAG, "Successfully read %lld bytes from %s", (long long)file_size, filename);
            result = ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Error reading file: %s", full_path);
            const char *err_msg = "Error: File read error";
            strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
            result = ESP_FAIL;
        }
    }
    catch (const std::exception &e)
    {
        ESP_LOGE(TAG, "Exception during file read: %s", e.what());
        snprintf(buffer, buffer_size - 1, "%s", default_text ? default_text : e.what());
        result = ESP_FAIL;
    }
    catch (...)
    {
        ESP_LOGE(TAG, "Unknown exception during file read.");
        const char *err_msg = "Error: Unknown read error";
        strncpy(buffer, default_text ? default_text : err_msg, buffer_size - 1);
        result = ESP_FAIL;
    }

    file_stream.close();
    xSemaphoreGive(s_sd_mutex);

    return result;
}

// --- LVGL Filesystem Callback Functions (Using VFS/Standard C) ---
static lv_fs_res_t sd_open_cb(lv_fs_drv_t* drv, void* file_p, const char* path, lv_fs_mode_t mode) {

    char full_path[128];
    if (!build_full_path(path, full_path, sizeof(full_path))) {
        ESP_LOGE(TAG, "Invalid path for LVGL: %s", path);
        return LV_FS_RES_INV_PARAM;
    }

    const char* mode_str;
    if (mode == LV_FS_MODE_WR) mode_str = "wb";
    else if (mode == LV_FS_MODE_RD) mode_str = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) mode_str = "rb+"; // Or "wb+" depending on need
    else return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file open!");
        return LV_FS_RES_HW_ERR; // Use HW_ERR or BUSY
    }

    FILE* f = fopen(full_path, mode_str);
    xSemaphoreGive(s_sd_mutex);

    if (f == NULL) {
        ESP_LOGE(TAG, "LVGL CB: Failed to open file: %s (%s)", full_path, strerror(errno));
        return LV_FS_RES_NOT_EX; // Or map errno
    }

    // Store FILE pointer (file_p is FILE**)
    FILE** fp = (FILE**)file_p;
    *fp = f;

    ESP_LOGI(TAG, "LVGL CB: Opened file: %s", full_path);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_close_cb(lv_fs_drv_t* drv, void* file_p) {
    FILE** fp = (FILE**)file_p;
    if (fp == NULL || *fp == NULL) return LV_FS_RES_INV_PARAM; // Already closed or invalid

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file close!");
         // Should we still try to close? Maybe not.
        return LV_FS_RES_HW_ERR;
    }

    int ret = fclose(*fp);
     *fp = NULL; // Mark as closed regardless of fclose result
    xSemaphoreGive(s_sd_mutex);

    if (ret == 0) {
        ESP_LOGD(TAG, "LVGL CB: Closed file");
        return LV_FS_RES_OK;
    } else {
        ESP_LOGE(TAG, "LVGL CB: Failed to close file (errno %d)", errno);
        return LV_FS_RES_UNKNOWN; // Or map errno
    }
}

static lv_fs_res_t sd_read_cb(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    FILE** fp = (FILE**)file_p;
    if (fp == NULL || *fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file read!");
        *br = 0;
        return LV_FS_RES_HW_ERR;
    }

    *br = fread(buf, 1, btr, *fp);
    // fread returns number of items successfully read. Error checking is done via ferror/feof
    bool error_occurred = ferror(*fp);
    xSemaphoreGive(s_sd_mutex);

    // ESP_LOGD(TAG, "LVGL CB: Read attempt btr=%u, br=%u", btr, *br); // Can be noisy

    if(error_occurred) {
        ESP_LOGE(TAG, "LVGL CB: fread error occurred (errno %d)", errno);
        // *br might be unreliable on error, potentially clear it?
        // *br = 0;
        return LV_FS_RES_UNKNOWN;
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_write_cb(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw) {
     FILE** fp = (FILE**)file_p;
    if (fp == NULL || *fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file write!");
        *bw = 0;
        return LV_FS_RES_HW_ERR;
    }

    *bw = fwrite(buf, 1, btw, *fp);
    bool error_occurred = ferror(*fp); // Check error after write
    xSemaphoreGive(s_sd_mutex);

    ESP_LOGD(TAG, "LVGL CB: Write attempt btw=%u, bw=%u", btw, *bw);

     if(error_occurred) {
        ESP_LOGE(TAG, "LVGL CB: fwrite error occurred (errno %d)", errno);
        // *bw might be unreliable on error
        return LV_FS_RES_UNKNOWN;
    }

    return LV_FS_RES_OK;
}


static lv_fs_res_t sd_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos) {
    FILE** fp = (FILE**)file_p;
    if (fp == NULL || *fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file seek!");
        return LV_FS_RES_HW_ERR; // Use HW_ERR or BUSY
    }

    // LVGL v7.11 seek_cb only supports seeking from the beginning (SEEK_SET)
    int result = fseek(*fp, (long)pos, SEEK_SET); // Cast pos to long for fseek

    xSemaphoreGive(s_sd_mutex);

    ESP_LOGD(TAG, "sd_seek_cb (v7): pos=%u, result=%d", pos, result);

    return (result == 0) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}


static lv_fs_res_t sd_tell_cb(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    FILE** fp = (FILE**)file_p;
    if (fp == NULL || *fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SD card mutex for file tell!");
         *pos_p = (uint32_t)-1; // Indicate error
        return LV_FS_RES_HW_ERR;
    }

    long pos = ftell(*fp);
    xSemaphoreGive(s_sd_mutex);

    if (pos < 0) {
         ESP_LOGE(TAG, "LVGL CB: ftell error occurred (errno %d)", errno);
        *pos_p = (uint32_t)-1; // Indicate error
        return LV_FS_RES_UNKNOWN;
    }

    *pos_p = (uint32_t)pos; // Cast assuming file size fits in 32 bits
    // ESP_LOGD(TAG, "LVGL CB: Tell pos=%u", *pos_p); // Can be noisy
    return LV_FS_RES_OK;
}

/**
 * @brief Register the SD card with LVGL's filesystem interface.
 *
 * Call this after sd_init() to enable LVGL to load images/files directly
 * from the SD card using the VFS layer.
 * Files can then be loaded using path "S:/filename.png" in LVGL functions.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_register_with_lvgl() {
    if (s_card == NULL || s_sd_mutex == NULL) {
        ESP_LOGE(TAG, "SD card not initialized. Call sd_init() first.");
        return ESP_FAIL;
    }

        lv_fs_drv_t fs_drv;
        lv_fs_drv_init(&fs_drv);   // Initialize fields to NULL/0

        // Set driver letter
        fs_drv.letter = 'S'; // Assign 'S' for SD card

        // Set callbacks (using the VFS wrappers)
        fs_drv.open_cb = sd_open_cb;
        fs_drv.close_cb = sd_close_cb;
        fs_drv.read_cb = sd_read_cb;
        fs_drv.write_cb = sd_write_cb;
        fs_drv.seek_cb = sd_seek_cb;
        fs_drv.tell_cb = sd_tell_cb;

        // Directory functions (set to NULL if not implemented)
        fs_drv.dir_open_cb = NULL;
        fs_drv.dir_read_cb = NULL;
        fs_drv.dir_close_cb = NULL;

        // Register driver
        if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to take SD card mutex for LVGL registration!");
            return ESP_FAIL;
        }
        lv_fs_drv_register(&fs_drv);
        xSemaphoreGive(s_sd_mutex);
        ESP_LOGI(TAG, "SD card VFS driver registered with LVGL as drive 'S:'");
        return ESP_OK;
}