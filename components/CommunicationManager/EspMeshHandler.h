// EspMeshHandler.h
#pragma once

#include "CommHandler.h"
#include "esp_mesh.h" // Main mesh header
#include "xasin/mqtt/Handler.h" 
#include "esp_event.h"      // For esp_event_base_t

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace Xasin {
namespace Communication {

class EspMeshHandler : public CommHandler {
public:
    struct EspMeshHandlerConfig {
        // Mesh specific configs
        std::string mesh_password; // Password of the mesh network (ESP-MESH uses password, not SSID for mesh ID)
        int mesh_channel = 0; // 0 for auto, 1-13
        // uint8_t mesh_id[6]; // MESH_ID, if to be configurable

        // WiFi AP config for Root Node (for connecting to external network)
        std::string wifi_ssid;
        std::string wifi_password;
        
        // MQTT Broker config for all nodes
        std::string mqtt_broker_uri;
        std::string mqtt_client_id_prefix; // e.g., "lzrtag_". Actual client ID will be prefix + device_id
        std::string mqtt_username;
        std::string mqtt_password;
        std::string mqtt_base_topic; // Base topic for application messages

        // Default constructor or constructor with specific settings
        EspMeshHandlerConfig() = default; 
    };

    EspMeshHandler(bool is_root_node = false);
    ~EspMeshHandler() override;

    bool start(void* config = nullptr) override;
    void stop() override;
    bool isConnected() const override; // Checks both mesh and MQTT connection
    bool publish(const std::string& topic, const void* data, size_t length, bool retain = false, int qos = 0) override;
    bool subscribe(const std::string& topic, comm_message_callback_t callback, int qos = 0) override;
    bool unsubscribe(const std::string& topic) override;
    void update() override; // For periodic tasks, if any (mesh itself is event-driven)
    std::string getDeviceId() override;
    void initialize_mesh_and_wifi(const std::string& wifi_ssid, const std::string& wifi_password);

    bool is_root() const { return is_root_; }

    void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    static void static_mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    
    static void static_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    // static void mesh_task(void *arg); // Task to run mesh operations - often handled by esp_mesh_start and events

    // Callback for when mesh raw data is received (if needed for non-IP mesh comms)
    // void on_mesh_data_received(const mesh_addr_t &from, const mesh_data_t &data);

    // Configuration and fundamental state
    EspMeshHandlerConfig current_config_;
    std::string device_id_; // Cached device ID
    bool is_root_ = false;

    // Mesh state
    mesh_addr_t mesh_id_ = {}; // MAC address of this node, used as MESH_ID in some contexts
    bool mesh_initialized_ = false;
    bool mesh_connected_ = false;   // True if non-root is connected to a parent, or root mesh is active/formed
    bool ip_acquired_ = false;       // True if the node has an IP address (either from mesh DHCP or external Wi-Fi for root)

    // MQTT Handler instance
    Xasin::MQTT::Handler m_mqtt_handler_;

    // Event handler instances for unregistration
    esp_event_handler_instance_t mesh_event_instance_ = nullptr;
    esp_event_handler_instance_t ip_event_instance_ = nullptr;

    // Structure to hold original callback and the Xasin Subscription
    struct SubscriptionInfo {
        Xasin::MQTT::Subscription* xasin_sub;
        comm_message_callback_t original_callback;
    };
    std::map<std::string, SubscriptionInfo> active_subscriptions_;

    // Task handle (if a dedicated task is still needed, e.g. for update())
    // TaskHandle_t mesh_task_handle_ = nullptr;
};

} // namespace Communication
} // namespace Xasin
