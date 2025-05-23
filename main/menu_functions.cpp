#include "menu_functions.h"
#include "ota_manager.h"
#include "laser_tag.h"
#include "esp_wifi.h" // Add this include for esp_wifi_* functions
#include "menu_log.h"
#include <functional>
#include "xasin/audio/ByteCassette.h" // Added for Xasin Audio
#include "xasin/BatteryManager.h"
#include "joystick.h"
#include "setup.h"
#include "cd4053b_wrapper.h"
#include "ui_manager.h"
#include "telescope_game.h"

#define TAG_MENU_FUNC "menu_func"

static void close_modal_from_child(lv_obj_t *obj);
static void ok_button_cb(lv_obj_t *obj, lv_event_t event);
static void blink_mcp_led(MCP23008_NamedPin pin, uint32_t duration_ms);

extern mcp23008_t main_gpio_extender; // Extern for the main GPIO extender

static bool modal_open = false;

// Forward declarations for laser tag mode entry/exit
void enter_laser_tag_mode_from_menu(void);
static void exit_laser_tag_mode_from_menu(lv_obj_t *obj, lv_event_t event);

void register_menu_functions(void) {
    // Register the menu functions with the predefined function list
    // G_PredefinedFunctions["OTA_UPDATE"] = trigger_ota_update_from_menu;
    // G_PredefinedFunctions["START_WIFI"] = start_wifi_from_menu;
    G_PredefinedFunctions["SHOW_WIFI_STATUS"] = show_wifi_status_from_menu;
    G_PredefinedFunctions["PLAY_SOUND"] = play_audio_file_in_background;
    G_PredefinedFunctions["TELESCOPE_CONTROL"] = open_telescope_control_modal;
    G_PredefinedFunctions["ENTER_LASER_TAG_MODE"] = enter_laser_tag_mode_from_menu;
    G_PredefinedFunctions["BATTERY_STATUS"] = show_battery_status_from_menu;
    G_PredefinedFunctions["SHOW_RECENT_MESSAGES"] = show_recent_messages_from_menu;
}


void handle_focus_change(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_KEY) {
        uint32_t key = *((uint32_t *)lv_event_get_data()); // Retrieve key directly from event data
        if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT) {
            lv_group_focus_next((lv_group_t *)lv_obj_get_group(obj)); // Cast to lv_group_t*
        }
        if (key == LV_KEY_UP || key == LV_KEY_LEFT) {
            lv_group_focus_prev((lv_group_t *)lv_obj_get_group(obj)); // Cast to lv_group_t*
        }
    }
}

void show_wifi_status_from_menu(void) {
    menu_log_add(TAG_MENU_FUNC, "WiFi status check triggered from menu");
    ModalDialogParts dialog_parts = create_modal_dialog(
        "WIFI STATUS",
        "Checking WiFi status...",
        "CLOSE",
        ok_button_cb,
        nullptr, nullptr, nullptr, nullptr
    );
    // Use dialog_parts.msg directly for the message label
    lv_task_t *status_task = lv_task_create([](lv_task_t *task) {
        lv_obj_t *msg = (lv_obj_t*)task->user_data;
        if (!msg) {
            lv_task_del(task);
            return;
        }
        wifi_mode_t mode;
        esp_err_t err = esp_wifi_get_mode(&mode);
        char status_buf[256];
        if (err != ESP_OK || mode == WIFI_MODE_NULL) {
            snprintf(status_buf, sizeof(status_buf), "WiFi Status: OFF\n\nWiFi is currently disabled.");
        } else {
            wifi_ap_record_t ap_info;
            err = esp_wifi_sta_get_ap_info(&ap_info);
            if (err != ESP_OK) {
                snprintf(status_buf, sizeof(status_buf), "WiFi Status: ON\n\nNot connected to any network.\nSSID: %s", WIFI_STATION_SSID);
            } else {
                tcpip_adapter_ip_info_t ip_info;
                tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                        (ip_info.ip.addr) & 0xff,
                        (ip_info.ip.addr >> 8) & 0xff,
                        (ip_info.ip.addr >> 16) & 0xff,
                        (ip_info.ip.addr >> 24) & 0xff);
                const char* signal_str;
                if (ap_info.rssi >= -50) {
                    signal_str = "Excellent";
                } else if (ap_info.rssi >= -60) {
                    signal_str = "Good";
                } else if (ap_info.rssi >= -70) {
                    signal_str = "Fair";
                } else {
                    signal_str = "Poor";
                }
                snprintf(status_buf, sizeof(status_buf), "WiFi Status: CONNECTED\n\nNetwork: %s\nIP: %s\nSignal: %s (%d dBm)\nChannel: %d",
                        ap_info.ssid, ip_str, signal_str, ap_info.rssi, ap_info.primary);
            }
        }
        lv_label_set_text(msg, status_buf);
        lv_obj_align(msg, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_task_del(task);
    }, 100, LV_TASK_PRIO_MID, NULL);
    status_task->user_data = dialog_parts.msg;
    lv_task_once(status_task);
    // Add joystick group support for close button
    lv_group_t* joy_group = lvgl_joystick_get_group();
    lv_group_add_obj(joy_group, dialog_parts.btn1); // btn1 is the CLOSE button
}

