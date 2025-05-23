#include "persistent_state.h"
#include "sd_raw_access.h"
#include "setup.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // Added for mutex
#include "freertos/semphr.h" // Added for mutex

namespace PersistentState {
    static const char* TAG = "persistent_state";
    static const char* menu_state_path = "DEI/menu_state.txt";

    static MenuPersistentState g_current_menu_state;
    static bool g_menu_state_loaded = false;
    static SemaphoreHandle_t g_state_mutex = NULL; // Mutex for g_current_menu_state

    // Getter for the global menu state
    const MenuPersistentState& get_current_menu_state() {
        // Ensure state is loaded before returning. This might be redundant if ensure_state_loaded()
        // is called at all other entry points, but good for safety.
        // Consider if ensure_state_loaded() should be called here or if it's guaranteed by initialization.
        // For now, assume it's loaded by initialize_default_persistent_state_if_needed() at startup.
        // If direct access without prior initialization is possible, uncommenting ensure_state_loaded()
        // or a similar check would be necessary, along with mutex protection for the read.
        
        // if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) { // If direct read needs protection
        //    if (!g_menu_state_loaded) { ESP_LOGW(TAG, \"Accessing g_current_menu_state before it is loaded!\"); }
        //    // Create a copy to return if the reference could become invalid, though const& should be safe for reads
        //    xSemaphoreGive(g_state_mutex);
        // }
        return g_current_menu_state; // Direct return, assuming loaded and mutex handled by callers or init
    }

    // Function to create the mutex
    static void ensure_mutex_created() {
        if (g_state_mutex == NULL) {
            g_state_mutex = xSemaphoreCreateMutex();
            if (g_state_mutex == NULL) {
                ESP_LOGE(TAG, "Failed to create persistent state mutex!");
            }
        }
    }

    // Helper to trim whitespace from a string (from original file)
    std::string trim_string(const std::string& str) {
        const std::string whitespace = " \\t\\n\\r\\f\\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    // Helper to join a set/vector of strings into a comma-separated string
    template<typename T>
    static std::string join_string_collection(const T& collection, char delimiter = ',') {
        std::string out;
        for (auto it = collection.begin(); it != collection.end(); ++it) {
            if (it != collection.begin()) {
                out += delimiter;
            }
            out += *it;
        }
        return out;
    }

    // Helper to split a comma-separated string into a vector of strings
    static std::vector<std::string> split_string_to_vector(const std::string& s, char delimiter = ',') {
        std::vector<std::string> out;
        if (s.empty()) return out;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            out.push_back(trim_string(item));
        }
        return out;
    }
    
