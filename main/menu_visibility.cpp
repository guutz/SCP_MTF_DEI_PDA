#include "menu_visibility.h"
#include "menu_structures.h" // Assumed to declare G_MenuScreens (e.g., extern std::map<std::string, MenuItemScreen> G_MenuScreens;)
#include "persistent_state.h" // For MenuPersistentState and get_current_menu_state()
#include "EspMeshHandler.h"      // For Xasin::Communication::EspMeshHandler
#include "xasin/mqtt/Handler.h"  // For Xasin::MQTT::MQTT_Packet and potentially the type returned by g_mesh_handler.mqtt()

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <sstream>   // Required for std::istringstream
#include <iomanip>   // Required for std::get_time
#include "esp_log.h"

// Assuming G_MenuScreens is declared in menu_structures.h like:
// extern std::map<std::string, MenuItemScreen> G_MenuScreens;
// If it's not, it needs to be made accessible here.

static const char *TAG_VISIBILITY = "menu_visibility";

// Global state variables for MQTT states
static std::map<std::string, std::string> g_mqtt_state_vars;
// static SemaphoreHandle_t g_mqtt_state_mutex = xSemaphoreCreateMutex(); // Consider if needed

// External declaration for the global mesh handler.
extern Xasin::Communication::EspMeshHandler g_mesh_handler; // Defined in main.cpp or setup.cpp

// Helper function to parse time string (HH:MM format)
static bool parse_time_string(const std::string& time_str, std::tm& tm_out) {
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm_out, "%H:%M");
    return !ss.fail();
}

// Helper function to parse date string (YYYY-MM-DD format)
static bool parse_date_string(const std::string& date_str, std::tm& tm_out) {
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm_out, "%Y-%m-%d");
    // std::get_time might not fill all fields, esp. if format is only date/time
    tm_out.tm_isdst = -1; // Let mktime figure it out
    return !ss.fail();
}

// Helper function to parse datetime string (YYYY-MM-DD HH:MM format)
static bool parse_datetime_string(const std::string& datetime_str, std::tm& tm_out) {
    std::istringstream ss(datetime_str);
    ss >> std::get_time(&tm_out, "%Y-%m-%d %H:%M");
    tm_out.tm_isdst = -1; // Let mktime figure it out
    return !ss.fail();
}

// Helper function to get current time as tm struct
static std::tm get_current_tm() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);
    return now_tm;
}


void set_mqtt_state_variable(const std::string& topic, const std::string& value) {
    // if (xSemaphoreTake(g_mqtt_state_mutex, portMAX_DELAY) == pdTRUE) {
    ESP_LOGI(TAG_VISIBILITY, "Setting MQTT state: %s = %s", topic.c_str(), value.c_str());
    g_mqtt_state_vars[topic] = value;
    // xSemaphoreGive(g_mqtt_state_mutex);
    // TODO: Trigger UI refresh here if a menu is active and its visibility might change.
    // This could involve sending an event or calling a UI update function.
    // Example: ui_manager_request_refresh();
    // }
}

