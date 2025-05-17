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

// OTA Event Handler
static void ghota_event_callback(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    ESP_LOGI(TAG_OTA, "OTA Event: %s", ghota_get_event_str((ghota_event_e)id));
    ghota_client_handle_t *client = (ghota_client_handle_t *)handler_args;

    char buf[128];
    switch (id) {
        case GHOTA_EVENT_FINISH_UPDATE:
            snprintf(buf, sizeof(buf), "OTA Update Finished! Rebooting...");
            ESP_LOGI(TAG_OTA, "OTA Update Finished successfully. Rebooting device.");
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Give UI time to update
            esp_restart();
            break;
        case GHOTA_EVENT_UPDATE_FAILED:
            snprintf(buf, sizeof(buf), "OTA Update Failed.");
            ESP_LOGE(TAG_OTA, "OTA Update Failed.");
            break;
        case GHOTA_EVENT_NOUPDATE_AVAILABLE:
            snprintf(buf, sizeof(buf), "No update available.\nFirmware is up to date.");
            ESP_LOGI(TAG_OTA, "No update available or firmware is already up to date.");
            break;
        case GHOTA_EVENT_FIRMWARE_UPDATE_PROGRESS:
            if (event_data) {
                int progress = *((int*)event_data);
                snprintf(buf, sizeof(buf), "Downloading update... %d%%", progress);
                ESP_LOGI(TAG_OTA, "Firmware Update Progress: %d%%", progress);
            }
            break;
        default:
            break;
    }
}

