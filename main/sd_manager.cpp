#include "sd_manager.h"

#include <cstdio>   // For snprintf, FILE ops
#include <cerrno>   // For errno
#include <cstring>  // For strerror

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "sd_manager";

#define MOUNT_POINT "/sdcard"

// Define SPI bus pins
#define PIN_NUM_MISO  GPIO_NUM_19
#define PIN_NUM_MOSI  GPIO_NUM_23
#define PIN_NUM_CLK   GPIO_NUM_18
#define PIN_NUM_CS    GPIO_NUM_5

static SemaphoreHandle_t s_sd_mutex = NULL;
static sdmmc_card_t* s_card = NULL;

// LVGL Filesystem Callback Prototypes
static lv_fs_res_t sd_open_cb(lv_fs_drv_t* drv, void* file_p, const char* path, lv_fs_mode_t mode);
static lv_fs_res_t sd_close_cb(lv_fs_drv_t* drv, void* file_p);
static lv_fs_res_t sd_read_cb(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br);
static lv_fs_res_t sd_write_cb(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw);
static lv_fs_res_t sd_seek_cb(lv_fs_drv_t* drv, void* file_p, uint32_t pos);
static lv_fs_res_t sd_tell_cb(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p);

void sd_init(lv_task_t*) {
    ESP_LOGI(TAG, "Initializing SD card...");

    if (s_card != NULL) {
        ESP_LOGW(TAG, "SD card already initialized.");
        return;
    }

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST; // ESP32 specific

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
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = VSPI_HOST;

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem: %s", esp_err_to_name(ret));
        spi_bus_free(VSPI_HOST);
        s_card = NULL;
        return;
    }
    ESP_LOGI(TAG, "SD card mounted at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);

    s_sd_mutex = xSemaphoreCreateMutex();
    if (s_sd_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SD card mutex");
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
        spi_bus_free(VSPI_HOST);
        s_card = NULL;
        return;
    }
    
    // Automatically register with LVGL after successful init
    return sd_register_with_lvgl();
}

// --- LVGL Filesystem Callbacks (using VFS/Standard C) ---