void setup_mqtt_visibility_handlers() {
    ESP_LOGI(TAG_VISIBILITY, "Setting up MQTT visibility handlers...");

    if (G_MenuScreens.empty()) {
        ESP_LOGI(TAG_VISIBILITY, "No menu screens found, skipping MQTT visibility handler setup.");
        return;
    }

    // Directly use g_mesh_handler.subscribe_to as per user feedback and EspMeshHandler.cpp structure
    // The EspMeshHandler::subscribe method takes a topic, a comm_message_callback_t, and qos.
    // However, our menu_visibility.cpp set_mqtt_state_variable needs the raw packet data.
    // The internal m_mqtt_handler_.subscribe_to takes a Xasin::MQTT::mqtt_callback.
    // We need to ensure g_mesh_handler exposes a way to subscribe with a Xasin::MQTT::mqtt_callback
    // or we adapt. 

    // For now, assuming g_mesh_handler has a method that aligns with Xasin::MQTT::Handler::subscribe_to
    // or that EspMeshHandler::subscribe can be adapted or a new method added to EspMeshHandler
    // if its current subscribe() signature is too high-level (with CommReceivedData).

    // Revisiting the EspMeshHandler.cpp, it seems its `subscribe` method is for a higher-level callback.
    // The `m_mqtt_handler_` is a `Xasin::MQTT::Handler`.
    // So, to use the `Xasin::MQTT::MQTT_Packet` directly, we would indeed need access to `m_mqtt_handler_`.
    // Let's assume for a moment that `g_mesh_handler.mqtt()` or similar exists to get the underlying `Xasin::MQTT::Handler`.
    // If not, EspMeshHandler needs a new method or modification.

    // Based on the user's prompt \"no, you can call g_mesh_handler.subscribe directly\",
    // this implies g_mesh_handler itself has a subscribe method suitable for our needs.
    // The provided EspMeshHandler.cpp shows `EspMeshHandler::subscribe` which wraps `m_mqtt_handler_.subscribe_to`
    // but it uses a `comm_message_callback_t` which passes `CommReceivedData`.
    // This is not what menu_visibility.cpp's lambda expects (it expects `const Xasin::MQTT::MQTT_Packet&`).

    // THERE IS A MISMATCH. 
    // menu_visibility.cpp wants to subscribe with a lambda that takes `const Xasin::MQTT::MQTT_Packet&`.
    // EspMeshHandler::subscribe expects a lambda that takes `CommReceivedData`.

    // To resolve: 
    // 1. Change menu_visibility.cpp lambda to accept CommReceivedData (if payload is sufficient).
    // 2. Modify EspMeshHandler to add a new subscribe method that accepts Xasin::MQTT::mqtt_callback.

    // Option 1: Adapt lambda in menu_visibility.cpp if CommReceivedData is enough.
    // CommReceivedData contains: topic, payload (vector<uint8_t>), source_id.
    // The current lambda uses packet.data (string) and packet.full_topic (string).
    // packet.data can be constructed from CommReceivedData.payload.
    // packet.full_topic is not directly in CommReceivedData, but CommReceivedData.topic is the filter.

    // Let's try to adapt the lambda in menu_visibility.cpp to use CommReceivedData.

    for (const auto& pair : G_MenuScreens) {
        const auto& screen = pair.second;
        for (const auto& item : screen.items) {
            if (item.visibility.type == VISIBILITY_MQTT_STATE && !item.visibility.state_variable.empty()) {
                std::string topic_to_subscribe = item.visibility.state_variable;

                ESP_LOGI(TAG_VISIBILITY, "Preparing to subscribe to MQTT topic for visibility (via g_mesh_handler.subscribe): %s", topic_to_subscribe.c_str());

                // Using g_mesh_handler.subscribe, which expects comm_message_callback_t
                // comm_message_callback_t is std::function<void(const CommReceivedData&)>)
                bool subscribed = g_mesh_handler.subscribe(
                    topic_to_subscribe,
                    [topic_to_subscribe](const Xasin::Communication::CommReceivedData& data) { // Lambda now takes CommReceivedData
                        // data.topic is the filter, data.payload is vector<uint8_t>
                        // The original lambda used packet.full_topic and packet.data (std::string)
                        // We need to convert payload to string for set_mqtt_state_variable
                        std::string payload_str(data.payload.begin(), data.payload.end());
                        
                        ESP_LOGI(TAG_VISIBILITY, "MQTT callback (via g_mesh_handler.subscribe) for visibility: Topic Filter: %s, Payload: %s", 
                                 data.topic.c_str(), payload_str.c_str());
                        
                        // set_mqtt_state_variable expects the exact topic that was matched,
                        // which is `topic_to_subscribe` (the filter we used).
                        set_mqtt_state_variable(topic_to_subscribe, payload_str);
                        // TODO: Trigger UI refresh
                    },
                    1 // QoS level
                );

                if (!subscribed) {
                    ESP_LOGE(TAG_VISIBILITY, "Failed to subscribe via g_mesh_handler.subscribe for topic: %s", topic_to_subscribe.c_str());
                }
            }
        }
    }
}

