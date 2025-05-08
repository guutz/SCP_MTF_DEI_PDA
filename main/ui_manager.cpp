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
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // lv_obj_set_hidden(label, true);

    // lv_obj_t *scp_logo = lv_img_create(page, NULL);
    // lv_img_set_src(scp_logo, "S:/DEI/SCP_Foundation.bin");
    // lv_obj_align(scp_logo, NULL, LV_ALIGN_CENTER, 0, 0);

    // vTaskDelay(pdMS_TO_TICKS(3000));
    // lv_obj_del(scp_logo);

    // lv_obj_set_hidden(label, false);
    // lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    // lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    // lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);  // Wrap long lines
    // lv_obj_set_width(label, lv_page_get_width_fit(page));
    // display_text_from_file("/DEI/content.md", label);
    // lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
}