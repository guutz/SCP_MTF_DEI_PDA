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
    
    // Open the file using LVGL's file system
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, filename, LV_FS_MODE_RD);
    
    if (res != LV_FS_RES_OK) {
        ESP_LOGE("TEXT_LOADER", "Failed to open file: %s (error: %d)", filename, res);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    // Get file size by seeking to the end and getting position
    res = lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    if (res != LV_FS_RES_OK) {
        ESP_LOGE("TEXT_LOADER", "Failed to seek to end: %s (error: %d)", filename, res);
        lv_fs_close(&file);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    uint32_t file_size;
    res = lv_fs_tell(&file, &file_size);
    if (res != LV_FS_RES_OK) {
        ESP_LOGE("TEXT_LOADER", "Failed to get position: %s (error: %d)", filename, res);
        lv_fs_close(&file);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    // Seek back to the beginning
    res = lv_fs_seek(&file, 0, LV_FS_SEEK_SET);
    if (res != LV_FS_RES_OK) {
        ESP_LOGE("TEXT_LOADER", "Failed to seek to beginning: %s (error: %d)", filename, res);
        lv_fs_close(&file);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    // Check if buffer is large enough
    uint32_t read_size = file_size;
    if (file_size >= TEXT_BUFFER_SIZE) {
        ESP_LOGW("TEXT_LOADER", "File too large, truncating: %s (%u bytes)", filename, file_size);
        read_size = TEXT_BUFFER_SIZE - 1; // Leave space for null terminator
    }
    
    // Read the file content
    uint32_t bytes_read;
    res = lv_fs_read(&file, text_buffer, read_size, &bytes_read);
    
    if (res != LV_FS_RES_OK || bytes_read != read_size) {
        ESP_LOGE("TEXT_LOADER", "Failed to read file: %s (error: %d, read: %u/%u)", 
                 filename, res, bytes_read, read_size);
        lv_fs_close(&file);
        lv_label_set_text(label, error_msg);
        return ESP_FAIL;
    }
    
    // Close the file
    lv_fs_close(&file);
    
    // Null-terminate the buffer
    text_buffer[bytes_read] = '\0';
    
    // Set the label text
    lv_label_set_text(label, text_buffer);
    
    ESP_LOGI("TEXT_LOADER", "Successfully loaded text from %s (%u bytes)", filename, bytes_read);
    return ESP_OK;
}


void ui_init(void) {

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *page = lv_obj_create(scr);
    lv_obj_set_size(page, 320, 240);
    lv_obj_align(page, LV_ALIGN_CENTER, 0, 0);  
    lv_obj_set_style_bg_color(page, lv_color_hex(0x000000), 0);
    
    // Add scrollbar to page
    lv_obj_set_style_pad_all(page, 0, 0); 
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
    
    lv_obj_t *label = lv_label_create(page);
    lv_obj_set_style_text_font(label, &lv_font_firacode_16, 0); 
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); 
    
    lv_obj_set_width(label, lv_obj_get_width(page) - 20);  // Account for scrollbar
    lv_label_set_text(label, "Loading...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); 
    vTaskDelay(pdMS_TO_TICKS(1000));
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);  

    lv_obj_t *scp_logo = lv_img_create(page);
    lv_img_set_src(scp_logo, "S:DEI/SCP_Foundation.bin");
    lv_obj_align(scp_logo, LV_ALIGN_CENTER, 0, 0);  

    vTaskDelay(pdMS_TO_TICKS(3000));
    lv_obj_del(scp_logo);

    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);  
    lv_obj_set_style_text_font(label, &lv_font_firacode_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_obj_get_width(page) - 20);
    display_text_from_file("S:DEI/content.md", label);  
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);  
}
