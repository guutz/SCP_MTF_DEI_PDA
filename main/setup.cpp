#include "setup.h"
#include "lvgl_helpers.h"
#include "lvgl.h"
#include <assert.h>
#include <stdlib.h> 
#include <stdio.h>
#include <time.h>
#include "esp_sntp.h"

// --- Global Style Objects ---
lv_style_t style_default_screen_bg;
lv_style_t style_default_label; // General purpose label style (e.g., for titles, static text)
lv_style_t style_default_button;
lv_style_t style_transparent_container; // For label containers
lv_style_t style_focused_button; // Style for focused buttons

static void lv_tick_task(void *arg);
static lv_obj_t *time_display_label = NULL;

static void get_formatted_current_time(char *buf, size_t buf_len, const char *format) {
    if (!buf || buf_len == 0) return;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo); // Thread-safe version of localtime
    strftime(buf, buf_len, format, &timeinfo);
}

static void time_update_lv_task(lv_task_t *task) {
    if (time_display_label) {
        char time_buf[64];
        get_formatted_current_time(time_buf, sizeof(time_buf), "%H:%M:%S");
        lv_label_set_text(time_display_label, time_buf);
    }
}

void display_touch_init(void) {
    lvgl_driver_init();
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    lv_disp_drv_register(&disp_drv);
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
}

// Add definition for lv_tick_task
static void lv_tick_task(void *arg) {
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

void time_overlay_init(void) {
    setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
    tzset();
    lv_obj_t* target_parent_for_time = lv_disp_get_layer_sys(NULL);
    if (!target_parent_for_time) {
        target_parent_for_time = lv_scr_act();
    }
    if (target_parent_for_time) {
        time_display_label = lv_label_create(target_parent_for_time, NULL);
        lv_obj_set_style_local_text_font(time_display_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, TERMINAL_FONT);
        lv_obj_set_style_local_text_color(time_display_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, TERMINAL_COLOR_FOREGROUND_ALT);
        lv_obj_align(time_display_label, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
        lv_label_set_text(time_display_label, "Starting...");
        lv_task_create(time_update_lv_task, 1000, LV_TASK_PRIO_LOW, NULL);
    }
}

void lvgl_tick_timer_setup(void) {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task, .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

void lvgl_full_init(void) {
    lv_init();
    display_touch_init();
    // time_overlay_init();
    lvgl_tick_timer_setup();
}


void ui_styles_init(void) {
    // Default Screen Background Style
    lv_style_init(&style_default_screen_bg);
    lv_style_set_bg_color(&style_default_screen_bg, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND);
    lv_style_set_bg_opa(&style_default_screen_bg, LV_STATE_DEFAULT, LV_OPA_COVER);

    // Default Label Style (for titles, static text items on menus)
    lv_style_init(&style_default_label);
    lv_style_set_text_font(&style_default_label, LV_STATE_DEFAULT, TERMINAL_FONT);
    lv_style_set_text_color(&style_default_label, LV_STATE_DEFAULT, TERMINAL_COLOR_FOREGROUND_ALT);
    lv_style_set_text_opa(&style_default_label, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_line_space(&style_default_label, LV_STATE_DEFAULT, TERMINAL_LABEL_LINE_SPACE);

    // Default Button Style (Normal and Pressed States)
    lv_style_init(&style_default_button);
    lv_style_set_radius(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_RADIUS);

    // Normal state
    lv_style_set_bg_opa(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_BG_OPA);
    lv_style_set_bg_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND); // Transparent, relies on border
    lv_style_set_border_width(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_BORDER_WIDTH);
    lv_style_set_border_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BUTTON_BORDER);
    lv_style_set_text_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BUTTON_TEXT);
    lv_style_set_text_opa(&style_default_button, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_font(&style_default_button, LV_STATE_DEFAULT, TERMINAL_FONT);
    lv_style_set_pad_all(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_INNER_PADDING);


    // Pressed state
    lv_style_set_bg_opa(&style_default_button, LV_STATE_PRESSED, LV_OPA_COVER); // Make background visible on press
    lv_style_set_bg_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_BG);
    lv_style_set_text_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_TEXT);
    lv_style_set_border_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_BORDER); // Change border on press

    // Style for transparent containers (used to wrap static labels) - remains the same
    lv_style_init(&style_transparent_container);
    lv_style_set_bg_opa(&style_transparent_container, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_transparent_container, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_all(&style_transparent_container, LV_STATE_DEFAULT, 0);

    // Focused Button Style
    lv_style_set_text_color(&style_default_button, LV_STATE_FOCUSED, lv_color_hex(0xFF0000)); // Red text when focused
}