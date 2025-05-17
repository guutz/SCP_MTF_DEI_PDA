#include "sd_manager.h"
#include "menu_structures.h" // For MenuScreenDefinition, G_MenuScreens etc.
#include "sd_raw_access.h" // Include the raw SD card access functions

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream> // For parsing lines

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG_SD = "sd_manager";

// Define SPI bus pins (ensure these are correct for your hardware)
#define PIN_NUM_MISO  GPIO_NUM_19 
#define PIN_NUM_MOSI  GPIO_NUM_23 
#define PIN_NUM_CLK   GPIO_NUM_18 
#define PIN_NUM_CS    GPIO_NUM_5  

// Externalize for use by sd_raw_access functions
SemaphoreHandle_t s_sd_mutex = NULL;
static sdmmc_card_t* s_card = NULL;
static bool s_lvgl_fs_registered = false;

// LVGL Filesystem Callback Prototypes
static lv_fs_res_t sd_open_cb(lv_fs_drv_t* drv, void* file_p_arg, const char* path, lv_fs_mode_t mode);
static lv_fs_res_t sd_close_cb(lv_fs_drv_t* drv, void* file_p_arg);
static lv_fs_res_t sd_read_cb(lv_fs_drv_t* drv, void* file_p_arg, void* buf, uint32_t btr, uint32_t* br);
static lv_fs_res_t sd_write_cb(lv_fs_drv_t* drv, void* file_p_arg, const void* buf, uint32_t btw, uint32_t* bw);
static lv_fs_res_t sd_seek_cb(lv_fs_drv_t* drv, void* file_p_arg, uint32_t pos);
static lv_fs_res_t sd_tell_cb(lv_fs_drv_t* drv, void* file_p_arg, uint32_t* pos_p);

static void provision_github_token_from_sd(void) {
    const char* token_filename = "DEI/github_token.txt";

    FILE* token_fp = sd_raw_fopen(token_filename, "r");
    if (token_fp) {
        char token[128] = {0};
        size_t len = sd_raw_fread(token, 1, sizeof(token) - 1, token_fp);
        sd_raw_fclose(token_fp);
        // Remove trailing newline if present
        if (len > 0) {
            int i = (int)len - 1;
            while (i >= 0 && (token[i] == '\n' || token[i] == '\r')) {
                token[i] = '\0';
                --i;
            }
        }
        if (strlen(token) >= 10) {
            nvs_handle_t nvs_handle;
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                nvs_flash_erase();
                nvs_flash_init();
            }
            err = nvs_open("ota", NVS_READWRITE, &nvs_handle);
            if (err == ESP_OK) {
                nvs_set_str(nvs_handle, "gh_token", token);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
                ESP_LOGI(TAG_SD, "GitHub token loaded from SD and stored in NVS.");
                // Delete the file after storing
                sd_raw_remove(token_filename);
                ESP_LOGI(TAG_SD, "github_token.txt deleted from SD after provisioning.");
            } else {
                ESP_LOGE(TAG_SD, "Failed to open NVS for writing GitHub token.");
            }
        } else {
            ESP_LOGW(TAG_SD, "GitHub token file found but token is too short or invalid.");
        }
    } else {
        ESP_LOGI(TAG_SD, "No github_token.txt found on SD, skipping token provisioning.");
    }
}

void sd_init(lv_task_t* task) {
    (void)task; 
    ESP_LOGI(TAG_SD, "Initializing SD card...");

    if (s_card != NULL) {
        ESP_LOGW(TAG_SD, "SD card already initialized.");
        if (!s_lvgl_fs_registered) { 
            sd_register_with_lvgl();
        }
        return;
    }

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST; 

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4096,
    };

    ret = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot), &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG_SD, "SPI bus (host %d) already initialized.", host.slot);
        } else {
            ESP_LOGE(TAG_SD, "Failed to initialize SPI bus (host %d): %s", host.slot, esp_err_to_name(ret));
            return;
        }
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to mount filesystem: %s", esp_err_to_name(ret));
        s_card = NULL;
        return;
    }
    ESP_LOGI(TAG_SD, "SD card mounted at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);

    if (s_sd_mutex == NULL) {
        s_sd_mutex = xSemaphoreCreateMutex();
        if (s_sd_mutex == NULL) {
            ESP_LOGE(TAG_SD, "Failed to create SD card mutex");
            esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
            s_card = NULL;
            return;
        }
    }
    
    // Register with LVGL
    sd_register_with_lvgl();
    
    provision_github_token_from_sd();
}