    // Helper to split a comma-separated string into a set of strings
    static std::set<std::string> split_string_to_set(const std::string& s, char delimiter = ',') {
        std::set<std::string> out;
        if (s.empty()) return out;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            out.insert(trim_string(item));
        }
        return out;
    }


    bool load_menu_persistent_state(MenuPersistentState& state) {
        // This function is called internally by initialize_default_persistent_state_if_needed
        // which is already mutex-protected. If called directly, ensure protection.
        FILE* fp = sd_raw_fopen(menu_state_path, "r");
        if (!fp) {
            ESP_LOGW(TAG, "Failed to open %s for reading.", menu_state_path);
            return false;
        }
        ESP_LOGI(TAG, "Loading menu persistent state from %s", menu_state_path);

        char line[256]; // Increased buffer size for potentially long comma-separated lists
        std::map<std::string, std::string> kv;
        while (fgets(line, sizeof(line), fp)) {
            std::string str_line(line);
            size_t eq_pos = str_line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = trim_string(str_line.substr(0, eq_pos));
                std::string val = trim_string(str_line.substr(eq_pos + 1));
                kv[key] = val;
            }
        }
        sd_raw_fclose(fp);

        state.playerInfo.device_id = kv.count("device_id") ? kv["device_id"] : "0";
        state.playerInfo.read_pages = kv.count("read_pages") ? split_string_to_vector(kv["read_pages"]) : std::vector<std::string>{};
        state.available_content_ids = kv.count("available_content_ids") ? split_string_to_set(kv["available_content_ids"]) : std::set<std::string>{};
        
        // g_menu_state_loaded is managed by the calling function (ensure_state_loaded)
        ESP_LOGI(TAG, "Menu persistent state loaded. Device ID: %s, Read Pages: %zu, Available Content: %zu",
                 state.playerInfo.device_id.c_str(), state.playerInfo.read_pages.size(), state.available_content_ids.size());
        return true;
    }

    bool save_menu_persistent_state(const MenuPersistentState& state) {
        // This function is called by other functions that should already have acquired the mutex.
        FILE* fp = sd_raw_fopen(menu_state_path, "w");
        if (!fp) {
            ESP_LOGE(TAG, "Failed to open %s for writing.", menu_state_path);
            return false;
        }
        ESP_LOGI(TAG, "Saving menu persistent state to %s", menu_state_path);

        fprintf(fp, "device_id=%s\\n", state.playerInfo.device_id.c_str());
        fprintf(fp, "read_pages=%s\\n", join_string_collection(state.playerInfo.read_pages).c_str());
        fprintf(fp, "available_content_ids=%s\\n", join_string_collection(state.available_content_ids).c_str());
        
        sd_raw_fclose(fp);
        ESP_LOGI(TAG, "Menu persistent state saved.");
        return true;
    }

    bool initialize_default_persistent_state_if_needed() {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return false; // Mutex creation failed

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            if (g_menu_state_loaded) { // Already loaded by a previous call
                xSemaphoreGive(g_state_mutex);
                return true;
            }

            FILE* fp = sd_raw_fopen(menu_state_path, "r");
            if (fp) {
                sd_raw_fclose(fp);
                ESP_LOGI(TAG, "%s already exists. Loading existing state.", menu_state_path);
                if (load_menu_persistent_state(g_current_menu_state)) {
                    g_menu_state_loaded = true;
                    g_device_id = g_current_menu_state.playerInfo.device_id; // Set global device ID from loaded state
                    xSemaphoreGive(g_state_mutex);
                    return true;
                } else {
                    ESP_LOGE(TAG, "Failed to load existing persistent state from %s", menu_state_path);
                    // Proceed to create default as a fallback
                }
            }

            ESP_LOGI(TAG, "%s does not exist or failed to load. Creating default state file.", menu_state_path);
            MenuPersistentState default_state;
            default_state.playerInfo.device_id = "0";

            if (!save_menu_persistent_state(default_state)) {
                ESP_LOGE(TAG, "Failed to write default persistent state to %s", menu_state_path);
                xSemaphoreGive(g_state_mutex);
                return false;
            }
            
            g_current_menu_state = default_state;
            g_menu_state_loaded = true;
            g_device_id = default_state.playerInfo.device_id; // Set global device ID from default state
            ESP_LOGI(TAG, "Default persistent state initialized and loaded.");
            xSemaphoreGive(g_state_mutex);
            return true;
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for initialization.");
            return false;
        }
    }

    // Helper to ensure state is loaded before use by other functions
    static bool ensure_state_loaded() {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return false;

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            if (!g_menu_state_loaded) {
                ESP_LOGI(TAG, "Global menu state not loaded. Attempting to initialize/load from ensure_state_loaded.");
                // Release mutex before calling initialize_default_persistent_state_if_needed to avoid deadlock
                // as it also takes the mutex.
                xSemaphoreGive(g_state_mutex);
                if (!initialize_default_persistent_state_if_needed()) {
                    ESP_LOGE(TAG, "Failed to initialize or load persistent state from ensure_state_loaded.");
                    return false; 
                }
                 // Re-take mutex to check g_menu_state_loaded again safely, though initialize should set it.
                if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdFALSE) {
                    ESP_LOGE(TAG, "Failed to re-take state mutex in ensure_state_loaded.");
                    return false;
                }
            }
            // If we are here, state should be loaded. Give back the mutex.
            xSemaphoreGive(g_state_mutex);
            return g_menu_state_loaded; // Return the status of g_menu_state_loaded protected by mutex
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex in ensure_state_loaded.");
            return false;
        }
    }

    void mark_content_as_available(const std::string& content_id) {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return;
        if (!ensure_state_loaded()) return;

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            bool updated = g_current_menu_state.available_content_ids.insert(content_id).second;
            if (updated) {
                ESP_LOGI(TAG, "Marking content as available: %s", content_id.c_str());
                save_menu_persistent_state(g_current_menu_state); // Assumes sd_raw_access mutex handles file part
            } else {
                ESP_LOGD(TAG, "Content already available: %s", content_id.c_str());
            }
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for mark_content_as_available.");
        }
    }

    void mark_content_as_unavailable(const std::string& content_id) {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return;
        if (!ensure_state_loaded()) return;

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            bool erased = g_current_menu_state.available_content_ids.erase(content_id) > 0;
            if (erased) {
                ESP_LOGI(TAG, "Marking content as unavailable: %s", content_id.c_str());
                save_menu_persistent_state(g_current_menu_state);
            } else {
                ESP_LOGD(TAG, "Content was not in available list: %s", content_id.c_str());
            }
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for mark_content_as_unavailable.");
        }
    }

    bool is_content_available(const std::string& content_id) {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return false;
        if (!ensure_state_loaded()) return false;

        bool available = false;
        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            available = g_current_menu_state.available_content_ids.count(content_id) > 0;
            ESP_LOGD(TAG, "Checking content availability for %s: %s", content_id.c_str(), available ? "true" : "false");
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for is_content_available.");
        }
        return available;
    }

    void mark_page_as_viewed(const std::string& page_id) {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return;
        if (!ensure_state_loaded()) return;

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            bool found = false;
            for (const auto& viewed_page : g_current_menu_state.playerInfo.read_pages) {
                if (viewed_page == page_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                g_current_menu_state.playerInfo.read_pages.push_back(page_id);
                ESP_LOGI(TAG, "Marking page as viewed: %s", page_id.c_str());
                save_menu_persistent_state(g_current_menu_state);
            } else {
                ESP_LOGD(TAG, "Page already marked as viewed: %s", page_id.c_str());
            }
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for mark_page_as_viewed.");
        }
    }

    bool has_page_been_viewed(const std::string& page_id) {
        ensure_mutex_created();
        if (g_state_mutex == NULL) return false;
        if (!ensure_state_loaded()) return false;

        bool viewed = false;
        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            for (const auto& viewed_page : g_current_menu_state.playerInfo.read_pages) {
                if (viewed_page == page_id) {
                    viewed = true;
                    break;
                }
            }
            ESP_LOGD(TAG, "Checking page viewed status for %s: %s", page_id.c_str(), viewed ? "true" : "false");
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for has_page_been_viewed.");
        }
        return viewed;
    }

    void set_device_id(const std::string& device_id) {
        if (!g_menu_state_loaded) {
            if (!initialize_default_persistent_state_if_needed()) {
                return;
            }
        }

        if (xSemaphoreTake(g_state_mutex, portMAX_DELAY) == pdTRUE) {
            bool updated = (g_current_menu_state.playerInfo.device_id != device_id);
            if (updated) {
                g_current_menu_state.playerInfo.device_id = device_id;
                ESP_LOGI(TAG, "Setting device ID to: %s", device_id.c_str());
                save_menu_persistent_state(g_current_menu_state);
            } else {
                ESP_LOGD(TAG, "Device ID already set to: %s", device_id.c_str());
            }
            xSemaphoreGive(g_state_mutex);
        } else {
            ESP_LOGE(TAG, "Failed to take state mutex for set_device_id.");
        }
    }

}
