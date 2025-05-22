// EspMeshHandler.cpp
#include "EspMeshHandler.h"
#include "esp_log.h"
#include "esp_wifi.h"      // For Wi-Fi specific functions
#include "esp_mac.h"       // For MAC address functions
#include "esp_event.h"     // For esp_event_loop_create_default, etc.
#include "esp_mesh.h"      // For ESP-MESH API
#include "nvs_flash.h"   // For nvs_flash_init
#include "xasin/mqtt/Subscription.h"
#include <string.h>        // For memcpy, memset, strncpy
#include <sstream>         // For std::stringstream
#include <iomanip>         // For std::setw, std::setfill
#include <inttypes.h>      // For PRId32

// Assuming mesh_netif.h is in the same directory or include paths are set up
// It should have extern "C" guards for C++ compatibility
extern "C" {
#include "mesh_netif.h"
}

static const char *MESH_TAG = "EspMeshHandler"; // Logging tag

// Static event handler adapters
static void static_mesh_event_handler_adapter(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    Xasin::Communication::EspMeshHandler *handler = static_cast<Xasin::Communication::EspMeshHandler *>(arg);
    if (handler) {
        handler->mesh_event_handler(arg, event_base, event_id, event_data);
    }
}

static void static_ip_event_handler_adapter(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    Xasin::Communication::EspMeshHandler *handler = static_cast<Xasin::Communication::EspMeshHandler *>(arg);
    if (handler) {
        handler->ip_event_handler(arg, event_base, event_id, event_data);
    }
}

namespace Xasin {
namespace Communication {

// Helper to get MAC address as string
std::string get_device_mac_string() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA); // Get STA MAC, usually base MAC
    std::stringstream ss;
    for (int i = 0; i < 6; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)mac[i];
        if (i < 5) ss << ":";
    }
    return ss.str();
}

EspMeshHandler::EspMeshHandler(bool is_root_node)
    : CommHandler(),
      current_config_(),
      device_id_(""),
      is_root_(is_root_node), // Initialized by constructor, will be updated dynamically
      mesh_id_(),
      mesh_initialized_(false),
      mesh_connected_(false),
      ip_acquired_(false),
      m_mqtt_handler_(), // Default construct Xasin::MQTT::Handler
      active_subscriptions_() { // Initialize active_subscriptions_ map
    ESP_LOGI(MESH_TAG, "EspMeshHandler instance created. Initial is_root_ hint: %s", is_root_ ? "true" : "false");
}

EspMeshHandler::~EspMeshHandler() {
    stop();
}

bool EspMeshHandler::start(void *config) {
    ESP_LOGI(MESH_TAG, "Starting EspMeshHandler...");
    if (mesh_initialized_) {
        ESP_LOGW(MESH_TAG, "Already initialized.");
        return true;
    }

    if (!config) {
        ESP_LOGE(MESH_TAG, "Configuration is null!");
        return false;
    }
    current_config_ = *static_cast<EspMeshHandlerConfig *>(config);

    device_id_ = get_device_mac_string();
    ESP_LOGI(MESH_TAG, "Device ID: %s", device_id_.c_str());
    
    // Construct base topic for MQTT Handler if needed
    if (!current_config_.mqtt_base_topic.empty()) {
        // Assuming Xasin::MQTT::Handler has a way to set base topic after construction
        // or its constructor can take it. If it uses a fixed format like "/esp32/PROJECT_NAME/DEVICE_ID/",
        // ensure device_id_ is available or pass it.
        // For now, we assume Xasin::MQTT::Handler is configured with a base topic or uses a default.
        // If Xasin::MQTT::Handler's constructor `Handler(const std::string &base_t)` is to be used:
        // m_mqtt_handler_ = Xasin::MQTT::Handler(current_config_.mqtt_base_topic + device_id_ + "/");
        // This requires m_mqtt_handler_ to be pointer or initialized differently.
        // For simplicity, assuming default construction is fine and topics are full paths or handled by Handler.
    }


    initialize_mesh_and_wifi();

    mesh_initialized_ = true;
    ESP_LOGI(MESH_TAG, "EspMeshHandler start sequence initiated. Waiting for network events.");
    return true;
}

