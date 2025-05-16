#include "menu_functions.h"
#include "ota_manager.h"

#define TAG_MENU_FUNC "menu_func"

static void close_modal_from_child(lv_obj_t *obj);
static void ok_button_cb(lv_obj_t *obj, lv_event_t event);


void register_menu_functions(void) {
    // Register the menu functions with the predefined function list
    G_PredefinedFunctions["OTA_UPDATE"] = trigger_ota_update_from_menu;
    G_PredefinedFunctions["TOGGLE_WIFI"] = toggle_wifi_from_menu;
    G_PredefinedFunctions["SHOW_WIFI_STATUS"] = show_wifi_status_from_menu;
    G_PredefinedFunctions["TELESCOPE_CONTROL"] = open_telescope_control_modal;
}

void trigger_ota_update_from_menu(void) {
    ESP_LOGI(TAG_MENU_FUNC, "OTA update triggered from menu");
    wifi_mode_t current_mode;
    esp_err_t err = esp_wifi_get_mode(&current_mode);
    bool wifi_is_on = (err == ESP_OK && current_mode != WIFI_MODE_NULL);

    if (!wifi_is_on) {
        auto yes_cb = [](lv_obj_t *obj, lv_event_t event) {
            if (event == LV_EVENT_CLICKED) {
                close_modal_from_child(obj);
                lv_obj_t *status_msg = lv_label_create(lv_scr_act(), NULL);
                lv_label_set_text(status_msg, "Enabling WiFi...");
                lv_obj_add_style(status_msg, LV_OBJ_PART_MAIN, &style_default_label);
                lv_obj_align(status_msg, NULL, LV_ALIGN_CENTER, 0, 0);
                lv_task_t *wifi_task = lv_task_create([](lv_task_t *task) {
                    g_wifi_intentional_stop = false;
                    Xasin::MQTT::Handler::start_wifi(WIFI_STATION_SSID, WIFI_STATION_PASSWD);
                    lv_obj_t *msg = (lv_obj_t*)task->user_data;
                    if (msg) {
                        lv_label_set_text(msg, "WiFi enabled.\nChecking for updates...");
                        lv_task_t *ota_task = lv_task_create([](lv_task_t *ota_task) {
                            trigger_ota_update();
                            lv_obj_t *msg = (lv_obj_t*)ota_task->user_data;
                            if (msg) {
                                lv_obj_del(msg);
                            }
                            lv_task_del(ota_task);
                        }, 5000, LV_TASK_PRIO_MID, NULL);
                        ota_task->user_data = msg;
                        lv_task_once(ota_task);
                    }
                    lv_task_del(task);
                }, 100, LV_TASK_PRIO_MID, NULL);
                wifi_task->user_data = status_msg;
                lv_task_once(wifi_task);
            }
        };
        auto no_cb = [](lv_obj_t *obj, lv_event_t event) {
            if (event == LV_EVENT_CLICKED) {
                close_modal_from_child(obj);
            }
        };
        create_modal_dialog(
            "WIFI REQUIRED",
            "WiFi is currently OFF.\nIt must be enabled for OTA updates.\nEnable WiFi and continue?",
            "YES", yes_cb, "NO", no_cb
        );
        return;
    }

    auto yes_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            close_modal_from_child(obj);
            lv_obj_t *update_msg = lv_label_create(lv_scr_act(), NULL);
            lv_label_set_text(update_msg, "Checking for updates...\nPlease wait.");
            lv_obj_add_style(update_msg, LV_OBJ_PART_MAIN, &style_default_label);
            lv_obj_align(update_msg, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_task_t *ota_task = lv_task_create([](lv_task_t *task) {
                trigger_ota_update();
                lv_task_del(task);
            }, 500, LV_TASK_PRIO_MID, NULL);
            lv_task_once(ota_task);
        }
    };
    auto no_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            close_modal_from_child(obj);
        }
    };
    create_modal_dialog(
        "OTA UPDATE",
        "Start firmware update?\nDevice will reboot if update is available.",
        "YES", yes_cb, "NO", no_cb
    );
}

