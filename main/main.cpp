#include "ui_manager.h"
#include "sd_manager.h"
#include "stuff.h"
#include "menu_structures.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

#include <xnm/net_helpers.h>
#include "esp_wifi.h" // Added for esp_wifi_stop/start

#include "i2c_manager.h"
#include "mcp23008.h"
#include "mcp23008_wrapper.h"
#include "cd4053b_wrapper.h" // Added CD4053B wrapper include
#include "joystick.h"        // Added Joystick/ADC include

//                       ..,,,,,,,,,,,,,,,,.                                                                      
//                      .%@&%%##########%%@@(.                                                                    
//                      ,&&(,............*%@&,                                                                    
//                   .,/%@(,             .*#@#*.                                                                  
//                .,#@@%/,.               ..,(&@&/..                                                              
//             .*%@@(,.          ,//.         .,*#@@#,                                                            
//            ,#@%*,.          .,#@@/.           .*(&@/.                                                          
//          .(@%*.        .,*#%&@@@@@@&%(*..        ,/&&*.                                                        
//        .,&@/.       ,(&@@@@@@@@@@@@@@@@@@%*.       ,#@#.                                                       
//       .#@%*.      *%@@@@&(*,,*%@@(,.,/#@@@@@(.      ./@%/                                                      
//      .(@&*.     ./@@@@#,..   ,#@@/.   .,/%@@@&,      ,(@&*                                                     
//      *@#/.    .*#@@@(,      .*%@@(,      .*%@@@/,     ,(&&.                                                    
//     ,&&*.     *@@@&*.      .(@@@@@@*       ,(@@@&,    .,/@(.                                                   
//     /@(..    ,&@@@/.        .(@@@@/.        ,#@@@#.    .,%&,                                                   
//    .#@/.     /@@@%,          ,#@@(.         ./&@@&,    .,#@*                                                   
//    .%&*.    .(@@@(.           .**.           ,%@@@*    .,/@/                                                   
//    .%&*.    .#@@@(.   ,//((###,  /###((/*.   ,%@@@*    ../@(                                                   
//    .#@*.    ./@@@#,  .*&@@@@@/.  ,#@@@@@%,. .*&@@&,    .,(@/.                                                  
//   .(@@*      ,&@@@/,*(&@@@@@/.    ,#@@@@@&(,*(@@@#.    ..#@#,.                                                 
// .(@@/,       ./@@@@@@@#/*/%,.      ./%**/%@@@@@@&*       .*%@&,                                                
//  /@#,       .*%@@@@@#*.                  ./%@@@@@%*       .*%&,                                                
//  .*@#,      .#&(/#@@@&/..              ,*#@@@&(/(%/      .*&&,                                                 
//   ./@(,        .../&@@@*,.       ..,(&@@@@%,.         .*%&,                                                  
//     *@#,.          .*%@@@@@@&&%%%&&@@@@@@@(,           ,*&%,                                                   
//      ,&%/.            .,*#&@@@@@@@@@@%(,,.           .,#.                                                    
//       ,%@(,..,*,.          .........          ..,,...*%&(.                                                     
//        ,(@#%@@@@%*,                         .,/&@@@%@*.                                                      
//         .*(*,..*/&@&(*,.               ..,*#&@%*,,,*/#,                                                        
//                  ..*(&@@&%(/*******/(#&@@@%/,.                                                                 
//                       ..,*//##%%%%#(/*,...                                                                     
//                              ......                                                                            

static const char *TAG_MAIN = "main";
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void lvglTask(void *pvParameter);
static void peripheralsTask(void *pvParameter);
SemaphoreHandle_t xGuiSemaphore;

// Flag to indicate if Wi-Fi stop was intentional (e.g., for ADC readings)
static volatile bool g_wifi_intentional_stop = false;

void get_formatted_current_time(char *buf, size_t buf_len, const char *format) {
    if (!buf || buf_len == 0) return;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo); // Thread-safe version of localtime
    strftime(buf, buf_len, format, &timeinfo);
}

static lv_obj_t *time_display_label = NULL; // Global for the time label