void EspMeshHandler::stop() {
    ESP_LOGI(MESH_TAG, "Stopping EspMeshHandler...");
    if (!mesh_initialized_) {
        ESP_LOGW(MESH_TAG, "Not initialized or already stopped.");
        return;
    }

    // Stop MQTT client
    // m_mqtt_handler_.stop(); // Assuming Xasin::MQTT::Handler destructor handles this or has explicit stop

    // Unsubscribe all active MQTT subscriptions
    for (auto it = active_subscriptions_.begin(); it != active_subscriptions_.end(); ++it) {
        if (it->second.xasin_sub) {
            delete it->second.xasin_sub; // Xasin::MQTT::Subscription destructor should unsubscribe
        }
    }
    active_subscriptions_.clear();

    mesh_netifs_destroy(); 
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_stop());
    
    // Unregister event handlers
    if (mesh_event_instance_) {
        esp_event_handler_instance_unregister(MESH_EVENT, ESP_EVENT_ANY_ID, mesh_event_instance_);
        mesh_event_instance_ = nullptr;
    }
    if (ip_event_instance_) {
        esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_instance_);
        ip_event_instance_ = nullptr;
    }

    mesh_initialized_ = false;
    mesh_connected_ = false;
    ip_acquired_ = false;
    ESP_LOGI(MESH_TAG, "EspMeshHandler stopped.");
}

