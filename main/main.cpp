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
#include "laser_tag.h"
#include "EspMeshHandler.h"
#include "xasin/audio/AudioTX.h"
#include "xasin/audio/ByteCassette.h" // For Xasin::Audio::TX
#include "xasin/BatteryManager.h"

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
#include "esp_pm.h"

// LVGL Headers
#include "lvgl.h"
#include "lvgl_helpers.h"

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
static void audio_core_processing_task(void *args); // Forward declaration for audio task

SemaphoreHandle_t xGuiSemaphore;
Xasin::Communication::EspMeshHandler g_mesh_handler;
Xasin::Audio::TX audioManager; // Definition of the global audioManager
Housekeeping::BatteryManager g_battery_manager;

mcp23008_t main_gpio_extender = {
    .port = I2C_NUM_0,
    .address = 0x20,
    .current = 0
};


extern "C" void app_main(void) {
    nvs_flash_init();

    // tcpip_adapter_init(); // Removed: Handled by EspMeshHandler or underlying ESP-IDF mesh init
    // esp_event_loop_init(event_handler, 0); // Removed: Event handling is part of EspMeshHandler

    ESP_LOGI(TAG_MAIN, "Creating Init Task.");
    xTaskCreatePinnedToCore(initTask, "init", 4096, NULL, 10, NULL, 0);
}

void power_config() {
	esp_pm_config_esp32_t pCFG;
	pCFG.max_freq_mhz = 240;
	pCFG.min_freq_mhz = 240;
	pCFG.light_sleep_enable = false;
	esp_pm_configure(&pCFG);
}


static void initTask(void *pvParameter) {
    (void)pvParameter;

    power_config();

    xTaskCreatePinnedToCore(wifi_init_task, "wifi_init", 4096*2, NULL, 5, NULL, 0);

    lvgl_full_init();

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing I2C manager.");
    ESP_ERROR_CHECK(i2c_manager_init(I2C_NUM_0));
    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing main MCP23008 wrapper.");
    ESP_ERROR_CHECK(mcp23008_wrapper_init(&main_gpio_extender));

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing main mux switch (CD4053B) paths.");
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S2_PATH_B0_ACCESSORY_A);
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S1_PATH_A0_JOYSTICK_AXIS1);
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S3_PATH_C0_JOYSTICK_AXIS2);

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing Joystick and ADC.");
    ESP_ERROR_CHECK(joystick_init());

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing LVGL Joystick Input.");
    lvgl_joystick_input_init(&main_gpio_extender); // Add joystick LVGL init here

    ESP_LOGI(TAG_MAIN, "[InitTask] Initializing Audio System.");
    TaskHandle_t audioProcessingTaskHandle = nullptr; // Local handle for init
    // xTaskCreate(audio_core_processing_task, "AudioLargeStack", 32768, nullptr, 5, &audioProcessingTaskHandle);
    // audioManager.init(audioProcessingTaskHandle); 
    // audioManager.volume_mod = 160; 
    ESP_LOGI(TAG_MAIN, "[InitTask] Audio system initialized.");

    // Schedule SD card and UI init as LVGL tasks
    ESP_LOGI(TAG_MAIN, "[InitTask] Scheduling SD card initialization as LVGL task.");
    lv_task_t* sd_init_task = lv_task_create(sd_init, 0, LV_TASK_PRIO_MID, NULL);
    lv_task_once(sd_init_task);

    ESP_LOGI(TAG_MAIN, "[InitTask] Scheduling UI init as LVGL task.");
    lv_task_t* ui_init_task = lv_task_create(ui_init, 500, LV_TASK_PRIO_MID, NULL);
    lv_task_once(ui_init_task);

    ESP_LOGI(TAG_MAIN, "[InitTask] Starting GUI tasks.");
    // xTaskCreatePinnedToCore(peripheralsTask, "peripherals", 1024 * 2, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(lvglTask, "gui", 1024 * 8, NULL, 5, NULL, 1);

    ESP_LOGI(TAG_MAIN, "[InitTask] Initialization complete. Deleting self.");
    vTaskDelete(NULL);
}

// Audio processing task
static void audio_core_processing_task(void *args) {
	while(true) {
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		audioManager.largestack_process();
	}
}


static void wifi_init_task(void *pvParameter) {
    (void)pvParameter;
    // ESP_LOGI(TAG_MAIN, "Wi-Fi/Mesh init task started, waiting for 3 seconds...");
    // vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for 3 seconds

    ESP_LOGI(TAG_MAIN, "Configuring and starting ESP-MESH handler...");
    
    Xasin::Communication::EspMeshHandler::EspMeshHandlerConfig mesh_config;
    mesh_config.wifi_ssid = WIFI_STATION_SSID;
    mesh_config.wifi_password = WIFI_STATION_PASSWD;

    // Populate ESP-MESH and MQTT specific configurations
    mesh_config.mesh_password = MESH_PASSWORD_STR;
    mesh_config.mesh_channel = MESH_CHANNEL; // This is uint8_t
    mesh_config.mqtt_broker_uri = MQTT_BROKER_URI_STR;

    g_mesh_handler.start(&mesh_config);

    ESP_LOGI(TAG_MAIN, "Wi-Fi/Mesh init task finished, deleting self.");
    vTaskDelete(NULL);
}


static void peripheralsTask(void *pvParameter) {
    (void) pvParameter;

    JoystickState_t current_joystick_state;
    
    while (1) {
        // Read Joystick State
        esp_err_t joy_ret = joystick_read_state(&main_gpio_extender, &current_joystick_state);
        if (joy_ret == ESP_OK) {
            // ESP_LOGI(TAG_MAIN, "Joystick: X=%d, Y=%d, Btn=%s", 
            //          current_joystick_state.x, 
            //          current_joystick_state.y, 
            //          current_joystick_state.button_pressed ? "Pressed" : "Released");

            // if (current_joystick_state.button_pressed) {
            //     mcp23008_wrapper_write_pin(&main_gpio_extender, MCP_PIN_ETH_LED_1, true);
            //     mcp23008_wrapper_write_pin(&main_gpio_extender, MCP_PIN_ETH_LED_2, false);
            // }
            // else {
            //     mcp23008_wrapper_write_pin(&main_gpio_extender, MCP_PIN_ETH_LED_1, false);
            //     mcp23008_wrapper_write_pin(&main_gpio_extender, MCP_PIN_ETH_LED_2, true);
            // }
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