static void time_update_lv_task(lv_task_t *task) {
    if (time_display_label) {
        char time_buf[64];
        get_formatted_current_time(time_buf, sizeof(time_buf), "%H:%M:%S");
        lv_label_set_text(time_display_label, time_buf);
    }
}


esp_err_t event_handler(void *context, system_event_t *event) {
    // If Wi-Fi was stopped intentionally, don't try to reconnect on disconnect event
    if (event->event_id == SYSTEM_EVENT_STA_DISCONNECTED && g_wifi_intentional_stop) {
        ESP_LOGI(TAG_MAIN, "Wi-Fi intentionally disconnected, skipping reconnect attempt.");
        // Potentially inform MQTT handler that it's an intentional disconnect if it has such a feature
        // For now, we just prevent Xasin::MQTT::Handler::try_wifi_reconnect
    } else {
        Xasin::MQTT::Handler::try_wifi_reconnect(event);
    }

	// mqtt.wifi_handler(event);

	return ESP_OK;
}

// Function to enable/disable Wi-Fi for prioritizing dual-axis joystick readings
void set_joystick_dual_axis_priority(bool prioritize_dual_axis) {
    if (prioritize_dual_axis) {
        ESP_LOGI(TAG_MAIN, "Prioritizing dual-axis joystick: Stopping Wi-Fi.");
        g_wifi_intentional_stop = true; // Signal that the stop is intentional
        esp_err_t stop_err = esp_wifi_stop();
        if (stop_err == ESP_OK) {
            ESP_LOGI(TAG_MAIN, "Wi-Fi stopped successfully for ADC2 priority.");
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to stop Wi-Fi: %s", esp_err_to_name(stop_err));
            // If stopping fails, we might still have ADC2 issues.
            // Consider checking esp_wifi_get_mode to confirm if it's truly off if robust handling is needed.
        }
    } else {
        ESP_LOGI(TAG_MAIN, "Restoring normal Wi-Fi operation.");
        g_wifi_intentional_stop = false; // Clear the flag before starting
        esp_err_t start_err = esp_wifi_start(); // This will trigger events for the handler to reconnect
        if (start_err == ESP_OK) {
            ESP_LOGI(TAG_MAIN, "Wi-Fi start initiated. MQTT handler should reconnect.");
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to start Wi-Fi: %s", esp_err_to_name(start_err));
        }
    }
}

mcp23008_t mcp23008_device = {
    .port = I2C_NUM_0,
    .address = 0x20,
    .current = 0
};


extern "C" void app_main(void) {
    nvs_flash_init();

    tcpip_adapter_init();
    esp_event_loop_init(event_handler, 0);

    // XNM::NetHelpers::init_global_r3_ca();
    Xasin::MQTT::Handler::start_wifi(WIFI_STATION_SSID, WIFI_STATION_PASSWD);


    ESP_LOGI(TAG_MAIN, "Creating peripherals task.");
    xTaskCreatePinnedToCore(peripheralsTask, "peripherals", 1024 * 2, NULL, 5, NULL, 1);
    ESP_LOGI(TAG_MAIN, "Creating GUI task.");
    xTaskCreatePinnedToCore(lvglTask, "gui", 1024 * 8, NULL, 5, NULL, 1);
}