void EspMeshHandler::initialize_mesh_and_wifi() {
    ESP_LOGI(MESH_TAG, "Initializing Network Stack, Wi-Fi, and ESP-MESH...");

    // Memory checks before initialization
    size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    ESP_LOGI(MESH_TAG, "Largest free memory block: %zu bytes", largest_free_block);

    bool heap_integrity = heap_caps_check_integrity_all(true);
    ESP_LOGI(MESH_TAG, "Heap integrity check: %s", heap_integrity ? "PASSED" : "FAILED");

    // 1. Initialize NVS (ensure it's done once globally, typically in app_main)
    esp_err_t ret = nvs_flash_init(); //
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(MESH_TAG, "NVS: Erasing and reinitializing.");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init(); //
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize TCP/IP stack and default event loop
    ESP_LOGI(MESH_TAG, "Initializing netif and event loop.");
    ret = esp_netif_init(); //
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(MESH_TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        ESP_ERROR_CHECK(ret);
    }
    ret = esp_event_loop_create_default(); //
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(MESH_TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        ESP_ERROR_CHECK(ret);
    }

    // 3. Initialize Wi-Fi Core
    ESP_LOGI(MESH_TAG, "Initializing Wi-Fi core.");
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT(); //
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg)); //
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); //

    // 4. Register event handlers
    ESP_LOGI(MESH_TAG, "Registering event handlers.");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(MESH_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &static_mesh_event_handler_adapter,
                                                        this,
                                                        &mesh_event_instance_)); //
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &static_ip_event_handler_adapter,
                                                        this,
                                                        &ip_event_instance_)); //

    // 5. Start Wi-Fi Subsystem
    ESP_LOGI(MESH_TAG, "Starting Wi-Fi subsystem.");
    ESP_ERROR_CHECK(esp_wifi_start()); //

    // 6. Initialize mesh_netif layer (from mesh_netif.c)
    // This should come after esp_wifi_start() as esp_netif_attach_wifi_station (called within mesh_netifs_init)
    // works best with an active Wi-Fi stack.
    ESP_LOGI(MESH_TAG, "Initializing mesh netifs.");
    ESP_ERROR_CHECK(mesh_netifs_init(nullptr)); //

    // 7. Initialize ESP-MESH Core
    // This must come after esp_wifi_start().
    ESP_LOGI(MESH_TAG, "Initializing ESP-MESH internal.");
    ESP_ERROR_CHECK(esp_mesh_init()); //

    // 8. Configure ESP-MESH
    esp_err_t config_err = apply_mesh_configuration(); //
    ESP_ERROR_CHECK(config_err); // Abort if initial configuration fails

    // 9. Start ESP-MESH Network Operation
    ESP_LOGI(MESH_TAG, "Starting ESP-MESH stack operation."); //
    while (true) { // Loop indefinitely until mesh starts //
        // Memory checks before each attempt to start mesh
        largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        ESP_LOGI(MESH_TAG, "[Retry] Largest free memory block: %zu bytes", largest_free_block); //

        heap_integrity = heap_caps_check_integrity_all(true);
        ESP_LOGI(MESH_TAG, "[Retry] Heap integrity check: %s", heap_integrity ? "PASSED" : "FAILED"); //

        esp_err_t mesh_start_ret = esp_mesh_start(); //
        if (mesh_start_ret == ESP_OK) {
            ESP_LOGI(MESH_TAG, "ESP-MESH started successfully. Waiting for mesh events."); //
            // Optional: Set mesh power-saving mode if needed
            // ESP_ERROR_CHECK(esp_mesh_set_ps(MESH_PS_NONE)); // Or MESH_PS_MANUAL, MESH_PS_AUTO
            return; // Successfully started and initialized mesh related parts
        } else {
            ESP_LOGE(MESH_TAG, "esp_mesh_start() failed with error 0x%x (%s).",
                     mesh_start_ret, esp_err_to_name(mesh_start_ret)); //

            bool reconfig_needed = false;
            bool wifi_needs_restart_after_deinit = false;

            if (mesh_start_ret == ESP_ERR_MESH_NOT_INIT) {
                ESP_LOGW(MESH_TAG, "Mesh reported as not initialized. Attempting to de-init and re-init."); //
                esp_err_t deinit_err = esp_mesh_deinit(); //
                if (deinit_err != ESP_OK && deinit_err != ESP_ERR_INVALID_STATE) { // ESP_ERR_INVALID_STATE if not inited //
                     ESP_LOGW(MESH_TAG, "esp_mesh_deinit() before re-init failed unexpectedly: %s", esp_err_to_name(deinit_err)); //
                }
                wifi_needs_restart_after_deinit = true; // Mark that Wi-Fi needs to be restarted as esp_mesh_deinit stops it.

                // Attempt to restart Wi-Fi if it was stopped by esp_mesh_deinit
                ESP_LOGI(MESH_TAG, "Attempting to restart Wi-Fi after mesh de-initialization.");
                esp_err_t wifi_start_again_err = esp_wifi_start();
                if (wifi_start_again_err != ESP_OK) {
                    ESP_LOGE(MESH_TAG, "Failed to restart Wi-Fi after mesh de-init: %s. Retrying full sequence.", esp_err_to_name(wifi_start_again_err));
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before full retry
                    continue; // Retry the entire while loop
                }

                esp_err_t reinit_ret = esp_mesh_init(); //
                if (reinit_ret == ESP_OK) {
                    ESP_LOGI(MESH_TAG, "esp_mesh_init() re-called successfully."); //
                    reconfig_needed = true; // Must reconfigure after re-init //
                } else {
                    ESP_LOGE(MESH_TAG, "Re-calling esp_mesh_init() failed with error 0x%x (%s).",
                             reinit_ret, esp_err_to_name(reinit_ret)); //
                    // Continue to retry esp_mesh_start after delay
                }
            } else if (mesh_start_ret == ESP_ERR_MESH_NOT_CONFIG) {
                ESP_LOGW(MESH_TAG, "Mesh reported as not configured. Attempting to re-configure."); //
                reconfig_needed = true;
            }
            // Add handling for other specific errors from esp_mesh_start() if necessary

            if (reconfig_needed) {
                esp_err_t apply_cfg_err = apply_mesh_configuration(); //
                if (apply_cfg_err == ESP_OK) {
                    ESP_LOGI(MESH_TAG, "ESP-MESH re-configuration applied successfully."); //
                } else {
                    ESP_LOGE(MESH_TAG, "Failed to re-apply ESP-MESH configuration: %s.", esp_err_to_name(apply_cfg_err)); //
                    // Continue to retry esp_mesh_start after delay
                }
            }

            ESP_LOGI(MESH_TAG, "Retrying esp_mesh_start() in 3 seconds..."); //
            vTaskDelay(pdMS_TO_TICKS(3000)); //
        }
    }
}