MenuItemVisibilityCondition parse_visibility_condition(const std::string& condition_str) {
    MenuItemVisibilityCondition condition;
    condition.type = VISIBILITY_ALWAYS; // Default

    size_t first_colon = condition_str.find(':');
    if (first_colon == std::string::npos) {
        ESP_LOGE(TAG_VISIBILITY, "Invalid visibility format (missing type colon): %s", condition_str.c_str());
        return condition; // Return default (always visible)
    }

    std::string type_str = condition_str.substr(0, first_colon);
    std::string params_str = condition_str.substr(first_colon + 1);

    ESP_LOGD(TAG_VISIBILITY, "Parsing visibility: Type=\'%s\', Params=\'%s\'", type_str.c_str(), params_str.c_str());


    if (type_str == "DATETIME_RANGE") {
        condition.type = VISIBILITY_DATETIME_RANGE;
        size_t comma_pos = params_str.find(',');
        if (comma_pos != std::string::npos) {
            condition.start_datetime_str = params_str.substr(0, comma_pos);
            condition.end_datetime_str = params_str.substr(comma_pos + 1);
        } else {
            ESP_LOGE(TAG_VISIBILITY, "Invalid DATETIME_RANGE format: %s", params_str.c_str());
            condition.type = VISIBILITY_ALWAYS; // Revert to default
        }
    } else if (type_str == "TIME_RANGE") {
        condition.type = VISIBILITY_TIME_RANGE;
        size_t comma_pos = params_str.find(',');
        if (comma_pos != std::string::npos) {
            condition.start_time_str = params_str.substr(0, comma_pos);
            condition.end_time_str = params_str.substr(comma_pos + 1);
        } else {
            ESP_LOGE(TAG_VISIBILITY, "Invalid TIME_RANGE format: %s", params_str.c_str());
            condition.type = VISIBILITY_ALWAYS;
        }
    } else if (type_str == "DATE_RANGE") {
        condition.type = VISIBILITY_DATE_RANGE;
        size_t comma_pos = params_str.find(',');
        if (comma_pos != std::string::npos) {
            condition.start_date_str = params_str.substr(0, comma_pos);
            condition.end_date_str = params_str.substr(comma_pos + 1);
        } else {
            ESP_LOGE(TAG_VISIBILITY, "Invalid DATE_RANGE format: %s", params_str.c_str());
            condition.type = VISIBILITY_ALWAYS;
        }
    } else if (type_str == "MQTT_STATE") {
        condition.type = VISIBILITY_MQTT_STATE;
        size_t comma_pos = params_str.find(',');
        if (comma_pos != std::string::npos) {
            condition.state_variable = params_str.substr(0, comma_pos); // Topic
            condition.state_value = params_str.substr(comma_pos + 1);    // Expected value
        } else {
            ESP_LOGE(TAG_VISIBILITY, "Invalid MQTT_STATE format: %s", params_str.c_str());
            condition.type = VISIBILITY_ALWAYS;
        }
    } else if (type_str == "CONTENT_AVAILABLE") {
        condition.type = VISIBILITY_CONTENT_AVAILABLE;
        condition.state_variable = params_str; // Content ID
    } else if (type_str == "CONTENT_VIEWED") {
        condition.type = VISIBILITY_CONTENT_VIEWED;
        condition.state_variable = params_str; // Page ID
    } else if (type_str == "CONTENT_NOT_VIEWED") {
        condition.type = VISIBILITY_CONTENT_NOT_VIEWED;
        condition.state_variable = params_str; // Page ID
    } else if (type_str != "ALWAYS") {
        ESP_LOGW(TAG_VISIBILITY, "Unknown visibility type: %s. Defaulting to ALWAYS.", type_str.c_str());
        condition.type = VISIBILITY_ALWAYS;
    }
    // For VISIBILITY_ALWAYS, no params needed.

    return condition;
}