// OTA Task
static void ota_task(void *pvParameter) {
    ESP_LOGI(TAG_OTA, "Starting OTA task...");

    const esp_app_desc_t *current_app_info = esp_ota_get_app_description();
    ESP_LOGI(TAG_OTA, "Current firmware version from app_desc: %s", current_app_info->version);

    ghota_config_t ghconfig;
    memset(&ghconfig, 0, sizeof(ghota_config_t)); // Initialize the struct to zeros/NULLs

    // Populate the char arrays
    // Ensure GITHUB_FILENAME length does not exceed CONFIG_MAX_FILENAME_LEN - 1
    strncpy(ghconfig.filenamematch, GITHUB_FILENAME, sizeof(ghconfig.filenamematch) - 1);
    ghconfig.filenamematch[sizeof(ghconfig.filenamematch) - 1] = '\\0';

    ghconfig.orgname = (char*)GITHUB_USER; // GITHUB_USER is a #define string literal
    ghconfig.reponame = (char*)GITHUB_REPO; // GITHUB_REPO is a #define string literal
    ghconfig.updateInterval = 0;      

    ghota_client_handle_t *ghota_client = ghota_init(&ghconfig);

    if (ghota_client == NULL) {
        ESP_LOGE(TAG_OTA, "ghota_client_init failed. Check Kconfig for GHOTA if defaults are used, or config struct.");
        vTaskDelete(NULL);
        return;
    }

    // Register event handler to get progress and status updates
    // Pass ghota_client as handler_args so callback can access it if needed, though not strictly necessary for basic reboot.
    if (esp_event_handler_register(GHOTA_EVENTS, ESP_EVENT_ANY_ID, &ghota_event_callback, ghota_client) != ESP_OK) {
        ESP_LOGE(TAG_OTA, "Failed to register OTA event handler.");
        ghota_free(ghota_client);
        vTaskDelete(NULL);
        return;
    }
    
    // For private repositories, you would set authentication:
    // const char* github_token = "YOUR_GITHUB_PAT_TOKEN"; // Store securely!
    // if (github_token && strlen(github_token) > 0) {
    //    if (ghota_set_auth(ghota_client, GITHUB_USER, github_token) != ESP_OK) {
    //        ESP_LOGW(TAG_OTA, "Failed to set ghota authentication.");
    //    }
    // }

    // After ghota_client is initialized, before ghota_check:
    char github_token[128] = {0};
    if (ota_manager_get_github_token(github_token, sizeof(github_token))) {
        if (ghota_set_auth(ghota_client, GITHUB_USER, github_token) != ESP_OK) {
            ESP_LOGW(TAG_OTA, "Failed to set ghota authentication.");
        } else {
            ESP_LOGI(TAG_OTA, "Loaded GitHub token from NVS and set for ghota.");
        }
    } else {
        ESP_LOGI(TAG_OTA, "No GitHub token found in NVS, proceeding without auth.");
    }

    ESP_LOGI(TAG_OTA, "Checking for firmware updates from %s/%s, asset: %s", 
             ghconfig.orgname, ghconfig.reponame, ghconfig.filenamematch);

    esp_err_t check_result = ghota_check(ghota_client);

    if (check_result == ESP_OK) {
        ESP_LOGI(TAG_OTA, "ghota_check indicates an update might be available.");
        
        semver_t *current_v = ghota_get_current_version(ghota_client); // Gets version from app_desc via ghota
        semver_t *latest_v = ghota_get_latest_version(ghota_client);   // Gets version from GitHub release

        if (current_v) {
            ESP_LOGI(TAG_OTA, "Parsed current version: %d.%d.%d", current_v->major, current_v->minor, current_v->patch);
        } else {
            ESP_LOGE(TAG_OTA, "Failed to parse current firmware version via ghota.");
        }

        if (latest_v) {
            ESP_LOGI(TAG_OTA, "Parsed latest GitHub version: %d.%d.%d", latest_v->major, latest_v->minor, latest_v->patch);
        } else {
            ESP_LOGE(TAG_OTA, "Failed to parse latest GitHub version via ghota.");
        }

        if (current_v && latest_v) {
            if (semver_gt(*latest_v, *current_v)) {
                ESP_LOGI(TAG_OTA, "Newer version found on GitHub. Starting update process...");
                esp_err_t update_result = ghota_update(ghota_client);
                // If ghota_update is successful, it usually triggers a reboot.
                // The GHOTA_EVENT_FINISH_UPDATE in the callback will handle the reboot.
                // If it returns, it means an error occurred or reboot is deferred.
                if (update_result != ESP_OK) {
                    ESP_LOGE(TAG_OTA, "ghota_update failed with error: 0x%x (%s)", update_result, esp_err_to_name(update_result));
                } else {
                    ESP_LOGI(TAG_OTA, "ghota_update call returned ESP_OK. Reboot should be handled by event callback.");
                }
            } else {
                ESP_LOGI(TAG_OTA, "Current firmware is up to date or newer. No update performed.");
                // Post NOUPDATE_AVAILABLE if not already posted by ghota_check implicitly
                 esp_event_post(GHOTA_EVENTS, GHOTA_EVENT_NOUPDATE_AVAILABLE, NULL, 0, portMAX_DELAY);
            }
        } else {
            ESP_LOGE(TAG_OTA, "Could not compare versions. One or both versions are unavailable.");
        }

        if (current_v) semver_free(current_v);
        if (latest_v) semver_free(latest_v);

    } else if (check_result == ESP_FAIL) { 
        // ESP_FAIL from ghota_check often means no update is available or a non-critical error.
        // The GHOTA_EVENT_NOUPDATE_AVAILABLE event should have been posted by ghota_check.
        ESP_LOGI(TAG_OTA, "ghota_check reported ESP_FAIL. Likely no update available or minor issue. Check logs from ghota library.");
    } else {
        ESP_LOGE(TAG_OTA, "ghota_check failed with error: 0x%x (%s)", check_result, esp_err_to_name(check_result));
    }
    
    // Cleanup: Unregister event handler and free client
    // This part will only be reached if the update didn't happen, failed before reboot, or if no update was needed.
    esp_event_handler_unregister(GHOTA_EVENTS, ESP_EVENT_ANY_ID, &ghota_event_callback);
    ghota_free(ghota_client);
    
    ESP_LOGI(TAG_OTA, "OTA task finished processing.");
    vTaskDelete(NULL);
}

void trigger_ota_update() {
    ESP_LOGI(TAG_OTA, "OTA update triggered. Creating ota_task.");
    // Stack size for OTA can be significant due to HTTPS and JSON parsing.
    // 8192 might be okay, but monitor for stack overflows. 10240 or 12288 could be safer.
    xTaskCreate(&ota_task, "ota_task", 10240, NULL, 5, NULL); 
}
