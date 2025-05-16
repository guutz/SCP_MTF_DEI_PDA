// Project Specific Headers
#include "cd4053b_wrapper.h"
#include "i2c_manager.h"
#include "joystick.h"
#include "mcp23008.h"
#include "mcp23008_wrapper.h"
#include "menu_structures.h"
#include "ota_manager.h"
#include "sd_manager.h"
#include "setup.h"

#include "ui_manager.h"

// C Standard Library
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ESP-IDF Headers
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

// LVGL Headers
#include "lvgl.h"
#include "lvgl_helpers.h"

// Other Libraries
#include <xnm/net_helpers.h> // Assuming this is a third-party or external library

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
static void lvglTask(void *pvParameter);
static void wifi_init_task(void *pvParameter); 
static void initTask(void *pvParameter);
static void peripheralsTask(void *pvParameter);
esp_err_t event_handler(void *context, system_event_t *event);
volatile bool g_wifi_intentional_stop = false;
SemaphoreHandle_t xGuiSemaphore;

mcp23008_t mcp23008_device = {
    .port = I2C_NUM_0,
    .address = 0x20,
    .current = 0
};


extern "C" void app_main(void) {
    nvs_flash_init();

    tcpip_adapter_init();
    esp_event_loop_init(event_handler, 0);

    ESP_LOGI(TAG_MAIN, "Creating Init Task.");
    xTaskCreatePinnedToCore(initTask, "init", 4096, NULL, 10, NULL, 0);
}


static void initTask(void *pvParameter) {
    (void)pvParameter;

    lvgl_full_init();

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing I2C manager.");
    ESP_ERROR_CHECK(i2c_manager_init(I2C_NUM_0));
    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing MCP23008 wrapper.");
    ESP_ERROR_CHECK(mcp23008_wrapper_init(&mcp23008_device));

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing CD4053B paths.");
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S2_PATH_B0_ACCESSORY_A);
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S1_PATH_A0_JOYSTICK_AXIS1);
    cd4053b_select_named_path(&mcp23008_device, CD4053B_S3_PATH_C0_JOYSTICK_AXIS2);

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing Joystick and ADC.");
    ESP_ERROR_CHECK(joystick_init());

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing LVGL Joystick Input.");
    lvgl_joystick_input_init(&mcp23008_device); // Add joystick LVGL init here

    // Schedule SD card and UI init as LVGL tasks
    ESP_LOGI(TAG_MAIN, "[InitTask] Scheduling SD card initialization as LVGL task.");
    lv_task_t* sd_init_task = lv_task_create(sd_init, 0, LV_TASK_PRIO_MID, NULL);
    lv_task_once(sd_init_task);

    ESP_LOGI(TAG_MAIN, "[InitTask] Scheduling UI init as LVGL task.");
    lv_task_t* ui_init_task = lv_task_create(ui_init, 500, LV_TASK_PRIO_MID, NULL);
    lv_task_once(ui_init_task);

    ESP_LOGI(TAG_MAIN, "[InitTask] Starting Wi-Fi, peripherals, and GUI tasks.");
    xTaskCreatePinnedToCore(wifi_init_task, "wifi_init", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(peripheralsTask, "peripherals", 1024 * 2, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(lvglTask, "gui", 1024 * 8, NULL, 5, NULL, 1);

    ESP_LOGI(TAG_MAIN, "[InitTask] Initialization complete. Deleting self.");
    vTaskDelete(NULL);
}


static void wifi_init_task(void *pvParameter) {
    (void)pvParameter;
    ESP_LOGI(TAG_MAIN, "Wi-Fi init task started, waiting for 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for 3 seconds

    ESP_LOGI(TAG_MAIN, "Initializing Wi-Fi...");
    // XNM::NetHelpers::init_global_r3_ca(); 
    Xasin::MQTT::Handler::start_wifi(WIFI_STATION_SSID, WIFI_STATION_PASSWD);

    ESP_LOGI(TAG_MAIN, "Wi-Fi init task finished, deleting self.");
    vTaskDelete(NULL);
}


static void peripheralsTask(void *pvParameter) {
    (void) pvParameter;

    JoystickState_t current_joystick_state;
    
    while (1) {
        // Read Joystick State
        esp_err_t joy_ret = joystick_read_state(&mcp23008_device, &current_joystick_state);
        if (joy_ret == ESP_OK) {
            // ESP_LOGI(TAG_MAIN, "Joystick: X=%d, Y=%d, Btn=%s", 
            //          current_joystick_state.x, 
            //          current_joystick_state.y, 
            //          current_joystick_state.button_pressed ? "Pressed" : "Released");

            if (current_joystick_state.button_pressed) {
                mcp23008_wrapper_write_pin(&mcp23008_device, MCP_PIN_ETH_LED_1, true);
                mcp23008_wrapper_write_pin(&mcp23008_device, MCP_PIN_ETH_LED_2, false);
            }
            else {
                mcp23008_wrapper_write_pin(&mcp23008_device, MCP_PIN_ETH_LED_1, false);
                mcp23008_wrapper_write_pin(&mcp23008_device, MCP_PIN_ETH_LED_2, true);
            }
        } else {
            ESP_LOGE(TAG_MAIN, "Failed to read joystick state: %s", esp_err_to_name(joy_ret));
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Read every 100 milliseconds
    }
}


static void lvglTask(void *pvParameter) {
    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    // Only keep the loop here
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    vTaskDelete(NULL);
}

esp_err_t event_handler(void *context, system_event_t *event) {
    // If Wi-Fi was stopped intentionally, don't try to reconnect on disconnect event
    if (event->event_id == SYSTEM_EVENT_STA_DISCONNECTED && g_wifi_intentional_stop) {
        ESP_LOGI(TAG_MAIN, "Wi-Fi intentionally disconnected, skipping reconnect attempt.");
    } else {
        Xasin::MQTT::Handler::try_wifi_reconnect(event);
    }

	// mqtt.wifi_handler(event);
    // Example: Listen for MQTT messages to trigger OTA
    // This is a simplified example. You'd integrate this with your Xasin::MQTT::Handler
    // and parse the message topic/payload appropriately.
    // For instance, in your MQTT message callback:
    // if (strcmp(topic, "yourdevice/ota/update") == 0) {
    //     trigger_ota_update();
    // }

	return ESP_OK;
}