// Private helper function to apply mesh configuration
esp_err_t EspMeshHandler::apply_mesh_configuration() {
    ESP_LOGI(MESH_TAG, "Applying ESP-MESH configuration.");
    mesh_cfg_t mesh_cfg_to_set = {};
    esp_err_t err = esp_mesh_get_config(&mesh_cfg_to_set);
    if (err != ESP_OK) {
        ESP_LOGE(MESH_TAG, "Failed to get default mesh config: %s. Using zeroed config.", esp_err_to_name(err));
        // Proceeding with a zeroed config and then setting fields.
        // Alternatively, return err here if defaults are strictly needed.
        // For robustness, let's clear it again to be sure if get_config failed.
        memset(&mesh_cfg_to_set, 0, sizeof(mesh_cfg_t));
    }

    // MESH_ID: All nodes in the same mesh must have the same MESH_ID.
    uint8_t network_mesh_id[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // Example MESH_ID
    memcpy(mesh_cfg_to_set.mesh_id.addr, network_mesh_id, sizeof(mesh_cfg_to_set.mesh_id.addr));

    mesh_cfg_to_set.channel = current_config_.mesh_channel; // 0 for auto-select
    
    strncpy((char *)mesh_cfg_to_set.mesh_ap.password, current_config_.mesh_password.c_str(), sizeof(mesh_cfg_to_set.mesh_ap.password) - 1);
    mesh_cfg_to_set.mesh_ap.password[sizeof(mesh_cfg_to_set.mesh_ap.password) - 1] = '\0'; // Ensure null termination
    mesh_cfg_to_set.mesh_ap.max_connection = 4; // Was CONFIG_ESP_WIFI_AP_MAX_STA_CONN; 

    ESP_LOGI(MESH_TAG, "Configuring node for automatic root election (in apply_mesh_configuration).");
    // All nodes need the potential router's SSID to be able to connect if elected root
    mesh_cfg_to_set.router.ssid_len = current_config_.wifi_ssid.length();
    if (current_config_.wifi_ssid.length() > 0) {
        strncpy((char *)mesh_cfg_to_set.router.ssid, current_config_.wifi_ssid.c_str(), sizeof(mesh_cfg_to_set.router.ssid) - 1);
        mesh_cfg_to_set.router.ssid[sizeof(mesh_cfg_to_set.router.ssid) - 1] = '\0';
    } else {
        mesh_cfg_to_set.router.ssid[0] = '\0'; // No SSID, cannot connect to external AP
    }

    // All nodes need the potential router's password to be eligible for root if the AP is secured
    if (!current_config_.wifi_password.empty() && current_config_.wifi_ssid.length() > 0) {
        strncpy((char *)mesh_cfg_to_set.router.password, current_config_.wifi_password.c_str(), sizeof(mesh_cfg_to_set.router.password) - 1);
        mesh_cfg_to_set.router.password[sizeof(mesh_cfg_to_set.router.password) - 1] = '\0';
    } else {
        if (current_config_.wifi_ssid.length() > 0 && current_config_.wifi_password.empty()) {
             ESP_LOGI(MESH_TAG, "Router Wi-Fi password is empty, but SSID is present. Assuming open network or mesh-internal root operation.");
        }
        mesh_cfg_to_set.router.password[0] = '\0';
    }

    ESP_LOGI(MESH_TAG, "Current mesh configuration:");
    ESP_LOGI(MESH_TAG, "  Mesh ID: %02x:%02x:%02x:%02x:%02x:%02x", 
             mesh_cfg_to_set.mesh_id.addr[0], mesh_cfg_to_set.mesh_id.addr[1], 
             mesh_cfg_to_set.mesh_id.addr[2], mesh_cfg_to_set.mesh_id.addr[3], 
             mesh_cfg_to_set.mesh_id.addr[4], mesh_cfg_to_set.mesh_id.addr[5]);
    ESP_LOGI(MESH_TAG, "  Channel: %d", mesh_cfg_to_set.channel);
    ESP_LOGI(MESH_TAG, "  Mesh AP Password: %s", mesh_cfg_to_set.mesh_ap.password);
    ESP_LOGI(MESH_TAG, "  Max Connections: %d", mesh_cfg_to_set.mesh_ap.max_connection);
    ESP_LOGI(MESH_TAG, "  Router SSID: %s", mesh_cfg_to_set.router.ssid);
    ESP_LOGI(MESH_TAG, "  Router Password: %s", mesh_cfg_to_set.router.password);

    err = esp_mesh_set_config(&mesh_cfg_to_set);
    if (err != ESP_OK) {
        ESP_LOGE(MESH_TAG, "esp_mesh_set_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Set max layer for the mesh. This is a general mesh parameter.
    err = esp_mesh_set_max_layer(6); // Was CONFIG_ESP_MESH_MAX_LAYER;
    if (err != ESP_OK) {
        ESP_LOGE(MESH_TAG, "esp_mesh_set_max_layer failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(MESH_TAG, "ESP-MESH configuration applied successfully.");
    return ESP_OK;
}

// --- Event Handlers ---
void EspMeshHandler::mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(MESH_TAG, "Mesh event received: ID: %" PRId32, event_id);

    switch (event_id) {
    case MESH_EVENT_STARTED:
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED");
        esp_mesh_get_id(&mesh_id_); // Get the node's own mesh ID (MAC based)
        ESP_LOGI(MESH_TAG, "Node Mesh MAC ID: " MACSTR, MAC2STR(mesh_id_.addr));
        
        this->is_root_ = esp_mesh_is_root(); // Update root status
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED: Node is_root actual status: %s", this->is_root_ ? "true" : "false");

        mesh_connected_ = true; // Mesh protocol has started
        mesh_recovery_task_running_ = false;

        if (this->is_root_ && current_config_.wifi_ssid.empty()) { // Standalone root
            ESP_LOGI(MESH_TAG, "Standalone root started. Starting mesh AP netif.");
            ESP_ERROR_CHECK(mesh_netif_start_root_ap(true, 0)); 
            ip_acquired_ = true; 
            ESP_LOGI(MESH_TAG, "Standalone Root IP acquired (static). Starting MQTT client.");
            if (!current_config_.mqtt_broker_uri.empty()) {
                m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }
        }
        break;
    case MESH_EVENT_STOPPED:
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STOPPED");
        mesh_connected_ = false;
        ip_acquired_ = false;
        this->is_root_ = false; // When mesh stops, not acting as root
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STOPPED: Node is_root set to false.");
        // m_mqtt_handler_.stop(); // Or let it handle disconnects
        break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected_event = (mesh_event_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "MESH_EVENT_PARENT_CONNECTED to BSSID: " MACSTR ", Layer: %d",
                 MAC2STR(connected_event->connected.bssid), esp_mesh_get_layer());
        
        this->is_root_ = false; // Connected to a parent, so not root
        ESP_LOGI(MESH_TAG, "MESH_EVENT_PARENT_CONNECTED: Node is_root set to false.");

        mesh_connected_ = true; 
        ESP_LOGI(MESH_TAG, "Node connected to parent. Starting mesh STA netif for IP.");
        ESP_ERROR_CHECK(mesh_netifs_start(false)); // false = is_node
        break;
    }
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected_event = (mesh_event_disconnected_t *)event_data;
        ESP_LOGW(MESH_TAG, "MESH_EVENT_PARENT_DISCONNECTED from BSSID: " MACSTR ", Reason: %d",
                 MAC2STR(disconnected_event->bssid), disconnected_event->reason);
        mesh_connected_ = false;
        ip_acquired_ = false;
        // If it was a child and lost parent, it's not root. esp_mesh_is_root() might become true if it self-heals as root.
        // For now, we don't change is_root_ here, MESH_EVENT_STARTED will update if it becomes root.
        // m_mqtt_handler_.stop(); 
        if (!this->is_root_) { // Check current state before logging
            ESP_LOGI(MESH_TAG, "Node (child) lost parent. ESP-MESH will attempt to find a new parent.");
        }
        break;
    }
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change_event = (mesh_event_layer_change_t *)event_data;
        ESP_LOGI(MESH_TAG, "MESH_EVENT_LAYER_CHANGE: New layer: %d", layer_change_event->new_layer);
        break;
    }
    // ESP-IDF v4.4.x: Event for root obtaining its address for the mesh
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *addr_event = (mesh_event_root_address_t *)event_data;
        // addr_event is of type mesh_addr_t*
        // The IP address is in addr_event->mip.ip4
        ESP_LOGI(MESH_TAG, "MESH_EVENT_ROOT_ADDRESS: Root has address on mesh: " IPSTR, IP2STR(&(addr_event->mip.ip4)));

        if (is_root_) {
            // This event confirms the root's address within the mesh.
            // If the root is also connecting to an external router, it will get another IP via DHCP (IP_EVENT_STA_GOT_IP).
            // If it's a standalone root, its mesh IP is static (usually 10.0.0.1 from mesh_netif),
            // and this event might just confirm that.
            // If MQTT broker is on this root, and it's a standalone root, MQTT might have already started if ip_acquired_ was set.
        } else {
            // A child node has received the root's address.
            // This could be useful if the MQTT broker is the root node and its IP needs to be discovered.
            // For now, current_config_.mqtt_broker_uri is assumed to be fixed or handled by IP_EVENT_STA_GOT_IP if root is gateway.
            // If the root IS the broker and this is its mesh IP:
            // char root_ip_str[16];
            // sprintf(root_ip_str, IPSTR, IP2STR(&(addr_event->mip.ip4)));
            // std::string broker_uri = "mqtt://" + std::string(root_ip_str);
            // Potentially update m_mqtt_handler_ if it's not started or needs re-config.
            // This logic depends heavily on your MQTT broker discovery strategy.
        }

        // Update is_root_ based on esp_mesh_is_root() as this event can signify a change in root status or confirmation.
        bool current_is_root = esp_mesh_is_root();
        if (this->is_root_ != current_is_root) {
            ESP_LOGI(MESH_TAG, "MESH_EVENT_ROOT_ADDRESS: is_root_ changed from %s to %s", this->is_root_ ? "true" : "false", current_is_root ? "true" : "false");
            this->is_root_ = current_is_root;
        }
        break;
    }
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGW(MESH_TAG, "MESH_EVENT_NO_PARENT_FOUND. Will keep trying.");
        break;
    // Add other mesh events as needed
    default:
        ESP_LOGD(MESH_TAG, "Unhandled Mesh Event ID: %" PRId32, event_id);
        break;
    }
}

