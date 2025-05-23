// Project Specific Headers
#include "cd4053b_wrapper.h"
#include "i2c_manager.h"
#include "joystick.h"
#include "mcp23008.h"
#include "mcp23008_wrapper.h"
#include "menu_structures.h"
#include "ota_manager.h"
#include "sd_manager.h"
#include "menu_log.h"
#include "setup.h"
#include "xasin/mqtt/Handler.h"
#include "lzrtag/sounds.h"

#include "ui_manager.h"
#include "laser_tag.h"
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
#include "esp_tls.h"

// LVGL Headers
#include "lvgl.h"
#include "lvgl_helpers.h"

// Project Specific Headers for PersistentState
#include "persistent_state.h" 

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
static void power_config();
static void start_wifi();
static esp_err_t event_handler(void *context, system_event_t *event);
static void sd_init_and_persistent_state(lv_task_t *task); // New function for SD and PersistentState init

std::string g_device_id;

SemaphoreHandle_t xGuiSemaphore;
Xasin::MQTT::Handler mqtt = Xasin::MQTT::Handler();
Housekeeping::BatteryManager g_battery_manager;
LZR::Sounds::SoundManager g_sound_manager(&mqtt);

mcp23008_t main_gpio_extender = {
    .port = I2C_NUM_0,
    .address = 0x20,
    .current = 0
};

extern const uint8_t ca_cert_bundle_start[] asm("_binary_hivemq_pem_start");
extern const uint8_t ca_cert_bundle_end[] asm("_binary_hivemq_pem_end");

extern "C" void app_main(void) {
    nvs_flash_init();
    menu_log_add(TAG_MAIN, "[InitTask] Starting main application.");
    power_config();

    start_wifi();

    lvgl_full_init();

    g_sound_manager.init();

    menu_log_add(TAG_MAIN, "[InitTask] Initializing I2C manager.");
    ESP_ERROR_CHECK(i2c_manager_init(I2C_NUM_0));
    menu_log_add(TAG_MAIN, "[InitTask] Initializing main MCP23008 wrapper.");
    ESP_ERROR_CHECK(mcp23008_wrapper_init(&main_gpio_extender));

    menu_log_add(TAG_MAIN, "[InitTask] Initializing main mux switch (CD4053B) paths.");
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S2_PATH_B0_ACCESSORY_A);
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S1_PATH_A0_JOYSTICK_AXIS1);
    cd4053b_select_main_path(&main_gpio_extender, MAIN_MUX_S3_PATH_C0_JOYSTICK_AXIS2);

    menu_log_add(TAG_MAIN, "[InitTask] Initializing Joystick and ADC.");
    ESP_ERROR_CHECK(joystick_init());

    menu_log_add(TAG_MAIN, "[InitTask] Initializing LVGL Joystick Input.");
    lvgl_joystick_input_init(&main_gpio_extender); // Add joystick LVGL init here

    menu_log_add(TAG_MAIN, "[InitTask] Initializing Audio System.");
    g_sound_manager.init(); // Initialize the sound manager
    menu_log_add(TAG_MAIN, "[InitTask] Audio system initialized.");

    // Schedule SD card and UI init as LVGL tasks
    menu_log_add(TAG_MAIN, "[InitTask] Scheduling SD card initialization as LVGL task.");
    lv_task_t* sd_init_task = lv_task_create(sd_init_and_persistent_state, 0, LV_TASK_PRIO_MID, NULL); // Renamed and using new function
    lv_task_once(sd_init_task);

    menu_log_add(TAG_MAIN, "[InitTask] Scheduling UI init as LVGL task.");
    lv_task_t* ui_init_task = lv_task_create(ui_init, 500, LV_TASK_PRIO_MID, NULL);
    lv_task_once(ui_init_task);

    menu_log_add(TAG_MAIN, "[InitTask] Starting GUI tasks.");
    xTaskCreatePinnedToCore(lvglTask, "gui", 8192, NULL, 5, NULL, 1);

    menu_log_add(TAG_MAIN, "[InitTask] Initialization complete.");
}

static void power_config() {
	esp_pm_config_esp32_t pCFG;
	pCFG.max_freq_mhz = 240;
	pCFG.min_freq_mhz = 240;
	pCFG.light_sleep_enable = false;
	esp_pm_configure(&pCFG);
}

static void start_wifi() {
    tcpip_adapter_init();
    esp_event_loop_init(event_handler, 0);
    // esp_tls_set_global_ca_store(ca_cert_bundle_start, ca_cert_bundle_end - ca_cert_bundle_start);
    Xasin::MQTT::Handler::start_wifi(WIFI_STATION_SSID, WIFI_STATION_PASSWD);
}

// New combined SD init and PersistentState init task
static void sd_init_and_persistent_state(lv_task_t *task) {
    menu_log_add(TAG_MAIN, "Starting SD card and Persistent State initialization...");
    sd_init(task); // Initialize SD card first
    
    // Now that SD is up, initialize persistent state
    if (PersistentState::initialize_default_persistent_state_if_needed()) {
        menu_log_add(TAG_MAIN, "Persistent state initialized successfully.");
        mqtt.set_base_topic_from_device_id(g_device_id);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize persistent state!");
        // Handle error appropriately - perhaps a critical error screen or retry?
    }
    menu_log_add(TAG_MAIN, "SD card and Persistent State initialization complete.");
}

// // Audio processing task
// static void audio_core_processing_task(void *args) {
// 	while(true) {
// 		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
// 		audioManager.largestack_process();
// 	}
// }



static esp_err_t event_handler(void *context, system_event_t *event) {
	Xasin::MQTT::Handler::try_wifi_reconnect(event);

	mqtt.wifi_handler(event);

	return ESP_OK;
}

static void lvglTask(void *pvParameter) {
    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    vTaskDelete(NULL);
}
