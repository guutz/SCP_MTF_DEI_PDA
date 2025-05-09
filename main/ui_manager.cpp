#include "lvgl.h"
#include "lvgl_helpers.h" // For lv_disp_get_hor_res etc.
#include "ui_manager.h"   // For the declarations above
#include "esp_log.h"
#include <cinttypes>      // For PRIu32 macro if used in logging

LV_FONT_DECLARE(lv_font_firacode_16);
LV_IMG_DECLARE(SCP_Foundation);

static const char* TAG_UI_MGR = "ui_manager";

// --- Screen Creation Function Implementations ---
// (These create and return the top-level object for each screen)
static lv_obj_t* create_splash_screen(void);
static lv_obj_t* create_main_menu_screen(void);
static lv_obj_t* create_settings_screen(void); // Example for another screen

// --- LVGL Task Callback for initial splash timeout ---
static void initial_splash_timeout_cb(lv_task_t *task);

// --- Generic Screen Transition Function Implementation ---
void ui_load_screen_animated(screen_create_func_t target_screen_creator,
                             lv_scr_load_anim_t anim_type,
                             uint32_t anim_time,
                             uint32_t anim_delay,
                             bool auto_delete_old) {
    if (!target_screen_creator) {
        ESP_LOGE(TAG_UI_MGR, "ui_load_screen_animated: NULL screen creator function provided.");
        return;
    }

    // Call the provided function to create the new screen object
    lv_obj_t *new_screen = target_screen_creator();

    if (!new_screen) {
        ESP_LOGE(TAG_UI_MGR, "ui_load_screen_animated: Screen creator function failed to return a screen object.");
        return;
    }

    ESP_LOGI(TAG_UI_MGR, "Loading new screen with animation. Type: %d, Time: %" PRIu32 "ms", (int)anim_type, anim_time);
    lv_scr_load_anim(new_screen, anim_type, anim_time, anim_delay, auto_delete_old);
}


// --- UI Initialization ---
void ui_init(lv_task_t *current_init_task) {
    (void)current_init_task;
    ESP_LOGI(TAG_UI_MGR, "UI Init: Creating and loading splash screen.");

    lv_obj_t *splash_screen_obj = create_splash_screen();
    lv_disp_load_scr(splash_screen_obj); // Load initial screen

    // Task to transition from splash to main menu after a delay
    lv_task_t *transition_task = lv_task_create(initial_splash_timeout_cb, 3000, LV_TASK_PRIO_MID, NULL);
    lv_task_once(transition_task);
}

// --- Timed Task Callback for Splash Screen ---
static void initial_splash_timeout_cb(lv_task_t *task) {
    (void)task;
    ESP_LOGI(TAG_UI_MGR, "Splash timeout. Transitioning to Main Menu screen.");
    // Use the generic function to load the main menu screen
    ui_load_screen_animated(create_main_menu_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, true);
}


// --- Screen Creation Function Implementations ---

static lv_obj_t* create_splash_screen(void) {
    ESP_LOGI(TAG_UI_MGR, "Creating splash screen elements.");
    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));

    lv_obj_t *scp_logo = lv_img_create(screen, NULL);
    lv_img_set_src(scp_logo, &SCP_Foundation);
    lv_obj_set_style_local_image_recolor(scp_logo, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_image_recolor_opa(scp_logo, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_align(scp_logo, NULL, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *loading_label = lv_label_create(screen, NULL);
    lv_obj_set_style_local_text_font(loading_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(loading_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_align(loading_label, scp_logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    return screen;
}

static lv_obj_t* create_main_menu_screen(void) {
    ESP_LOGI(TAG_UI_MGR, "Creating main menu screen elements.");
    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));

    lv_obj_t *title_label = lv_label_create(screen, NULL);
    lv_obj_set_style_local_text_font(title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_label_set_text(title_label, "SCP PDA - Main Menu");
    lv_obj_align(title_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

    // Example: Button to go to a "Settings" screen
    lv_obj_t *btn_to_settings = lv_btn_create(screen, NULL);
    lv_obj_set_style_local_text_font(btn_to_settings, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(btn_to_settings, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
    lv_obj_set_style_local_bg_color(btn_to_settings, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_bg_opa(btn_to_settings, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_size(btn_to_settings, 320, 25);
    lv_obj_align(btn_to_settings, NULL, LV_ALIGN_CENTER, 0, 0);

    // Event callback for the button
    lv_obj_set_event_cb(btn_to_settings, [](lv_obj_t * obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            ESP_LOGI(TAG_UI_MGR, "Settings button clicked. Transitioning to Settings screen.");
            // Use the generic function
            ui_load_screen_animated(create_settings_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, true);
        }
    });
    lv_obj_t *btn_label = lv_label_create(btn_to_settings, NULL);
    lv_label_set_text(btn_label, "Settings");

    return screen;
}

static lv_obj_t* create_settings_screen(void) {
    ESP_LOGI(TAG_UI_MGR, "Creating settings screen elements.");
    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));

    lv_obj_t *title_label = lv_label_create(screen, NULL);
    lv_obj_set_style_local_text_font(title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_firacode_16);
    lv_obj_set_style_local_text_color(title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_label_set_text(title_label, "Settings");
    lv_obj_align(title_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

    // Example: Button to go back to Main Menu
    lv_obj_t *btn_to_main = lv_btn_create(screen, NULL);
    lv_obj_align(btn_to_main, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_event_cb(btn_to_main, [](lv_obj_t * obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            ESP_LOGI(TAG_UI_MGR, "Back to Main Menu button clicked.");
            // Use the generic function
            ui_load_screen_animated(create_main_menu_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
        }
    });
    lv_obj_t *btn_label = lv_label_create(btn_to_main, NULL);
    lv_label_set_text(btn_label, "Back to Menu");

    return screen;
}