void EspMeshHandler::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(MESH_TAG, "IP event received: ID: %" PRId32, event_id);

    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        const char* if_key = esp_netif_get_ifkey(event->esp_netif);
        ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP: Interface: %s, IP: " IPSTR, if_key, IP2STR(&event->ip_info.ip));
        
        ip_acquired_ = true; 

        esp_netif_t* wifi_sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_t* mesh_sta_netif = esp_netif_get_handle_from_ifkey("MESH_STA_DEF");

        if (event->esp_netif == wifi_sta_netif) {
            ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP on external STA interface (WIFI_STA_DEF). Node is acting as ROOT.");
            this->is_root_ = true;
            
            esp_netif_dns_info_t dns_info;
            esp_err_t dns_ret = esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dns_info);
            uint32_t dns_addr = 0;
            if (dns_ret == ESP_OK) {
                dns_addr = dns_info.ip.u_addr.ip4.addr;
                ESP_LOGI(MESH_TAG, "Using DNS Server for mesh children: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
            } else {
                ESP_LOGW(MESH_TAG, "Failed to get DNS info for root\'s external STA (err: %s). Using 0.0.0.0 for children\'s DNS.", esp_err_to_name(dns_ret));
            }
            ESP_ERROR_CHECK(mesh_netif_start_root_ap(true, dns_addr)); 

            ESP_LOGI(MESH_TAG, "Root (external IP OK, mesh AP starting). Starting MQTT client.");
            if (!current_config_.mqtt_broker_uri.empty()) {
                m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }

        } else if (event->esp_netif == mesh_sta_netif) {
            ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP on MESH_STA interface (MESH_STA_DEF). Node is CHILD.");
            this->is_root_ = false;
            ESP_LOGI(MESH_TAG, "Node got IP on mesh interface. Starting MQTT client.");
            if (!current_config_.mqtt_broker_uri.empty()) {
                 m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }
        } else {
            // Could be MESH_AP_DEF or other interfaces.
            // Check if it became root if not already determined.
            bool current_is_root_status = esp_mesh_is_root();
            if(this->is_root_ != current_is_root_status) {
                 ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP for interface %s. Root status changed to %s", if_key, current_is_root_status ? "true" : "false");
                 this->is_root_ = current_is_root_status;
            } else {
                ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP for an unhandled or already consistent interface key: %s. Current is_root: %s", if_key, this->is_root_ ? "true" : "false");
            }
        }

    } else if (event_id == IP_EVENT_STA_LOST_IP) {
        // ESP-IDF v4.4.x: event_data for IP_EVENT_STA_LOST_IP is ip_event_got_ip_t*
        // to access the esp_netif handle.
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        const char* if_key_lost = event ? esp_netif_get_ifkey(event->esp_netif) : "UNKNOWN_IF";
        ESP_LOGW(MESH_TAG, "IP_EVENT_STA_LOST_IP on interface %s", if_key_lost);
        ip_acquired_ = false; 
    } else if (event_id == IP_EVENT_AP_STAIPASSIGNED && is_root_) {
        // This event means the root's DHCP server (on the mesh AP interface) assigned an IP to a child.
        ip_event_ap_staipassigned_t* assigned_event = (ip_event_ap_staipassigned_t*)event_data;
        ESP_LOGI(MESH_TAG, "Root's Mesh AP assigned IP: " IPSTR " to a station.", IP2STR(&assigned_event->ip));
    }
}


