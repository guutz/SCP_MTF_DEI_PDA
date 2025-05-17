#include "joystick.h" // Added for lvgl_joystick_get_group()
#include "telescope_controller.h" // For telescope controller
#include "audio_player.h"
#include "menu_structures.h"

#include "setup.h"

#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "sd_manager.h"
#include "sd_raw_access.h"
#include "ui_manager.h"

// Struct to hold references to modal dialog parts for easy access
struct ModalDialogParts {
    lv_obj_t* modal_bg;
    lv_obj_t* dialog;
    lv_obj_t* title;
    lv_obj_t* msg;
    lv_obj_t* btn1;
    lv_obj_t* btn2;
};

// Forward declarations for modal dialog helpers
ModalDialogParts create_modal_dialog(const char* title_text, const char* msg_text,
                                    const char* btn1_text, lv_event_cb_t btn1_cb,
                                    const char* btn2_text = nullptr, lv_event_cb_t btn2_cb = nullptr,
                                    void* btn1_user_data = nullptr, void* btn2_user_data = nullptr,
                                    int dialog_height_div = 3);



void register_menu_functions(void);


/**
 * @brief Trigger OTA update from the menu
 * This function is registered with G_PredefinedFunctions to allow
 * the OTA update to be triggered from a menu button
 */
void trigger_ota_update_from_menu(void);

/**
 * @brief Toggle WiFi on or off from the menu
 * This function is registered with G_PredefinedFunctions to allow
 * WiFi to be toggled from a menu button
 */
void toggle_wifi_from_menu(void);

/**
 * @brief Display WiFi status information
 * Shows current WiFi connection status, IP address, and signal strength
 */
void show_wifi_status_from_menu(void);

/**
 * @brief Play audio file in the background
 * This function is registered with G_PredefinedFunctions to allow
 * audio playback from a menu button
 */
void play_audio_file_in_background(void);

/**
 * @brief Open telescope control modal from the menu
 * This function is registered with G_PredefinedFunctions to allow
 * telescope control from a menu button
 */
void open_telescope_control_modal(void);

#ifdef __cplusplus
extern "C" {
#endif

// Expose the OTA status label pointer and update function
extern lv_obj_t* ota_status_label;
void update_ota_status_label(const char* text);

#ifdef __cplusplus
}
#endif