// Updated helper to create a modal dialog and return its parts
ModalDialogParts create_modal_dialog(const char* title_text, const char* msg_text,
                                    const char* btn1_text, lv_event_cb_t btn1_cb,
                                    const char* btn2_text, lv_event_cb_t btn2_cb,
                                    void* btn1_user_data, void* btn2_user_data,
                                    int dialog_height_div) {

    if (modal_open) ui_reinit_current_menu();
    modal_open = true;

    static lv_style_t modal_style;
    lv_style_init(&modal_style);
    lv_style_set_bg_color(&modal_style, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND);
    lv_style_set_bg_opa(&modal_style, LV_STATE_DEFAULT, LV_OPA_90);
    lv_style_set_border_width(&modal_style, LV_STATE_DEFAULT, 0);

    ModalDialogParts parts = {};
    parts.modal_bg = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_add_style(parts.modal_bg, LV_OBJ_PART_MAIN, &modal_style);
    lv_obj_set_pos(parts.modal_bg, 0, 0);
    lv_obj_set_size(parts.modal_bg, LV_HOR_RES, LV_VER_RES);

    parts.dialog = lv_page_create(parts.modal_bg, NULL);
    lv_obj_add_style(parts.dialog, LV_PAGE_PART_BG, &style_default_screen_bg);
    lv_obj_set_size(parts.dialog, LV_HOR_RES * 8 / 10, LV_VER_RES / dialog_height_div);
    lv_obj_align(parts.dialog, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_page_set_scrl_layout(parts.dialog, LV_LAYOUT_CENTER);

    parts.title = lv_label_create(parts.dialog, NULL);
    lv_label_set_long_mode(parts.title, LV_LABEL_LONG_BREAK);
    lv_label_set_text(parts.title, title_text);
    lv_obj_add_style(parts.title, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_set_width(parts.title, lv_obj_get_width(parts.dialog));

    parts.msg = lv_label_create(parts.dialog, NULL);
    lv_label_set_long_mode(parts.msg, LV_LABEL_LONG_BREAK);
    lv_label_set_text(parts.msg, msg_text);
    lv_obj_add_style(parts.msg, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_set_width(parts.msg, lv_obj_get_width(parts.dialog));

    parts.btn1 = nullptr;
    parts.btn2 = nullptr;
    if (btn1_text) {
        parts.btn1 = lv_btn_create(parts.dialog, NULL);
        lv_obj_t *lbl1 = lv_label_create(parts.btn1, NULL);
        lv_label_set_text(lbl1, btn1_text);
        lv_obj_add_style(parts.btn1, LV_OBJ_PART_MAIN, &style_default_button);
        lv_obj_align(parts.btn1, parts.dialog, LV_ALIGN_IN_BOTTOM_LEFT, 20, -10);
        if (btn1_cb) lv_obj_set_event_cb(parts.btn1, btn1_cb);
        if (btn1_user_data) lv_obj_set_user_data(parts.btn1, btn1_user_data);
    }
    if (btn2_text) {
        parts.btn2 = lv_btn_create(parts.dialog, NULL);
        lv_obj_t *lbl2 = lv_label_create(parts.btn2, NULL);
        lv_label_set_text(lbl2, btn2_text);
        lv_obj_add_style(parts.btn2, LV_OBJ_PART_MAIN, &style_default_button);
        lv_obj_align(parts.btn2, parts.dialog, LV_ALIGN_IN_BOTTOM_RIGHT, -20, -10);
        if (btn2_cb) lv_obj_set_event_cb(parts.btn2, btn2_cb);
        if (btn2_user_data) lv_obj_set_user_data(parts.btn2, btn2_user_data);
    }

    lv_group_t* joy_group = lvgl_joystick_get_group();
    lv_group_remove_all_objs(joy_group);
    if (parts.btn1) lv_group_add_obj(joy_group, parts.btn1);
    if (parts.btn2) lv_group_add_obj(joy_group, parts.btn2);
    lv_group_focus_obj(parts.btn1 ? parts.btn1 : parts.btn2);

    return parts;
}

// Helper to close a modal dialog from any child object (e.g., button)
static void close_modal_from_child(lv_obj_t *obj) {
    if (!modal_open) return; // Prevent double close
    modal_open = false;
    menu_log_add(TAG_MENU_FUNC, "Closing modal dialog");
    ui_reinit_current_menu();
}

// Generic OK button callback for modal dialogs
static void ok_button_cb(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        menu_log_add(TAG_MENU_FUNC, "OK button clicked");
        close_modal_from_child(obj);
    }
}

// Custom predefined function to play an audio file in the background
void play_audio_file_in_background(void) {
    const char* audio_file_path = "ride"; // Replace with actual file path
    menu_log_add(TAG_MENU_FUNC, "Playing audio file using Xasin::Audio: %s", audio_file_path);

    g_sound_manager.play_audio(audio_file_path);

}

void open_telescope_control_modal(void) {
    static TelescopeController* telescope = nullptr;
    static lv_task_t* joystick_task = nullptr;

    // Helper: wrap send_command and read_response to update labels
    struct TeleProxy : public TelescopeController {
        lv_obj_t* commanded;
        lv_obj_t* actual_label; // Add actual_label here
        TeleProxy(lv_obj_t* cmd, lv_obj_t* act_lbl) : commanded(cmd), actual_label(act_lbl) {}

        esp_err_t send_command(const char* cmd) override {
            // Parse and update commanded Az/El
            float az, el;
            if (sscanf(cmd, "AZ %f EL %f", &az, &el) == 2) {
                lv_label_set_text_fmt(commanded, "Commanded Az/El: %.1f / %.1f", az, el);
                std::string message;
                if (check_secret_combination(az, el, message)) {
                    lv_label_set_text_fmt(commanded, "%s", message.c_str());
                }
            }
            return TelescopeController::send_command(cmd);
        }

        void update_actual_label(float az, float el) {
            if (actual_label) {
                lv_label_set_text_fmt(actual_label, "Actual Az/El: %.1f / %.1f", az, el);
            }
        }
    };

    // Create modal dialog
    ModalDialogParts dialog = create_modal_dialog(
        "Telescope Control",
        "Use the joystick to move the telescope.\nPress CLOSE to exit.",
        "CLOSE", nullptr,
        nullptr, nullptr, nullptr, nullptr
    );

    // Add labels for commanded and actual Az/El
    lv_obj_t* commanded_label = lv_label_create(dialog.dialog, NULL);
    lv_label_set_text(commanded_label, "Commanded Az/El: ");
    lv_obj_add_style(commanded_label, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_align(commanded_label, dialog.msg, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    lv_obj_t* actual_label = lv_label_create(dialog.dialog, NULL);
    lv_label_set_text(actual_label, "Actual Az/El: ");
    lv_obj_add_style(actual_label, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_align(actual_label, commanded_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    telescope = new TeleProxy(commanded_label, actual_label);

    // Modal close callback
    auto close_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            if (joystick_task) {
                lv_task_del(joystick_task);
                joystick_task = nullptr;
            }
            if (telescope) {
                delete telescope;
                telescope = nullptr;
            }
            close_modal_from_child(obj);
            modal_open = false;
        }
    };
    lv_obj_set_event_cb(dialog.btn1, close_cb);

    // Joystick polling task
    joystick_task = lv_task_create([](lv_task_t *task) {
        if (!telescope) return;
        JoystickState_t js;
        if (joystick_read_state(&main_gpio_extender, &js) == ESP_OK) {
            telescope->process_joystick_input(&js);
        }
    }, 100, LV_TASK_PRIO_LOW, NULL); // 100ms polling
}

// Helper: blink any MCP23008 LED for a given duration (non-blocking)
static void blink_mcp_led(MCP23008_NamedPin pin, uint32_t duration_ms) {
    mcp23008_wrapper_write_pin(&main_gpio_extender, pin, true);
    lv_task_t* led_task = lv_task_create([](lv_task_t* task) {
        MCP23008_NamedPin pin = (MCP23008_NamedPin)(uintptr_t)task->user_data;
        mcp23008_wrapper_write_pin(&main_gpio_extender, pin, false);
        lv_task_del(task);
    }, duration_ms, LV_TASK_PRIO_LOW, (void*)(uintptr_t)pin);
    lv_task_once(led_task);
}

// void trigger_ota_update_from_menu(void) {
//     menu_log_add(TAG_MENU_FUNC, "OTA update triggered from menu");
//     static lv_obj_t* ota_label = nullptr;
//     // Custom close callback to mark label as invalid
//     auto ota_close_cb = [](lv_obj_t *obj, lv_event_t event) {
//         if (event == LV_EVENT_CLICKED) {
//             ota_label = nullptr;
//             close_modal_from_child(obj);
//         }
//     };
//     ModalDialogParts dialog = create_modal_dialog(
//         "OTA UPDATE",
//         "Starting OTA update...\nPlease wait.",
//         "CLOSE",
//         ota_close_cb,
//         nullptr, nullptr, nullptr, nullptr
//     );
//     ota_label = dialog.msg;
//     // Lambda checks if label is still valid
//     auto update_label = [](const char* msg) {
//         if (ota_label) {
//             lv_label_set_text(ota_label, msg);
//             lv_obj_align(ota_label, NULL, LV_ALIGN_CENTER, 0, 0);
//         }
//     };
//     static OtaManager* ota_mgr = nullptr;
//     // Do not delete ota_mgr here; let the OTA task delete itself!
//     ota_mgr = new OtaManager(update_label);
//     ota_mgr->start_update();
//     // Add joystick group support for close button
//     lv_group_t* joy_group = lvgl_joystick_get_group();
//     lv_group_add_obj(joy_group, dialog.btn1);
// }

// // Helper to get log buffer as a single string (for dashboard, etc)
std::string get_menu_log_as_string(int max_lines = 16) {
    auto lines = menu_log_get_buffer();
    std::string out;
    int start = (int)lines.size() > max_lines ? (int)lines.size() - max_lines : 0;
    for (int i = start; i < (int)lines.size(); ++i) {
        out += lines[i] + "\n";
    }
    if (out.empty()) out = "(No log messages)";
    return out;
}

// Example: Laser Tag Dashboard screen
void show_laser_tag_dashboard(void) {
    // Compose dashboard text (log + status)
    std::string log_text = get_menu_log_as_string();
    std::string status = "Laser Tag Mode Active\n";
    std::string msg = status + "\nRecent Log:\n" + log_text;
    ModalDialogParts dialog = create_modal_dialog(
        "Laser Tag Dashboard",
        msg.c_str(),
        "EXIT", exit_laser_tag_mode_from_menu,
        nullptr, nullptr, nullptr, nullptr, 2
    );
    // Add joystick group support for close button
    lv_group_t* joy_group = lvgl_joystick_get_group();
    lv_group_add_obj(joy_group, dialog.btn1);
}

// Menu function to enter laser tag mode
void enter_laser_tag_mode_from_menu(void) {
    if (laser_tag_mode_enter()) {
        menu_log_add(TAG_MENU_FUNC, "Laser tag mode entered successfully.");
        show_laser_tag_dashboard();
    } else {
        menu_log_add(TAG_MENU_FUNC, "Failed to enter laser tag mode.");
        ModalDialogParts dialog = create_modal_dialog(
            "Laser Tag Error",
            "Failed to enter laser tag mode.",
            "CLOSE", exit_laser_tag_mode_from_menu,
            nullptr, nullptr, nullptr, nullptr, 2
        );
        lv_group_t* joy_group = lvgl_joystick_get_group();
        lv_group_add_obj(joy_group, dialog.btn1);
    }
}

// Menu function to exit laser tag mode
void exit_laser_tag_mode_from_menu(lv_obj_t *obj, lv_event_t event) {
    if (event != LV_EVENT_CLICKED) return; // Only handle click event
    laser_tag_mode_exit();
    menu_log_add(TAG_MENU_FUNC, "Laser tag mode exited.");
    ok_button_cb(obj, event);
}

void show_battery_status_from_menu(void) {
    float voltage = 0.0f;
    esp_err_t ret = battery_read_voltage(&main_gpio_extender, &voltage);
    char msg[128];
    if (ret == ESP_OK) {
        // Update the global battery manager
        g_battery_manager.set_voltage((uint32_t)(voltage * 1000.0f));
        uint8_t percent = g_battery_manager.current_capacity();
        snprintf(msg, sizeof(msg), "Battery Voltage: %.2f V\nBattery Level: %u%%\nStatus: %s",
                 voltage, percent, g_battery_manager.battery_ok() ? "OK" : "LOW");
    } else {
        snprintf(msg, sizeof(msg), "Failed to read battery voltage!\nError: %d", ret);
    }
    // Restore CD4053B S1 path to joystick axis after battery read
    cd4053b_select_named_path(&main_gpio_extender, CD4053B_S1_PATH_A0_JOYSTICK_AXIS1);

    ModalDialogParts dialog = create_modal_dialog(
        "Battery Status",
        msg,
        "CLOSE", ok_button_cb
    );
    // Add joystick group support for close button
    lv_group_t* joy_group = lvgl_joystick_get_group();
    if (dialog.btn1) lv_group_add_obj(joy_group, dialog.btn1);
    lv_group_focus_obj(dialog.btn1);
}

void show_recent_messages_from_menu(void) {
    menu_log_add(TAG_MENU_FUNC, "Displaying recent messages from menu");

    // Retrieve recent messages from the log buffer
    std::string recent_messages = get_menu_log_as_string();

    // Use the create_text_display_screen_impl function to display the messages
    lv_obj_t* screen = create_text_display_screen_impl(
        "Recent Messages", // Title of the screen
        recent_messages,    // Content to display
        false,              // Content is not a file
        "Main Menu"         // Invoking parent menu name
    );

    // Load the created screen
    lv_scr_load(screen);
}