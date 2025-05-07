#define LV_CONF_SKIP
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "esp_err.h"

esp_err_t display_text_from_file(const char* filename, lv_obj_t* label);

void ui_init(void);

void esp_log_lvgl_cb(lv_log_level_t level, const char * file, uint32_t line, const char * func, const char * dsc);