void sd_register_with_lvgl() {
    if (s_card == NULL) {
        ESP_LOGE(TAG_SD, "SD card not properly initialized. Cannot register with LVGL.");
        return;
    }
    if (s_lvgl_fs_registered) {
        ESP_LOGI(TAG_SD, "SD card VFS driver already registered with LVGL.");
        return;
    }

    static lv_fs_drv_t fs_drv; 
    lv_fs_drv_init(&fs_drv);

    fs_drv.letter = 'S'; 
    fs_drv.open_cb = sd_open_cb;
    fs_drv.close_cb = sd_close_cb;
    fs_drv.read_cb = sd_read_cb;
    fs_drv.write_cb = sd_write_cb;
    fs_drv.seek_cb = sd_seek_cb;
    fs_drv.tell_cb = sd_tell_cb;
    fs_drv.dir_open_cb = NULL; 
    fs_drv.dir_read_cb = NULL;
    fs_drv.dir_close_cb = NULL;

    lv_fs_drv_register(&fs_drv);
    s_lvgl_fs_registered = true;
    ESP_LOGI(TAG_SD, "SD card VFS driver registered with LVGL as drive 'S:'");
}

// LVGL Filesystem Callbacks (implementations as before)
static lv_fs_res_t sd_open_cb(lv_fs_drv_t* drv, void* file_p_arg, const char* path, lv_fs_mode_t mode) {
    (void)drv; 
    const char* mode_str;
    if (mode == LV_FS_MODE_WR) mode_str = "wb";
    else if (mode == LV_FS_MODE_RD) mode_str = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) mode_str = "r+b"; 
    else return LV_FS_RES_INV_PARAM;

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR; 
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_open_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }

    char full_sd_path[LV_FS_MAX_PATH_LENGTH]; 
    snprintf(full_sd_path, sizeof(full_sd_path), "%s/%s", MOUNT_POINT, path);
    ESP_LOGD(TAG_SD, "sd_open_cb: LVGL path '%s', Full path '%s', Mode '%s'", path, full_sd_path, mode_str);

    FILE* f = fopen(full_sd_path, mode_str);
    xSemaphoreGive(s_sd_mutex);

    if (f == NULL) {
        ESP_LOGE(TAG_SD, "Failed to open file: %s (errno %d: %s)", full_sd_path, errno, strerror(errno));
        return LV_FS_RES_NOT_EX; 
    }
    FILE** fp_ptr = (FILE**)file_p_arg; 
    *fp_ptr = f;

    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_close_cb(lv_fs_drv_t* drv, void* file_p_arg) {
    (void)drv; 
    FILE* fp = (FILE*)file_p_arg; 
    if (fp == NULL) return LV_FS_RES_OK; 

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_close_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    int ret = fclose(fp);
    xSemaphoreGive(s_sd_mutex);
    
    return (ret == 0) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t sd_read_cb(lv_fs_drv_t* drv, void* file_p_arg, void* buf, uint32_t btr, uint32_t* br) {
    (void)drv; 
    FILE* fp = (FILE*)file_p_arg;
    *br = 0;
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_read_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    *br = fread(buf, 1, btr, fp);
    bool error_occurred = ferror(fp);
    xSemaphoreGive(s_sd_mutex);

    if (error_occurred) {
        ESP_LOGE(TAG_SD, "sd_read_cb: fread error (errno %d: %s)", errno, strerror(errno));
        return LV_FS_RES_FS_ERR;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_write_cb(lv_fs_drv_t* drv, void* file_p_arg, const void* buf, uint32_t btw, uint32_t* bw) {
    (void)drv; 
    FILE* fp = (FILE*)file_p_arg;
    *bw = 0;
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_write_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    *bw = fwrite(buf, 1, btw, fp);
    bool error_occurred = ferror(fp);
    xSemaphoreGive(s_sd_mutex);
    
    if (error_occurred) {
        ESP_LOGE(TAG_SD, "sd_write_cb: fwrite error (errno %d: %s)", errno, strerror(errno));
        return LV_FS_RES_FS_ERR;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_seek_cb(lv_fs_drv_t* drv, void* file_p_arg, uint32_t pos) {
    (void)drv; 
    FILE* fp = (FILE*)file_p_arg;
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_seek_cb: mutex timeout");
        return LV_FS_RES_HW_ERR;
    }
    int result = fseek(fp, (long)pos, SEEK_SET); 
    xSemaphoreGive(s_sd_mutex);

    return (result == 0) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t sd_tell_cb(lv_fs_drv_t* drv, void* file_p_arg, uint32_t* pos_p) {
    (void)drv; 
    FILE* fp = (FILE*)file_p_arg;
    *pos_p = (uint32_t)-1L; 
    if (fp == NULL) return LV_FS_RES_INV_PARAM;

    if (s_sd_mutex == NULL) return LV_FS_RES_HW_ERR;
    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG_SD, "sd_tell_cb: mutex timeout");
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


#define TEXT_DISPLAY_BUFFER_SIZE 1024
esp_err_t lvgl_display_text_from_sd_file(lv_obj_t *label, const char *lvgl_path_with_drive) {
    static char text_display_buffer[TEXT_DISPLAY_BUFFER_SIZE]; 
    const char *error_msg = "Error: File load failed.";

    if (!label || !lvgl_path_with_drive) return ESP_ERR_INVALID_ARG;
    text_display_buffer[0] = '\0'; 

    if (s_sd_mutex == NULL || s_card == NULL || !s_lvgl_fs_registered) {
        ESP_LOGE(TAG_SD, "SD card not ready for lvgl_display_text.");
        if (label) lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_SD, "Loading text for label from: %s", lvgl_path_with_drive);

    lv_fs_file_t file_lv;
    lv_fs_res_t fs_res = lv_fs_open(&file_lv, lvgl_path_with_drive, LV_FS_MODE_RD); 
    if (fs_res != LV_FS_RES_OK) {
        ESP_LOGE(TAG_SD, "LVGL fs_open failed for '%s': %d", lvgl_path_with_drive, fs_res);
        if (label) lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    uint32_t bytes_read_this_iteration = 0;
    fs_res = lv_fs_read(&file_lv, text_display_buffer, TEXT_DISPLAY_BUFFER_SIZE -1, &bytes_read_this_iteration);
    
    if (fs_res == LV_FS_RES_OK && bytes_read_this_iteration > 0) {
        text_display_buffer[bytes_read_this_iteration] = '\0';
        if (label) lv_label_set_text(label, text_display_buffer);
        if (bytes_read_this_iteration == TEXT_DISPLAY_BUFFER_SIZE -1) {
             ESP_LOGW(TAG_SD, "File '%s' might be truncated as it filled the display buffer.", lvgl_path_with_drive);
        }
    } else if (fs_res != LV_FS_RES_OK) {
         ESP_LOGE(TAG_SD, "LVGL fs_read error for '%s': res %d", lvgl_path_with_drive, fs_res);
        if (label) lv_label_set_text(label, error_msg);
        lv_fs_close(&file_lv); 
        return ESP_FAIL;
    } else { 
        ESP_LOGI(TAG_SD, "File '%s' is empty or read 0 bytes.", lvgl_path_with_drive);
        if (label) lv_label_set_text(label, ""); 
    }

    lv_fs_close(&file_lv); 
    return ESP_OK;
}

std::string trim_string(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; 
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

#define MENU_FILE_BUFFER_SIZE 8192
bool parse_menu_definition_file(const char* file_path_on_sd) {
    ESP_LOGI(TAG_SD, "Attempting to parse menu definition file: %s", file_path_on_sd);
    if (s_sd_mutex == NULL || s_card == NULL || !s_lvgl_fs_registered) {
        ESP_LOGE(TAG_SD, "SD card not initialized or LVGL FS not registered. Cannot parse menu file.");
        return false;
    }
    G_MenuScreens.clear(); 

    lv_fs_file_t file_lv;
    lv_fs_res_t fs_res = lv_fs_open(&file_lv, file_path_on_sd, LV_FS_MODE_RD);
    if (fs_res != LV_FS_RES_OK) {
        ESP_LOGE(TAG_SD, "Failed to open menu file '%s' with LVGL. Error: %d", file_path_on_sd, fs_res);
        return false;
    }
    
    uint32_t menu_file_size = 0;
    FILE* raw_fp = (FILE*)file_lv.file_d; 
    if (raw_fp == NULL) {
        ESP_LOGE(TAG_SD, "Could not get raw FILE* for menu file '%s'", file_path_on_sd);
        lv_fs_close(&file_lv);
        return false;
    }

    if (xSemaphoreTake(s_sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (fseek(raw_fp, 0, SEEK_END) == 0) {
            long size_long = ftell(raw_fp);
            if (size_long != -1L) {
                menu_file_size = (uint32_t)size_long;
                fseek(raw_fp, 0, SEEK_SET); 
            } else {
                ESP_LOGE(TAG_SD, "ftell failed for menu file '%s'", file_path_on_sd);
                menu_file_size = (uint32_t)-1; 
            }
        } else {
            ESP_LOGE(TAG_SD, "fseek to SEEK_END failed for menu file '%s'", file_path_on_sd);
            menu_file_size = (uint32_t)-1; 
        }
        xSemaphoreGive(s_sd_mutex);
    } else {
        ESP_LOGE(TAG_SD, "Mutex timeout while getting size of menu file '%s'", file_path_on_sd);
        lv_fs_close(&file_lv);
        return false;
    }
    
    if (menu_file_size == (uint32_t)-1) { 
        lv_fs_close(&file_lv);
        return false;
    }
    
    if (menu_file_size == 0) {
        ESP_LOGW(TAG_SD, "Menu file '%s' is empty.", file_path_on_sd);
        lv_fs_close(&file_lv);
        return true; 
    }

    if (menu_file_size >= MENU_FILE_BUFFER_SIZE) {
        ESP_LOGE(TAG_SD, "Menu file too large (%" PRIu32 " bytes) for buffer (%" PRIu32 " bytes).", menu_file_size, (uint32_t)MENU_FILE_BUFFER_SIZE);
        lv_fs_close(&file_lv);
        return false;
    }

    static char menu_content_buffer[MENU_FILE_BUFFER_SIZE]; 
    uint32_t bytes_read_total = 0;
    fs_res = lv_fs_read(&file_lv, menu_content_buffer, menu_file_size, &bytes_read_total);
    if (fs_res != LV_FS_RES_OK || bytes_read_total != menu_file_size) {
        ESP_LOGE(TAG_SD, "Failed to read entire menu file. Read %" PRIu32 "/%" PRIu32 ". LVGL res: %d",
                 bytes_read_total, menu_file_size, fs_res);
        lv_fs_close(&file_lv);
        return false;
    }
    lv_fs_close(&file_lv);
    menu_content_buffer[bytes_read_total] = '\0'; 

    ESP_LOGD(TAG_SD, "Read %" PRIu32 " bytes from menu file for parsing.", bytes_read_total);
    std::string file_content_str(menu_content_buffer);

    std::stringstream ss(file_content_str);
    std::string line;
    MenuScreenDefinition current_screen_def;
    bool in_screen_def = false;

    while (std::getline(ss, line, '\n')) {
        line = trim_string(line);
        if (line.empty() || line[0] == '#') continue; 

        ESP_LOGV(TAG_SD, "Parsing line: %s", line.c_str());

        if (line.rfind("MENU:", 0) == 0 || line.rfind("SCREEN:", 0) == 0) {
            if (in_screen_def && !current_screen_def.name.empty()) {
                G_MenuScreens[current_screen_def.name] = current_screen_def;
                ESP_LOGD(TAG_SD, "Stored menu/screen definition: %s", current_screen_def.name.c_str());
            }
            current_screen_def = MenuScreenDefinition(); 
            
            size_t colon_pos = line.find(':');
            std::string name_part = trim_string(line.substr(colon_pos + 1));
            size_t title_marker_pos = name_part.find("TITLE:");
            
            if (title_marker_pos != std::string::npos) {
                current_screen_def.name = trim_string(name_part.substr(0, title_marker_pos));
                current_screen_def.title = trim_string(name_part.substr(title_marker_pos + 6)); 
            } else {
                 current_screen_def.name = name_part;
                 current_screen_def.title = current_screen_def.name; 
            }
            current_screen_def.defined_parent_name = ""; 
            in_screen_def = true;
            ESP_LOGV(TAG_SD, "Started new screen. Name: '%s', Title: '%s'", current_screen_def.name.c_str(), current_screen_def.title.c_str());

        } else if (line.rfind("ENDMENU", 0) == 0 || line.rfind("ENDSCREEN", 0) == 0) {
            if (in_screen_def && !current_screen_def.name.empty()) {
                G_MenuScreens[current_screen_def.name] = current_screen_def;
                ESP_LOGD(TAG_SD, "Stored menu/screen definition (on END): %s", current_screen_def.name.c_str());
                in_screen_def = false;
            }
        } else if (in_screen_def && line.rfind("PARENT_MENU:", 0) == 0) {
            current_screen_def.defined_parent_name = trim_string(line.substr(12)); 
            ESP_LOGV(TAG_SD, "Screen '%s' has defined parent: '%s'", current_screen_def.name.c_str(), current_screen_def.defined_parent_name.c_str());
        }
        else if (in_screen_def && line.rfind("BUTTON:", 0) == 0) {
            MenuItemDefinition item;
            item.render_type = RENDER_AS_BUTTON; // This is a button
            item.action = ACTION_NONE; // Default action

            std::string btn_content = line.substr(7); 
            std::vector<std::string> parts;
            std::stringstream btn_ss(btn_content);
            std::string part_str;
            
            std::string label_part, action_type_part, action_target_part_full;
            
            if (!std::getline(btn_ss, label_part, ':')) { ESP_LOGW(TAG_SD, "Invalid BUTTON format (no label): %s", line.c_str()); continue; }
            item.text_to_display = trim_string(label_part);

            if (!std::getline(btn_ss, action_type_part, ':')) { ESP_LOGW(TAG_SD, "Invalid BUTTON format (no action type for label '%s'): %s", label_part.c_str(), line.c_str()); continue; }
            std::string action_type_str_parsed = trim_string(action_type_part);
            
            if (std::getline(btn_ss, action_target_part_full)) { 
                 item.action_target = trim_string(action_target_part_full);
            } else {
                 item.action_target = ""; // Ensure it's initialized
            }

            if (action_type_str_parsed == "SUBMENU") item.action = ACTION_NAVIGATE_SUBMENU;
            else if (action_type_str_parsed == "TEXTCONTENT") item.action = ACTION_DISPLAY_TEXT_CONTENT_SCREEN;
            else if (action_type_str_parsed == "TEXTFILE") item.action = ACTION_DISPLAY_TEXT_FILE_SCREEN;
            else if (action_type_str_parsed == "FUNC") item.action = ACTION_EXECUTE_PREDEFINED_FUNCTION;
            else if (action_type_str_parsed == "BACK") item.action = ACTION_GO_BACK;
            else {
                ESP_LOGW(TAG_SD, "Unknown button action type: '%s' for button '%s'", action_type_str_parsed.c_str(), item.text_to_display.c_str());
                continue; // Skip this invalid button
            }
            current_screen_def.items.push_back(item);
            ESP_LOGV(TAG_SD, "Added BUTTON: '%s', Action: %d, Target: '%s'", item.text_to_display.c_str(), item.action, item.action_target.c_str());

        } else if (in_screen_def && line.rfind("TEXT:", 0) == 0) { 
             MenuItemDefinition item;
             item.text_to_display = trim_string(line.substr(6)); 
             item.render_type = RENDER_AS_STATIC_LABEL; // This is text content
             current_screen_def.items.push_back(item);
             ESP_LOGV(TAG_SD, "Added TEXT: '%s'", item.text_to_display.c_str());
        }
    }

    if (in_screen_def && !current_screen_def.name.empty()) {
        G_MenuScreens[current_screen_def.name] = current_screen_def;
        ESP_LOGD(TAG_SD, "Stored final menu/screen definition: %s", current_screen_def.name.c_str());
    }
    
    ESP_LOGI(TAG_SD, "Finished parsing menu file. Loaded %zu screen definitions.", G_MenuScreens.size());
    return !G_MenuScreens.empty() || menu_file_size == 0; 
}
