#include "lvgl.h"
#include "lvgl_helpers.h"
#include "esp_err.h"

esp_err_t display_text_from_file(const char* filename, lv_obj_t* label);

void ui_init(void);