void toggle_wifi_from_menu(void) {
    ESP_LOGI(TAG_MENU_FUNC, "WiFi toggle triggered from menu");
    wifi_mode_t current_mode;
    esp_err_t err = esp_wifi_get_mode(&current_mode);
    bool wifi_is_on = (err == ESP_OK && current_mode != WIFI_MODE_NULL);

    // Button event handlers
    auto yes_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            bool wifi_is_on = (bool)lv_obj_get_user_data(obj);
            close_modal_from_child(obj);
            lv_obj_t *status_msg = lv_label_create(lv_scr_act(), NULL);
            if (wifi_is_on) {
                lv_label_set_text(status_msg, "Turning WiFi OFF...");
                lv_obj_add_style(status_msg, LV_OBJ_PART_MAIN, &style_default_label);
                lv_obj_align(status_msg, NULL, LV_ALIGN_CENTER, 0, 0);
                lv_task_t *wifi_task = lv_task_create([](lv_task_t *task) {
                    g_wifi_intentional_stop = true;
                    esp_err_t stop_err = esp_wifi_stop();
                    lv_obj_t *msg = (lv_obj_t*)task->user_data;
                    if (msg) {
                        if (stop_err == ESP_OK) {
                            lv_label_set_text(msg, "WiFi turned OFF");
                        } else {
                            lv_label_set_text(msg, "WiFi OFF failed!");
                            ESP_LOGE(TAG_MENU_FUNC, "Failed to stop WiFi: %s", esp_err_to_name(stop_err));
                        }
                        lv_task_t *msg_task = lv_task_create([](lv_task_t *msg_task) {
                            lv_obj_t *msg = (lv_obj_t*)msg_task->user_data;
                            if (msg) {
                                lv_obj_del(msg);
                            }
                            lv_task_del(msg_task);
                        }, 2000, LV_TASK_PRIO_LOW, NULL);
                        msg_task->user_data = msg;
                        lv_task_once(msg_task);
                    }
                    lv_task_del(task);
                }, 100, LV_TASK_PRIO_MID, NULL);
                wifi_task->user_data = status_msg;
                lv_task_once(wifi_task);
            } else {
                lv_label_set_text(status_msg, "Turning WiFi ON...");
                lv_obj_add_style(status_msg, LV_OBJ_PART_MAIN, &style_default_label);
                lv_obj_align(status_msg, NULL, LV_ALIGN_CENTER, 0, 0);
                lv_task_t *wifi_task = lv_task_create([](lv_task_t *task) {
                    g_wifi_intentional_stop = false;
                    Xasin::MQTT::Handler::start_wifi(WIFI_STATION_SSID, WIFI_STATION_PASSWD);
                    lv_obj_t *msg = (lv_obj_t*)task->user_data;
                    if (msg) {
                        lv_label_set_text(msg, "WiFi turned ON\nConnecting...");
                        lv_task_t *msg_task = lv_task_create([](lv_task_t *msg_task) {
                            lv_obj_t *msg = (lv_obj_t*)msg_task->user_data;
                            if (msg) {
                                lv_obj_del(msg);
                            }
                            lv_task_del(msg_task);
                        }, 2000, LV_TASK_PRIO_LOW, NULL);
                        msg_task->user_data = msg;
                        lv_task_once(msg_task);
                    }
                    lv_task_del(task);
                }, 100, LV_TASK_PRIO_MID, NULL);
                wifi_task->user_data = status_msg;
                lv_task_once(wifi_task);
            }
        }
    };
    auto no_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            close_modal_from_child(obj);
        }
    };
    create_modal_dialog(
        "WIFI CONTROL",
        wifi_is_on ? "WiFi is currently ON\nDo you want to turn it OFF?" : "WiFi is currently OFF\nDo you want to turn it ON?",
        "YES", yes_cb, "NO", no_cb, (void*)(wifi_is_on ? 1 : 0), nullptr, 3
    );
}