// --- Interface Methods ---
bool EspMeshHandler::isConnected() const {
    // For a root node, it's connected if mesh is up AND MQTT is connected (if MQTT is used).
    // For a child node, it's connected if mesh is up (connected to parent) AND it has an IP.
    // MQTT connection status for a child might also be relevant if it directly talks to a broker.

    if (!mesh_initialized_ || !mesh_connected_) {
        ESP_LOGV(MESH_TAG, "isConnected: Basic mesh not up (initialized: %d, connected: %d)", mesh_initialized_, mesh_connected_);
        return false; // Basic mesh connectivity is required.
    }

    if (is_root_) {
        // Root node: needs mesh to be active and, if using external MQTT, MQTT to be connected.
        // If MQTT broker URI is empty, we assume no MQTT is needed, so just mesh status.
        if (current_config_.mqtt_broker_uri.empty()) {
            ESP_LOGV(MESH_TAG, "isConnected (Root, No MQTT URI): IP acquired: %d", ip_acquired_);
            return ip_acquired_; // Standalone root needs its IP for mesh services.
        }
        // Root with MQTT: mesh must be up, IP acquired (either on mesh or external Wi-Fi), and MQTT connected.
        bool mqtt_ok = (m_mqtt_handler_.is_disconnected() == 0);
        ESP_LOGV(MESH_TAG, "isConnected (Root, With MQTT URI): IP acquired: %d, MQTT OK: %d", ip_acquired_, mqtt_ok);
        return ip_acquired_ && mqtt_ok;
    } else {
        // Child node: needs to be connected to a parent and have an IP.
        // If children also use MQTT directly:
        if (current_config_.mqtt_broker_uri.empty()) { // No MQTT for child
            ESP_LOGV(MESH_TAG, "isConnected (Child, No MQTT URI): IP acquired: %d", ip_acquired_);
            return ip_acquired_; // Connected if has IP from mesh
        }
        bool mqtt_ok = (m_mqtt_handler_.is_disconnected() == 0);
        ESP_LOGV(MESH_TAG, "isConnected (Child, With MQTT URI): IP acquired: %d, MQTT OK: %d", ip_acquired_, mqtt_ok);
        return ip_acquired_ && mqtt_ok;
    }
}