static void peripheralsTask(void *pvParameter) {
    (void) pvParameter;
    ESP_LOGI(TAG_MAIN, "Initializing I2C manager.");
    ESP_ERROR_CHECK(i2c_manager_init(I2C_NUM_0));
    ESP_LOGI(TAG_MAIN, "Initializing MCP23008 wrapper.");

    // Initial IODIR and GPPU are now set within mcp23008_wrapper_init using defaults
    ESP_ERROR_CHECK(mcp23008_wrapper_init(&mcp23008_device));

    // Initialize Joystick and ADC
    ESP_LOGI(TAG_MAIN, "Initializing Joystick and ADC.");
    ESP_ERROR_CHECK(joystick_init());

    // Set initial states for CD4053B switches using named paths
    // These correspond to setting S1, S2, and S3 to LOW (path 0 for each)
    // Joystick X (S1) and Y (S3) will be selected by joystick_read_state() as needed.
    // Set other switches to a known default if necessary.
    ESP_LOGI(TAG_MAIN, "Setting initial CD4053B paths for non-joystick/battery use.");
    // Example: Set S2 to its default path if it's not joystick/battery related.
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S2_PATH_B0_ACCESSORY_A);
    // Ensure joystick/battery paths are NOT active by default if joystick_read/battery_read are not called immediately
    // Or, ensure the first call to joystick_read/battery_read correctly sets its path.
    // For instance, ensure S1 is initially set for Joystick X if that's the primary use before battery reading.
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S1_PATH_A0_JOYSTICK_AXIS1);
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S3_PATH_C0_JOYSTICK_AXIS2);

    JoystickState_t current_joystick_state;
    float battery_voltage;
    
    while (1) {
        // Read Joystick State
        esp_err_t joy_ret = joystick_read_state(&mcp23008_device, &current_joystick_state);
        if (joy_ret == ESP_OK) {
            ESP_LOGI(TAG_MAIN, "Joystick: X=%d, Y=%d, Btn=%s", 
                     current_joystick_state.x, 
                     current_joystick_state.y, 
                     current_joystick_state.button_pressed ? "Pressed" : "Released");
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to read joystick state: %s", esp_err_to_name(joy_ret));
        }

        // Read Battery Voltage
        esp_err_t batt_ret = battery_read_voltage(&mcp23008_device, &battery_voltage);
        if (batt_ret == ESP_OK) {
            ESP_LOGI(TAG_MAIN, "Battery Voltage: %.2fV", battery_voltage);
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to read battery voltage: %s", esp_err_to_name(batt_ret));
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Read every second
    }
}


static void lvglTask(void *pvParameter) {
    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
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

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task, .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    // --- Initialize SD Card FIRST ---
    // sd_init already calls sd_register_with_lvgl in your version.
    // It's a one-time task.
    ESP_LOGI(TAG_MAIN, "Creating SD init task.");
    lv_task_t* sd_init_task = lv_task_create(sd_init, 0, LV_TASK_PRIO_MID, NULL);
    lv_task_once(sd_init_task);
    
    // Crucial: Wait or ensure sd_init completes before ui_init tries to parse files from SD.
    // For simplicity, ui_init will attempt parsing. A robust solution would use a semaphore or event group.
    // Or, make sd_init blocking or have parse_menu_definition_file call sd_init if not done.
    // The lv_task_create and lv_task_once are non-blocking ways to schedule.
    // For now, ui_init contains the call to parse_menu_definition_file, which itself checks if SD is ready.

    // Create a label for time display (e.g., on the system layer or a base screen)
    // Ensure this happens after lv_init() and LVGL is ready
    // For simplicity, let's try adding to system layer if available,
    // otherwise, you'd add it to your main screen in ui_init
    setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
    tzset();
    lv_obj_t* target_parent_for_time = lv_disp_get_layer_sys(NULL);
    if (!target_parent_for_time) {
        target_parent_for_time = lv_scr_act(); // Fallback to current screen if sys layer not used/available
    }

    if (target_parent_for_time) {
        time_display_label = lv_label_create(target_parent_for_time, NULL);
        // Style it using definitions from stuff.h
        lv_obj_set_style_local_text_font(time_display_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, TERMINAL_FONT);
        lv_obj_set_style_local_text_color(time_display_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, TERMINAL_COLOR_FOREGROUND_ALT); // Using white for time
        lv_obj_align(time_display_label, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0); // Position it
        lv_label_set_text(time_display_label, "Starting...");
        lv_task_create(time_update_lv_task, 1000, LV_TASK_PRIO_LOW, NULL); // Update every second
    }

    ESP_LOGI(TAG_MAIN, "Creating UI init task.");
    // ui_init will call parse_menu_definition_file and then load the splash screen.
    // The splash screen callback will then load the first dynamic menu.
    lv_task_t* ui_init_task = lv_task_create(ui_init, 500, LV_TASK_PRIO_MID, NULL); // Delay ui_init slightly for sd_init
    lv_task_once(ui_init_task);


    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