static lv_fs_res_t sd_open_cb(lv_fs_drv_t* drv, void* file_p, const char* path, lv_fs_mode_t mode) {
    (void)drv; // Unused
    const char* mode_str;
    if (mode == LV_FS_MODE_WR) mode_str = "wb";
    else if (mode == LV_FS_MODE_RD) mode_str = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) mode_str = "rb+";
    else return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_open_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }

    // Construct full path relative to MOUNT_POINT
    char full_sd_path[LV_FS_MAX_PATH_LENGTH];
    snprintf(full_sd_path, sizeof(full_sd_path), "%s/%s", MOUNT_POINT, path);

    FILE* f = fopen(full_sd_path, mode_str);
    xSemaphoreGive(s_sd_mutex);

    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s (errno %d: %s)", full_sd_path, errno, strerror(errno));
        return LV_FS_RES_NOT_EX;
    }

    // Store FILE pointer. file_p is a pointer to lv_fs_file_t.file_d (void**)
    FILE** fp_ptr = (FILE**)file_p;
    *fp_ptr = f;

    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_close_cb(lv_fs_drv_t* drv, void* file_p) {
    (void)drv; // Unused
    FILE* fp = (FILE*)file_p; // file_p is the FILE* (lv_fs_file_t.file_d)
    if (fp == NULL) return LV_FS_RES_OK; // Closing a NULL file is fine

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_close_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    int ret = fclose(fp);
    xSemaphoreGive(s_sd_mutex);

    return (ret == 0) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t sd_read_cb(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    (void)drv; // Unused
    FILE* fp = (FILE*)file_p; // file_p is the FILE* (lv_fs_file_t.file_d)
    *br = 0;
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_read_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    *br = fread(buf, 1, btr, fp);
    bool error_occurred = ferror(fp);
    xSemaphoreGive(s_sd_mutex);

    if (error_occurred) {
        ESP_LOGE(TAG, "sd_read_cb: fread error (errno %d)", errno);
        return LV_FS_RES_FS_ERR;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_write_cb(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw) {
    (void)drv; // Unused
    FILE* fp = (FILE*)file_p; // file_p is the FILE* (lv_fs_file_t.file_d)
    *bw = 0;
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_write_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    *bw = fwrite(buf, 1, btw, fp);
    bool error_occurred = ferror(fp);
    xSemaphoreGive(s_sd_mutex);
    
    if (error_occurred) {
        ESP_LOGE(TAG, "sd_write_cb: fwrite error (errno %d)", errno);
        return LV_FS_RES_FS_ERR;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_seek_cb(lv_fs_drv_t* drv, void* file_p, uint32_t pos) {
    (void)drv; // Unused
    FILE* fp = (FILE*)file_p; // file_p is the FILE* (lv_fs_file_t.file_d)
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_seek_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    // LVGL v7 seek_cb expects absolute position from beginning (SEEK_SET)
    int result = fseek(fp, (long)pos, SEEK_SET);
    xSemaphoreGive(s_sd_mutex);

    return (result == 0) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t sd_tell_cb(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    (void)drv; // Unused
    FILE* fp = (FILE*)file_p; // file_p is the FILE* (lv_fs_file_t.file_d)
    *pos_p = (uint32_t)-1; // Indicate error by default
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "sd_tell_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    long pos = ftell(fp);
    xSemaphoreGive(s_sd_mutex);

    if (pos < 0) {
        return LV_FS_RES_FS_ERR;
    }
    *pos_p = (uint32_t)pos;
    return LV_FS_RES_OK;
}

void sd_register_with_lvgl() {
    if (s_card == NULL || s_sd_mutex == NULL) {
        ESP_LOGE(TAG, "SD card not initialized. Call sd_init() first.");
        return;
    }

    static lv_fs_drv_t fs_drv; // Make it static or global
    lv_fs_drv_init(&fs_drv);

    fs_drv.letter = 'S'; 
    fs_drv.open_cb = sd_open_cb;
    fs_drv.close_cb = sd_close_cb;
    fs_drv.read_cb = sd_read_cb;
    fs_drv.write_cb = sd_write_cb;
    fs_drv.seek_cb = sd_seek_cb;
    fs_drv.tell_cb = sd_tell_cb;

    // Directory functions (optional, set to NULL if not implemented)
    fs_drv.dir_open_cb = NULL;
    fs_drv.dir_read_cb = NULL;
    fs_drv.dir_close_cb = NULL;

    lv_fs_drv_register(&fs_drv);
    ESP_LOGI(TAG, "SD card VFS driver registered with LVGL as drive 'S:'");
}

// Buffer size for reading file content for LVGL label
#define LVGL_TEXT_BUFFER_SIZE (1024 * 16) // 16 KB 

esp_err_t lvgl_display_text_from_sd_file(lv_obj_t *label, const char *lvgl_path) {
    static char text_buffer[LVGL_TEXT_BUFFER_SIZE]; 
    const char *error_msg = "Error: File load failed.";

    if (!label || !lvgl_path) return ESP_ERR_INVALID_ARG;

    text_buffer[0] = '\0'; // Ensure buffer is initially empty

    if (s_sd_mutex == NULL || s_card == NULL) {
        ESP_LOGE(TAG, "SD card not ready for lvgl_display_text.");
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }

    esp_err_t overall_status = ESP_FAIL;
    lv_fs_file_t file_lv; // LVGL file object
    lv_fs_res_t fs_res;
    uint32_t file_size = 0;

    // 1. Open file using LVGL API (this will use sd_open_cb)
    fs_res = lv_fs_open(&file_lv, lvgl_path, LV_FS_MODE_RD); 
    if (fs_res != LV_FS_RES_OK) {
        ESP_LOGE(TAG, "LVGL fs_open failed for '%s': %d", lvgl_path, fs_res);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }

    // 2. Get file size using the underlying FILE* for v7 compatibility
    // Access the driver-specific file data, which is our FILE*
    FILE* raw_fp = (FILE*)file_lv.file_d; // *** CORRECTED MEMBER NAME HERE ***
    if (raw_fp == NULL) { 
        ESP_LOGE(TAG, "Raw file pointer is NULL after LVGL open for '%s'", lvgl_path);
        lv_label_set_text(label, error_msg);
        lv_fs_close(&file_lv);
        return ESP_FAIL;
    }

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        long current_raw_pos = ftell(raw_fp);
        if (current_raw_pos != -1L && fseek(raw_fp, 0, SEEK_END) == 0) {
            long size_long = ftell(raw_fp);
            if (size_long != -1L) {
                file_size = (uint32_t)size_long;
                fseek(raw_fp, 0, SEEK_SET); // Seek to beginning for LVGL full read
            } else { 
                ESP_LOGE(TAG, "ftell (size) failed for '%s'", lvgl_path);
                if(current_raw_pos != -1L) fseek(raw_fp, current_raw_pos, SEEK_SET); 
                file_size = (uint32_t)-1; 
            }
        } else { 
            ESP_LOGE(TAG, "fseek/ftell for size failed for '%s'", lvgl_path);
             if(current_raw_pos != -1L) fseek(raw_fp, current_raw_pos, SEEK_SET); 
            file_size = (uint32_t)-1; 
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG, "Mutex timeout for file size check of '%s'", lvgl_path);
        lv_label_set_text(label, error_msg);
        lv_fs_close(&file_lv);
        return ESP_FAIL;
    }
    
    if (file_size == (uint32_t)-1) { 
        lv_label_set_text(label, error_msg);
        lv_fs_close(&file_lv);
        return ESP_FAIL;
    }

    // 3. Read file content using LVGL API
    uint32_t bytes_to_read = file_size;
    if (file_size >= LVGL_TEXT_BUFFER_SIZE) {
        ESP_LOGW(TAG, "File '%s' (%u B) too large for buffer (%d B), truncating.",
                 lvgl_path, (unsigned int)file_size, LVGL_TEXT_BUFFER_SIZE);
        bytes_to_read = LVGL_TEXT_BUFFER_SIZE - 1;
    }

    uint32_t bytes_read_lv = 0;
    if (bytes_to_read > 0) {
        fs_res = lv_fs_read(&file_lv, text_buffer, bytes_to_read, &bytes_read_lv);
        if (fs_res == LV_FS_RES_OK && bytes_read_lv == bytes_to_read) {
            text_buffer[bytes_read_lv] = '\0';
            lv_label_set_text(label, text_buffer);
            overall_status = ESP_OK;
        } else {
            ESP_LOGE(TAG, "LVGL fs_read failed for '%s': res %d, read %u/%u",
                     lvgl_path, fs_res, (unsigned int)bytes_read_lv, (unsigned int)bytes_to_read);
            lv_label_set_text(label, error_msg);
        }
    } else if (file_size == 0) { 
        lv_label_set_text(label, ""); 
        overall_status = ESP_OK;
    } else { 
        lv_label_set_text(label, error_msg);
    }

    // 4. Close file using LVGL API
    lv_fs_close(&file_lv); 

    return overall_status;
}