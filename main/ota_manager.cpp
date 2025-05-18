#include "ota_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_ota_ops.h"      // For esp_app_desc_t
#include "esp_app_format.h"   // For esp_app_desc_t
#include "esp_ghota.h"
#include "esp_event.h"        // Required for esp_event_handler_register
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>           // Required for memset and strncpy
#include "menu_functions.h"

static const char *TAG_OTA = "ota_manager";

// --- ESP-GHOTA Configuration ---
#define GITHUB_USER "guutz" // This will be used as orgname
#define GITHUB_REPO "SCP_MTF_DEI_PDA" // This will be used as reponame
#define GITHUB_FILENAME "SCP_MTF_DEI_PDA.bin" // The exact name of the asset in your GitHub release
// Optional: Define a specific tag if you don't want the latest release
// #define GITHUB_TAG "v1.0.0" // Note: ghota_config_t doesn't directly take a tag for manual checks,
                             // it usually gets the "latest" or relies on Kconfig.
                             // For specific versions, the library might need different handling or Kconfig.

// Retrieve GitHub token from NVS (returns true if found)
bool ota_manager_get_github_token(char *out_token, size_t max_len) {
    static bool nvs_initialized = false;
    if (!nvs_initialized) {
        esp_err_t nvs_ret = nvs_flash_init();
        if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }
        nvs_initialized = true;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("ota", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        size_t required_size = max_len;
        err = nvs_get_str(nvs_handle, "gh_token", out_token, &required_size);
        nvs_close(nvs_handle);
        return (err == ESP_OK && strlen(out_token) > 0);
    }
    return false;
}

OtaManager::OtaManager(const ProgressCallback& cb)
    : progress_cb_(cb) {}

OtaManager::~OtaManager() {
    if (ghota_client_) {
        ghota_free(ghota_client_);
        ghota_client_ = nullptr;
    }
    if (ota_task_handle_) {
        vTaskDelete(ota_task_handle_);
        ota_task_handle_ = nullptr;
    }
}

void OtaManager::start_update() {
    if (!ota_task_handle_) {
        xTaskCreate(&OtaManager::ota_task, "ota_task", 10240, this, 5, &ota_task_handle_);
    }
}

void OtaManager::ota_task(void* pvParameter) {
    static_cast<OtaManager*>(pvParameter)->run_ota_task();
}

void OtaManager::run_ota_task() {
    ghota_config_t ghconfig;
    memset(&ghconfig, 0, sizeof(ghota_config_t));
    strncpy(ghconfig.filenamematch, GITHUB_FILENAME, sizeof(ghconfig.filenamematch) - 1);
    ghconfig.filenamematch[sizeof(ghconfig.filenamematch) - 1] = '\0';
    ghconfig.orgname = (char*)GITHUB_USER;
    ghconfig.reponame = (char*)GITHUB_REPO;
    ghconfig.updateInterval = 0;
    ghota_client_ = ghota_init(&ghconfig);
    if (!ghota_client_) {
        if (progress_cb_) progress_cb_("OTA client init failed");
        vTaskDelete(nullptr);
        return;
    }
    esp_event_handler_register(GHOTA_EVENTS, ESP_EVENT_ANY_ID, &OtaManager::ghota_event_callback, this);
    char github_token[128] = {0};
    if (ota_manager_get_github_token(github_token, sizeof(github_token))) {
        ghota_set_auth(ghota_client_, GITHUB_USER, github_token);
    }
    esp_err_t check_result = ghota_check(ghota_client_);
    if (check_result == ESP_OK) {
        semver_t *current_v = ghota_get_current_version(ghota_client_);
        semver_t *latest_v = ghota_get_latest_version(ghota_client_);
        if (current_v && latest_v) {
            if (semver_gt(*latest_v, *current_v)) {
                if (progress_cb_) progress_cb_("Update available. Starting update...");
                ghota_update(ghota_client_);
            } else {
                char msg[96];
                snprintf(msg, sizeof(msg), "Firmware is already up to date.\nCurrent: %d.%d.%d, Latest: %d.%d.%d",
                    current_v->major, current_v->minor, current_v->patch,
                    latest_v->major, latest_v->minor, latest_v->patch);
                if (progress_cb_) progress_cb_(msg);
                esp_event_post(GHOTA_EVENTS, GHOTA_EVENT_NOUPDATE_AVAILABLE, NULL, 0, portMAX_DELAY);
            }
        } else {
            if (progress_cb_) progress_cb_("Could not determine firmware version.");
        }
        if (current_v) semver_free(current_v);
        if (latest_v) semver_free(latest_v);
    } else {
        if (progress_cb_) progress_cb_("OTA check failed");
    }
    esp_event_handler_unregister(GHOTA_EVENTS, ESP_EVENT_ANY_ID, &OtaManager::ghota_event_callback);
    vTaskDelete(nullptr);
    delete this;
}

void OtaManager::ghota_event_callback(void* handler_args, esp_event_base_t, int32_t id, void* event_data) {
    auto* self = static_cast<OtaManager*>(handler_args);
    if (self) self->handle_event(id, event_data);
}

void OtaManager::handle_event(int32_t id, void* event_data) {
    char buf[128];
    switch (id) {
        case GHOTA_EVENT_FINISH_UPDATE:
            snprintf(buf, sizeof(buf), "OTA Update Finished! Rebooting...");
            if (progress_cb_) progress_cb_(buf);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
            break;
        case GHOTA_EVENT_UPDATE_FAILED:
            snprintf(buf, sizeof(buf), "OTA Update Failed.");
            if (progress_cb_) progress_cb_(buf);
            break;
        case GHOTA_EVENT_NOUPDATE_AVAILABLE:
            snprintf(buf, sizeof(buf), "No update available.\nFirmware is up to date.");
            if (progress_cb_) progress_cb_(buf);
            break;
        case GHOTA_EVENT_FIRMWARE_UPDATE_PROGRESS:
            if (event_data) {
                int progress = *((int*)event_data);
                snprintf(buf, sizeof(buf), "Downloading update... %d%%", progress);
                if (progress_cb_) progress_cb_(buf);
            }
            break;
        default:
            break;
    }
}
