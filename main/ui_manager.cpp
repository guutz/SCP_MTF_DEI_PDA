#include "lvgl.h"
#include "lvgl_helpers.h"
#include "sd_manager.h"
#include "ui_manager.h"
#include "esp_log.h"
#include <cinttypes>

LV_FONT_DECLARE(lv_font_firacode_16);

#define TEXT_BUFFER_SIZE 4096

esp_err_t display_text_from_file(const char* filename, lv_obj_t* label) {
    // Buffer for text content
    static char text_buffer[TEXT_BUFFER_SIZE];
    
    // Custom error message
    const char* error_msg = "Could not load text file";
    
    // Read file into buffer
    esp_err_t result = sd_read_file_to_buffer(
        filename, 
        text_buffer, 
        TEXT_BUFFER_SIZE, 
        error_msg
    );
    
    // Set the label text
    lv_label_set_text(label, text_buffer);

    return result;
}

void ui_init(void) {
    // lv_log_register_print_cb(esp_log_lvgl_cb);
    LV_LOG_TRACE("UI Init");
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_local_bg_color(scr, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));

    lv_obj_t *page = lv_page_create(scr, NULL);
    lv_obj_set_size(page, 320, 240);
    lv_obj_align(page, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_bg_color(page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x000000));
    
    lv_obj_t *label = lv_label_create(page, NULL);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);  // Wrap long lines
    lv_obj_set_width(label, lv_page_get_width_fit(page));
    lv_label_set_text(label, "Loading...");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    lv_obj_set_hidden(label, true);

    lv_obj_t *scp_logo = lv_img_create(page, NULL);
    lv_img_set_src(scp_logo, "S:/DEI/SCP_Foundation.bin");
    lv_obj_align(scp_logo, NULL, LV_ALIGN_CENTER, 0, 0);

    vTaskDelay(pdMS_TO_TICKS(3000));
    lv_obj_del(scp_logo);

    lv_obj_set_hidden(label, false);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);  // Wrap long lines
    lv_obj_set_width(label, lv_page_get_width_fit(page));
    display_text_from_file("/DEI/content.md", label);
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
}

/**
 * @brief LVGL log callback function that prints using ESP_LOGx macros.
 *
 * Matches the lv_log_print_g_cb_t signature.
 * Maps LVGL log levels to ESP-IDF log levels.
 *
 * @param level LVGL log level.
 * @param file Source file path where the log occurred.
 * @param line Line number in the source file.
 * @param func Function name where the log occurred.
 * @param dsc The log message description.
 */
void esp_log_lvgl_cb(lv_log_level_t level, const char * file, uint32_t line, const char * func, const char * dsc)
{
    // Map LVGL log level to ESP log level and call the appropriate macro
    // NOTE: ESP_LOGV is often filtered out by default in menuconfig.
    //       ESP_LOGD requires LVGL log level >= INFO to show up if default ESP log level is INFO.
    switch(level) {
        case LV_LOG_LEVEL_ERROR:
            // Format: E (timestamp) LVGL: error message (file:line @ func)
            ESP_LOGE("LVGL", "%s (%s:%" PRIu32 " @ %s)", dsc, file, line, func);
            break;
        case LV_LOG_LEVEL_WARN:
            // Format: W (timestamp) LVGL: warning message (file:line @ func)
            ESP_LOGW("LVGL", "%s (%s:%" PRIu32 " @ %s)", dsc, file, line, func);
            break;
        case LV_LOG_LEVEL_INFO:
            // Format: I (timestamp) LVGL: info message
            ESP_LOGI("LVGL", "%s", dsc); // Keep info simpler, file/line often not needed
            break;
        case LV_LOG_LEVEL_TRACE: // LVGL TRACE often maps well to ESP DEBUG
            // Format: D (timestamp) LVGL: trace message (file:line @ func)
            ESP_LOGI("LVGL", "%s (%s:%" PRIu32 " @ %s)", dsc, file, line, func);
            break;
        case LV_LOG_LEVEL_USER: // Map LVGL USER to ESP INFO or DEBUG
             // Format: I (timestamp) LVGL: user message
            ESP_LOGI("LVGL", "[USER] %s", dsc);
            break;
        case LV_LOG_LEVEL_NONE: // Fallthrough intentional
        default:
            // Don't print anything for LV_LOG_LEVEL_NONE or unknown levels
            break;
    }
}