bool EspMeshHandler::publish(const std::string& topic, const void* data, size_t length, bool retain, int qos) {
    if (!isConnected()) {
        ESP_LOGW(MESH_TAG, "Cannot publish, not fully connected. Mesh: %d, IP: %d, MQTT Connected: %s", 
                 mesh_connected_, ip_acquired_, (m_mqtt_handler_.is_disconnected() == 0) ? "yes" : "no");
        return false;
    }
    m_mqtt_handler_.publish_to(topic, data, length, retain, qos);
    return true; // Xasin::MQTT::Handler::publish_to is void, so assume success if connected.
}

bool EspMeshHandler::subscribe(const std::string &topic, comm_message_callback_t callback, int qos) {
    ESP_LOGI(MESH_TAG, "Subscribing to topic: %s", topic.c_str());
    if (m_mqtt_handler_.is_disconnected() == 255) { // 255 means not started
        ESP_LOGE(MESH_TAG, "MQTT handler not started, cannot subscribe to topic: %s", topic.c_str());
        return false;
    }

    comm_message_callback_t captured_user_callback = std::move(callback);
    // Capture the original topic filter string for use in the callback
    std::string topic_filter_for_lambda = topic;

    Xasin::MQTT::mqtt_callback xasin_callback =
        [captured_user_callback, topic_filter_for_lambda](const Xasin::MQTT::MQTT_Packet& packet) {
        CommReceivedData received_data;
        
        // Use the original topic filter the user subscribed with
        received_data.topic = topic_filter_for_lambda; 
        received_data.payload = std::vector<uint8_t>(packet.data.begin(), packet.data.end());
        
        std::string source_id_str = "";
        std::string full_topic_str = packet.topic; // Full topic from MQTT broker

        // Attempt to parse source_id from the full_topic_str
        // Expected format: /<prefix>/<project_name>/<device_id>/<actual_sub_topic>
        // Example: /esp32/lzrtag/deviceABC/event/hit
        
        size_t component_start_pos = 0;
        if (!full_topic_str.empty() && full_topic_str[0] == '/') {
            component_start_pos = 1; // Skip leading '/'
        }

        std::string path_to_parse = full_topic_str.substr(component_start_pos);
        std::string segment;
        std::vector<std::string> segments; // Requires <vector>
        std::istringstream segment_stream(path_to_parse); // Requires <sstream>

        while(std::getline(segment_stream, segment, '/')) {
            segments.push_back(segment);
        }

        // segments[0] = "esp32" (or similar prefix part)
        // segments[1] = project_name (e.g., "lzrtag")
        // segments[2] = device_id (this is what we want for source_id)
        if (segments.size() >= 3) {
            source_id_str = segments[2];
        }

        received_data.source_id = source_id_str;
        
        ESP_LOGD(MESH_TAG, "MQTT Pkt Recv. Full Topic: '%s', Filter: '%s', Parsed SrcID: '%s', Payload Sz: %zu. Relaying.", 
                 full_topic_str.c_str(), topic_filter_for_lambda.c_str(), source_id_str.c_str(), packet.data.size());
        captured_user_callback(received_data); 
    };

    Xasin::MQTT::Subscription* sub = m_mqtt_handler_.subscribe_to(topic, xasin_callback, qos);

    if (sub) {
        ESP_LOGI(MESH_TAG, "Successfully initiated subscription to topic: %s", topic.c_str());
        // Store the callback that was moved and captured.
        active_subscriptions_[topic] = {sub, captured_user_callback};
        return true; 
    } else {
        ESP_LOGE(MESH_TAG, "Failed to create subscription object for topic: %s", topic.c_str());
        return false;
    }
}

bool EspMeshHandler::unsubscribe(const std::string &topic) {
    auto it = active_subscriptions_.find(topic);
    if (it != active_subscriptions_.end()) {
        ESP_LOGI(MESH_TAG, "Unsubscribing from topic: %s", topic.c_str());
        // Xasin::MQTT::Subscription destructor should handle the actual MQTT unsubscribe.
        delete it->second.xasin_sub;
        active_subscriptions_.erase(it);
        return true;
    } else {
        ESP_LOGW(MESH_TAG, "Not subscribed to topic: %s", topic.c_str());
        return false;
    }
}

void EspMeshHandler::update() {
    // ESP-MESH and ESP-MQTT are event-driven.
    // Xasin::MQTT::Handler does not seem to require an update() call.
    // This can be left empty or used for other periodic tasks if necessary.
}

std::string EspMeshHandler::getDeviceId() {
    if (device_id_.empty()) {
        device_id_ = get_device_mac_string(); // Ensure it's fetched if not already
    }
    return device_id_;
}

bool EspMeshHandler::isRootNode() const {
    return is_root_;
}

} // namespace Communication
} // namespace Xasin