bool evaluate_visibility_condition(const MenuItemVisibilityCondition& condition) { // Removed persistent_state parameter
    const MenuPersistentState& current_persistent_state = PersistentState::get_current_menu_state();
    // Mutex for persistent_state is handled within its own methods or by get_current_menu_state if needed for read.

    switch (condition.type) {
        case VISIBILITY_ALWAYS:
            return true;

        case VISIBILITY_DATETIME_RANGE: {
            std::tm tm_start = {}, tm_end = {}, tm_now = get_current_tm();
            if (!parse_datetime_string(condition.start_datetime_str, tm_start) ||
                !parse_datetime_string(condition.end_datetime_str, tm_end)) {
                ESP_LOGE(TAG_VISIBILITY, "Failed to parse datetime strings for DATETIME_RANGE");
                return false; 
            }
            std::time_t time_start = std::mktime(&tm_start);
            std::time_t time_end = std::mktime(&tm_end);
            std::time_t time_now = std::mktime(&tm_now);
            return (time_now >= time_start && time_now <= time_end);
        }

        case VISIBILITY_TIME_RANGE: {
            std::tm tm_start = {}, tm_end = {}, tm_now_full = get_current_tm();
            if (!parse_time_string(condition.start_time_str, tm_start) ||
                !parse_time_string(condition.end_time_str, tm_end)) {
                 ESP_LOGE(TAG_VISIBILITY, "Failed to parse time strings for TIME_RANGE");
                return false;
            }

            std::tm tm_now_start = tm_now_full;
            tm_now_start.tm_hour = tm_start.tm_hour;
            tm_now_start.tm_min = tm_start.tm_min;
            tm_now_start.tm_sec = 0;

            std::tm tm_now_end = tm_now_full;
            tm_now_end.tm_hour = tm_end.tm_hour;
            tm_now_end.tm_min = tm_end.tm_min;
            tm_now_end.tm_sec = 59; 

            std::time_t time_start = std::mktime(&tm_now_start);
            std::time_t time_end = std::mktime(&tm_now_end);
            std::time_t time_now = std::mktime(&tm_now_full);
            
            if (time_end < time_start) { 
                return (time_now >= time_start || time_now <= time_end);
            } else {
                return (time_now >= time_start && time_now <= time_end);
            }
        }

        case VISIBILITY_DATE_RANGE: {
            std::tm tm_start = {}, tm_end = {}, tm_now_full = get_current_tm();
             if (!parse_date_string(condition.start_date_str, tm_start) ||
                !parse_date_string(condition.end_date_str, tm_end)) {
                ESP_LOGE(TAG_VISIBILITY, "Failed to parse date strings for DATE_RANGE");
                return false;
            }

            tm_start.tm_hour = 0; tm_start.tm_min = 0; tm_start.tm_sec = 0;
            tm_end.tm_hour = 23; tm_end.tm_min = 59; tm_end.tm_sec = 59; 
            
            std::tm tm_now_date_only = tm_now_full;
            tm_now_date_only.tm_hour = 0; tm_now_date_only.tm_min = 0; tm_now_date_only.tm_sec = 0;

            std::time_t time_start = std::mktime(&tm_start);
            std::time_t time_end = std::mktime(&tm_end);
            std::time_t time_now = std::mktime(&tm_now_date_only);

            return (time_now >= time_start && time_now <= time_end);
        }

        case VISIBILITY_MQTT_STATE: {
            auto it = g_mqtt_state_vars.find(condition.state_variable);
            bool match = (it != g_mqtt_state_vars.end() && it->second == condition.state_value);
            return match;
        }

        case VISIBILITY_CONTENT_AVAILABLE:
            return PersistentState::is_content_available(condition.state_variable); // Uses its own mutex

        case VISIBILITY_CONTENT_VIEWED:
            return PersistentState::has_page_been_viewed(condition.state_variable); // Uses its own mutex
            
        case VISIBILITY_CONTENT_NOT_VIEWED:
            return !PersistentState::has_page_been_viewed(condition.state_variable); // Uses its own mutex

        default:
            ESP_LOGW(TAG_VISIBILITY, "Unhandled visibility type: %d", static_cast<int>(condition.type));
            return true; 
    }
}


bool evaluate_menu_item_visibility(const MenuItemDefinition& item_def) { // Removed persistent_state parameter
    ESP_LOGD(TAG_VISIBILITY, "Evaluating visibility for item: %s", item_def.name.c_str());
    bool is_visible = evaluate_visibility_condition(item_def.visibility); // Call updated function
    ESP_LOGD(TAG_VISIBILITY, "Item '%s' visibility: %s", item_def.name.c_str(), is_visible ? "VISIBLE" : "HIDDEN");
    return is_visible;
}