void show_wifi_status_from_menu(void) {
    ESP_LOGI(TAG_MENU_FUNC, "WiFi status check triggered from menu");
    ModalDialogParts dialog_parts = create_modal_dialog(
        "WIFI STATUS",
        "Checking WiFi status...",
        "CLOSE",
        ok_button_cb,
        nullptr, nullptr, nullptr, nullptr, 1.5 // taller dialog for status
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
            if (g_wifi_intentional_stop) {
                snprintf(status_buf, sizeof(status_buf), "WiFi Status: OFF (INTENTIONAL)\n\nWiFi was manually disabled.\nUse 'Toggle WiFi' to enable.");
            } else {
                snprintf(status_buf, sizeof(status_buf), "WiFi Status: OFF\n\nWiFi is currently disabled.\nUse 'Toggle WiFi' to enable.");
            }
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
    static lv_style_t modal_style;
    lv_style_init(&modal_style);
    lv_style_set_bg_color(&modal_style, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND);
    lv_style_set_bg_opa(&modal_style, LV_STATE_DEFAULT, LV_OPA_70);
    lv_style_set_border_width(&modal_style, LV_STATE_DEFAULT, 0);

    ModalDialogParts parts = {};
    parts.modal_bg = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_add_style(parts.modal_bg, LV_OBJ_PART_MAIN, &modal_style);
    lv_obj_set_pos(parts.modal_bg, 0, 0);
    lv_obj_set_size(parts.modal_bg, LV_HOR_RES, LV_VER_RES);

    parts.dialog = lv_obj_create(parts.modal_bg, NULL);
    lv_obj_add_style(parts.dialog, LV_OBJ_PART_MAIN, &style_default_screen_bg);
    lv_obj_set_size(parts.dialog, LV_HOR_RES * 8 / 10, LV_VER_RES / dialog_height_div);
    lv_obj_align(parts.dialog, NULL, LV_ALIGN_CENTER, 0, 0);

    parts.title = lv_label_create(parts.dialog, NULL);
    lv_label_set_text(parts.title, title_text);
    lv_obj_add_style(parts.title, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_align(parts.title, parts.dialog, LV_ALIGN_IN_TOP_MID, 0, 10);

    parts.msg = lv_label_create(parts.dialog, NULL);
    lv_label_set_text(parts.msg, msg_text);
    lv_obj_add_style(parts.msg, LV_OBJ_PART_MAIN, &style_default_label);
    lv_obj_align(parts.msg, parts.dialog, LV_ALIGN_CENTER, 0, 0);

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
    return parts;
}

// Helper to close a modal dialog from any child object (e.g., button)
static void close_modal_from_child(lv_obj_t *obj) {
    if (!obj) return;
    lv_obj_t *dialog = lv_obj_get_parent(obj);
    if (!dialog) return;
    lv_obj_t *modal_bg = lv_obj_get_parent(dialog);
    if (modal_bg) lv_obj_del(modal_bg);
}

// Generic OK button callback for modal dialogs
static void ok_button_cb(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        close_modal_from_child(obj);
    }
}

// Custom predefined function to play an audio file in the background
void play_audio_file_in_background(void) {
    const char* audio_file_path = "DEI/sounds/GameStart.raw"; // Replace with actual file path
    ESP_LOGI(TAG_MENU_FUNC, "Playing audio file in the background: %s", audio_file_path);

    // Correct call for audio playback (assuming audio_player_start is the correct function)
    esp_err_t result = audio_player_play_file(audio_file_path, 16000);
    if (result != ESP_OK) {
        ESP_LOGE(TAG_MENU_FUNC, "Failed to play audio file: %s", esp_err_to_name(result));
    }
}

void open_telescope_control_modal(void) {
    static TelescopeController* telescope = nullptr;
    static lv_task_t* joystick_task = nullptr;
    static bool modal_open = false;

    if (modal_open) return; // Prevent multiple modals
    modal_open = true;

    // Allocate and initialize the controller
    telescope = new TelescopeController();
    if (telescope->init() != ESP_OK) {
        delete telescope;
        telescope = nullptr;
        ModalDialogParts err_dialog = create_modal_dialog(
            "Telescope Error",
            "Failed to initialize telescope controller.",
            "CLOSE", [](lv_obj_t *obj, lv_event_t event) {
                if (event == LV_EVENT_CLICKED) {
                    close_modal_from_child(obj);
                    modal_open = false;
                }
            },
            nullptr, nullptr, nullptr, nullptr, 2
        );
        lv_group_t* joy_group = lvgl_joystick_get_group();
        lv_group_add_obj(joy_group, err_dialog.btn1);
        return;
    }

    // Modal close callback
    auto close_cb = [](lv_obj_t *obj, lv_event_t event) {
        if (event == LV_EVENT_CLICKED) {
            // Stop joystick polling task
            if (joystick_task) {
                lv_task_del(joystick_task);
                joystick_task = nullptr;
            }
            // Delete controller
            if (telescope) {
                delete telescope;
                telescope = nullptr;
            }
            close_modal_from_child(obj);
            modal_open = false;
        }
    };

    // Create modal dialog
    ModalDialogParts dialog = create_modal_dialog(
        "Telescope Control",
        "Use the joystick to move the telescope.\nPress CLOSE to exit.",
        "CLOSE", close_cb,
        nullptr, nullptr, nullptr, nullptr, 2
    );
    lv_group_t* joy_group = lvgl_joystick_get_group();
    lv_group_add_obj(joy_group, dialog.btn1);

    // Joystick polling task
    joystick_task = lv_task_create([](lv_task_t *task) {
        if (!telescope) return;
        JoystickState_t js;
        extern mcp23008_t mcp23008_device;
        if (joystick_read_state(&mcp23008_device, &js) == ESP_OK) {
            telescope->process_joystick_input(&js);
        }
    }, 100, LV_TASK_PRIO_LOW, NULL); // 100ms polling
}
