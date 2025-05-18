#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H


#include "lvgl.h"
#include <functional>
#include "esp_ghota.h" // for ghota_client_handle_t

class OtaManager {
public:
    using ProgressCallback = std::function<void(const char*)>;

    OtaManager(const ProgressCallback& cb);
    ~OtaManager();
    void start_update();

private:
    static void ghota_event_callback(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
    void handle_event(int32_t id, void* event_data);
    static void ota_task(void* pvParameter);
    void run_ota_task();

    ProgressCallback progress_cb_;
    TaskHandle_t ota_task_handle_ = nullptr;
    ghota_client_handle_t* ghota_client_ = nullptr;
};


#endif // OTA_MANAGER_H
