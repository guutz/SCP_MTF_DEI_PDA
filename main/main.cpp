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

static const char *TAG_MAIN = "main"; // Changed TAG
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void lvglTask(void *pvParameter);
SemaphoreHandle_t xGuiSemaphore;


extern "C" void app_main(void) {
    nvs_flash_init();
    xTaskCreatePinnedToCore(lvglTask, "gui", 1024 * 16, NULL, 5, NULL, 1);
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
