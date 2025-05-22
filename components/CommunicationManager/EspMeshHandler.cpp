// EspMeshHandler.cpp
#include "EspMeshHandler.h"
#include "esp_log.h"
#include "esp_wifi.h"      // For Wi-Fi specific functions
#include "esp_mac.h"       // For MAC address functions
#include "esp_event.h"     // For esp_event_loop_create_default, etc.
#include "esp_mesh.h"      // For ESP-MESH API
#include "nvs_flash.h"   // For nvs_flash_init
#include "xasin/mqtt/Subscription.h" // Assuming this is your MQTT subscription handler
#include <string.h>        // For memcpy, memset, strncpy
#include <sstream>         // For std::stringstream
#include <iomanip>         // For std::setw, std::setfill
#include <inttypes.h>      // For PRId32
#include "esp_sntp.h"
#include "esp_tls.h"       // For esp_tls_init_global_ca_store

// Required for mesh_netif.c helper functions
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

// SNTP initialization function (moved outside class for clarity or if used globally)
void initialize_sntp_task() { // Renamed to avoid conflict if there's a global initialize_sntp
    ESP_LOGI("SNTP", "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); // Default Kconfig is "pool.ntp.org"
    esp_sntp_init();

    xTaskCreate([](void *param) {
        int retry = 0;
        const int retry_count = 15; // Increased retry count for SNTP
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
            ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds between retries
        }
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            timeval tv;
            gettimeofday(&tv, NULL);
            ESP_LOGI("SNTP", "System time set to: %ld.%06ld", tv.tv_sec, tv.tv_usec);
        } else {
            ESP_LOGW("SNTP", "Failed to synchronize time within the timeout period. Status: %d", sntp_get_sync_status());
        }
        vTaskDelete(NULL); 
    }, "SNTP_Wait", 2560, NULL, tskIDLE_PRIORITY + 1, NULL); // Increased stack size slightly
}


// Symbols for the embedded CA certificate bundle (if using Kconfig option)
// Ensure CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y is in your sdkconfig
extern const uint8_t ca_cert_bundle_start[] asm("_binary_hivemq_pem_start");
extern const uint8_t ca_cert_bundle_end[] asm("_binary_hivemq_pem_end");


namespace Xasin {
namespace Communication {

// Helper to get MAC address as string
std::string get_device_mac_string() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA); 
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

    // m_mqtt_handler_.stop(); // Assuming your MQTT handler has a stop method or handles it in destructor

    for (auto it = active_subscriptions_.begin(); it != active_subscriptions_.end(); ++it) {
        if (it->second.xasin_sub) {
            delete it->second.xasin_sub; 
        }
    }
    active_subscriptions_.clear();
    mesh_netifs_destroy(); 
    ESP_LOGI(MESH_TAG, "Mesh netifs destroyed.");
    
    esp_err_t err = esp_mesh_stop();
    if (err != ESP_OK && err != ESP_ERR_MESH_NOT_START) {
         ESP_LOGW(MESH_TAG, "esp_mesh_stop() failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(MESH_TAG, "ESP-MESH stopped or was not running.");
    }
    
    if (mesh_event_instance_) {
        esp_event_handler_instance_unregister(MESH_EVENT, ESP_EVENT_ANY_ID, mesh_event_instance_);
        mesh_event_instance_ = nullptr;
    }
    if (ip_event_instance_) {
        esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_instance_);
        ip_event_instance_ = nullptr;
    }

    // Consider esp_wifi_stop() and esp_wifi_deinit() if appropriate for your lifecycle
    // esp_wifi_stop();
    // esp_wifi_deinit();
    // esp_netif_destroy_default_wifi_sta(); // This was removed from init, so not needed here unless created elsewhere
    // esp_event_loop_delete_default();
    // esp_netif_deinit();


    mesh_initialized_ = false;
    mesh_connected_ = false;
    ip_acquired_ = false;
    ESP_LOGI(MESH_TAG, "EspMeshHandler stopped.");
}

void EspMeshHandler::initialize_mesh_and_wifi() {
    ESP_LOGI(MESH_TAG, "Initializing Network Stack, Wi-Fi, and ESP-MESH...");

    size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    ESP_LOGI(MESH_TAG, "Initial largest free memory block: %zu bytes", largest_free_block);
    heap_caps_check_integrity_all(true); // Log only, don't necessarily abort

    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(MESH_TAG, "NVS: Erasing and reinitializing due to: %s", esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize TCP/IP stack and default event loop
    ESP_LOGI(MESH_TAG, "Initializing LwIP and event loop.");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Initialize Mesh-specific Netifs (MESH_AP_DEF, MESH_STA_DEF)
    // This MUST be called before esp_wifi_init() if using mesh_netif.c helpers for IP internal network.
    // mesh_netifs_init(NULL) also creates the default STA netif ("WIFI_STA_DEF") internally.
    ESP_LOGI(MESH_TAG, "Initializing Mesh Netifs (for MESH_AP_DEF and default WIFI_STA_DEF).");
    ESP_ERROR_CHECK(mesh_netifs_init(NULL)); 

    // 4. Create default Wi-Fi STA netif (for external router connection if this node becomes root)
    // THIS IS NO LONGER NEEDED as mesh_netifs_init() already creates the default STA interface.
    // ESP_LOGI(MESH_TAG, "Creating default Wi-Fi STA netif (WIFI_STA_DEF for external connection).");
    // esp_netif_t *default_sta_netif = esp_netif_create_default_wifi_sta();
    // assert(default_sta_netif); 
    ESP_LOGI(MESH_TAG, "Default Wi-Fi STA netif (WIFI_STA_DEF) is created by mesh_netifs_init.");


    // 5. Initialize Wi-Fi Core
    ESP_LOGI(MESH_TAG, "Initializing Wi-Fi core.");
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_LOGI(MESH_TAG, "Setting Wi-Fi mode to STA."); // Mesh operates in STA mode primarily
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 6. Register event handlers
    ESP_LOGI(MESH_TAG, "Registering EspMeshHandler event handlers.");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(MESH_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &static_mesh_event_handler_adapter,
                                                        this,
                                                        &mesh_event_instance_));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &static_ip_event_handler_adapter,
                                                        this,
                                                        &ip_event_instance_));

    // 7. Start Wi-Fi Subsystem
    ESP_LOGI(MESH_TAG, "Starting Wi-Fi subsystem.");
    ESP_ERROR_CHECK(esp_wifi_start()); 

    // 8. Initialize Global CA Store (for TLS connections like MQTTS)
    // This uses the bundle embedded via Kconfig (CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y)
    ESP_LOGI(MESH_TAG, "Initializing Global CA Store for TLS.");
    esp_err_t ca_store_ret = esp_tls_set_global_ca_store(ca_cert_bundle_start, ca_cert_bundle_end - ca_cert_bundle_start);
    if (ca_store_ret != ESP_OK) {
        ESP_LOGE(MESH_TAG, "Failed to initialize global CA store: %s. TLS connections may fail.", esp_err_to_name(ca_store_ret));
        // Depending on strictness, you might want to ESP_ERROR_CHECK here or handle failure.
    } else {
        ESP_LOGI(MESH_TAG, "Global CA store initialized successfully.");
    }

    // 9. Initialize ESP-MESH Core
    ESP_LOGI(MESH_TAG, "Initializing ESP-MESH internal.");
    ESP_ERROR_CHECK(esp_mesh_init()); 

    // 10. Configure ESP-MESH
    ESP_LOGI(MESH_TAG, "Applying ESP-MESH configuration.");
    ESP_ERROR_CHECK(apply_mesh_configuration()); 

    // 11. Start ESP-MESH Network Operation (with retry logic)
    ESP_LOGI(MESH_TAG, "Attempting to start ESP-MESH stack operation.");
    int retry_count = 0;
    const int max_retries = 5; // Max retries for mesh start
    while (retry_count < max_retries) {
        largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        ESP_LOGI(MESH_TAG, "[Mesh Start Attempt %d/%d] Largest free memory: %zu bytes", retry_count + 1, max_retries, largest_free_block);
        
        esp_err_t mesh_start_ret = esp_mesh_start();
        if (mesh_start_ret == ESP_OK) {
            ESP_LOGI(MESH_TAG, "ESP-MESH started successfully! Waiting for mesh events.");
            return; 
        } 
        
        ESP_LOGE(MESH_TAG, "esp_mesh_start() failed (attempt %d/%d): 0x%x (%s).",
                 retry_count + 1, max_retries, mesh_start_ret, esp_err_to_name(mesh_start_ret));
        retry_count++;

        // Simplified retry logic from your previous version
        if (mesh_start_ret == ESP_ERR_MESH_NOT_INIT || mesh_start_ret == ESP_FAIL || mesh_start_ret == ESP_ERR_WIFI_NOT_INIT) {
            ESP_LOGW(MESH_TAG, "Significant mesh start failure. Attempting to de-init and re-init sequence.");
            esp_mesh_deinit(); // Ignore error, best effort
            esp_wifi_stop();   // Stop Wi-Fi before re-init
            esp_wifi_deinit(); // De-init Wi-Fi
            
            // Re-initialize Wi-Fi
            ESP_LOGI(MESH_TAG, "Re-initializing Wi-Fi core for retry.");
            wifi_init_config_t temp_wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&temp_wifi_cfg));
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_storage(WIFI_STORAGE_RAM));
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
            esp_err_t wifi_start_again_err = esp_wifi_start();
            if (wifi_start_again_err != ESP_OK) {
                 ESP_LOGE(MESH_TAG, "Failed to restart Wi-Fi after de-init: %s. Retrying mesh_start.", esp_err_to_name(wifi_start_again_err));
            } else {
                 ESP_LOGI(MESH_TAG, "Wi-Fi restarted successfully.");
            }

            ESP_LOGI(MESH_TAG, "Re-initializing ESP-MESH internal for retry.");
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_init());
        }
        
        if (mesh_start_ret == ESP_ERR_MESH_NOT_CONFIG || mesh_start_ret == ESP_ERR_MESH_NOT_INIT || mesh_start_ret == ESP_FAIL) {
             ESP_LOGI(MESH_TAG, "Re-applying ESP-MESH configuration for retry.");
             apply_mesh_configuration(); // Ignore error, best effort
        }
        
        ESP_LOGI(MESH_TAG, "Retrying esp_mesh_start() in 3 seconds...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    ESP_LOGE(MESH_TAG, "Failed to start ESP-MESH after %d attempts. Halting initialization.", max_retries);
    // Consider a more robust failure state here, e.g., deep sleep and retry, or error LED.
}

esp_err_t EspMeshHandler::apply_mesh_configuration() {
    ESP_LOGI(MESH_TAG, "Applying ESP-MESH configuration.");
    mesh_cfg_t mesh_cfg_to_set = {}; // Zero-initialize
    
    // It's generally safer to start from zeroed config and set all required fields,
    // rather than relying on esp_mesh_get_config if its success is uncertain.
    // However, if esp_mesh_get_config is reliable, it can preserve some defaults.
    // For this example, let's assume we set all necessary fields.

    // MESH_ID
    // uint8_t network_mesh_id[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // Example
    // For safety, use a configurable MESH_ID from current_config_ if available, or a fixed one.
    // Assuming current_config_.mesh_id is a std::array<uint8_t, 6> or similar.
    // If current_config_.mesh_id is not set, use a default.
    if (current_config_.mesh_id.size() == 6) { // Assuming mesh_id is a container like std::vector or std::array
        memcpy(mesh_cfg_to_set.mesh_id.addr, current_config_.mesh_id.data(), sizeof(mesh_cfg_to_set.mesh_id.addr));
    } else {
        uint8_t default_mesh_id[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        memcpy(mesh_cfg_to_set.mesh_id.addr, default_mesh_id, sizeof(mesh_cfg_to_set.mesh_id.addr));
        ESP_LOGW(MESH_TAG, "Using default MESH_ID as none was provided in config.");
    }


    mesh_cfg_to_set.channel = current_config_.mesh_channel; 
    if (current_config_.mesh_password.length() >= sizeof(mesh_cfg_to_set.mesh_ap.password)) {
        ESP_LOGW(MESH_TAG, "Mesh password too long, will be truncated.");
    }
    strncpy((char *)mesh_cfg_to_set.mesh_ap.password, current_config_.mesh_password.c_str(), sizeof(mesh_cfg_to_set.mesh_ap.password) - 1);
    mesh_cfg_to_set.mesh_ap.password[sizeof(mesh_cfg_to_set.mesh_ap.password) - 1] = '\0'; 
    
    // Use Kconfig for max connections if available, or a sensible default
    mesh_cfg_to_set.mesh_ap.max_connection = 4; 


    // Router (external AP) configuration - only relevant if this node *might* become root
    if (current_config_.wifi_ssid.length() > sizeof(mesh_cfg_to_set.router.ssid) -1 ) {
         ESP_LOGW(MESH_TAG, "Router SSID too long, will be truncated.");
    }
    strncpy((char *)mesh_cfg_to_set.router.ssid, current_config_.wifi_ssid.c_str(), sizeof(mesh_cfg_to_set.router.ssid) -1);
    mesh_cfg_to_set.router.ssid[sizeof(mesh_cfg_to_set.router.ssid) -1] = '\0';
    mesh_cfg_to_set.router.ssid_len = strlen((char*)mesh_cfg_to_set.router.ssid);


    if (current_config_.wifi_password.length() > sizeof(mesh_cfg_to_set.router.password) -1) {
        ESP_LOGW(MESH_TAG, "Router Password too long, will be truncated.");
    }
    strncpy((char *)mesh_cfg_to_set.router.password, current_config_.wifi_password.c_str(), sizeof(mesh_cfg_to_set.router.password) -1);
    mesh_cfg_to_set.router.password[sizeof(mesh_cfg_to_set.router.password) -1] = '\0';


    ESP_LOGI(MESH_TAG, "Setting mesh configuration: MESH_ID: %02x:%02x:%02x:%02x:%02x:%02x, Channel: %d, Router SSID: %s",
             mesh_cfg_to_set.mesh_id.addr[0], mesh_cfg_to_set.mesh_id.addr[1], mesh_cfg_to_set.mesh_id.addr[2],
             mesh_cfg_to_set.mesh_id.addr[3], mesh_cfg_to_set.mesh_id.addr[4], mesh_cfg_to_set.mesh_id.addr[5],
             mesh_cfg_to_set.channel, mesh_cfg_to_set.router.ssid);

    esp_err_t err = esp_mesh_set_config(&mesh_cfg_to_set);
    if (err != ESP_OK) {
        ESP_LOGE(MESH_TAG, "esp_mesh_set_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Set max layer for the mesh.
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(6)); // Use Kconfig value
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1)); // Example: A single node can become root
    ESP_ERROR_CHECK(esp_mesh_enable_ps()); // Enable mesh power saving if desired
    // ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10)); // Example: AP association expiry in seconds

    ESP_LOGI(MESH_TAG, "ESP-MESH configuration applied successfully.");
    return ESP_OK;
}

void EspMeshHandler::mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(MESH_TAG, "Mesh event received: ID: %" PRId32, event_id);

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED");
        esp_mesh_get_id(&mesh_id_); 
        ESP_LOGI(MESH_TAG, "Node Mesh MAC ID: " MACSTR, MAC2STR(mesh_id_.addr));
        mesh_connected_ = true; // Mesh stack has started
        
        // Initial root status check. This might change based on router connection.
        this->is_root_ = esp_mesh_is_root(); 
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED: Initial esp_mesh_is_root(): %s", this->is_root_ ? "true" : "false");

        if (this->is_root_ && current_config_.wifi_ssid.empty()) {
            // Standalone root (no external router configured)
            ESP_LOGI(MESH_TAG, "Standalone root started (no router config). Starting mesh AP netif.");
            ESP_ERROR_CHECK(mesh_netif_start_root_ap(true, 0)); // dns_addr 0 for standalone
            ip_acquired_ = true; // Internal IP is set up by mesh_netif_start_root_ap
            ESP_LOGI(MESH_TAG, "Standalone Root IP setup. Starting MQTT client (if configured).");
            if (!current_config_.mqtt_broker_uri.empty()) {
                m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }
            initialize_sntp_task(); // Initialize SNTP for standalone root if needed
        } else if (this->is_root_ && !current_config_.wifi_ssid.empty()){
             ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED: Node is root, router configured. Waiting for IP_EVENT_STA_GOT_IP on WIFI_STA_DEF.");
             // Root status will be fully confirmed and MQTT started upon getting external IP.
        } else if (!this->is_root_) {
            ESP_LOGI(MESH_TAG, "MESH_EVENT_STARTED: Node is child. Waiting for MESH_EVENT_PARENT_CONNECTED.");
        }
        break;
    }
    case MESH_EVENT_STOPPED:
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STOPPED");
        mesh_connected_ = false;
        ip_acquired_ = false;
        this->is_root_ = false; 
        ESP_LOGI(MESH_TAG, "MESH_EVENT_STOPPED: Node is_root set to false.");
        // Consider stopping MQTT if it was running
        // m_mqtt_handler_.stop(); 
        break;

    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected_event = (mesh_event_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "MESH_EVENT_PARENT_CONNECTED to BSSID: " MACSTR ", Layer: %d",
                 MAC2STR(connected_event->connected.bssid), esp_mesh_get_layer());
        
        mesh_connected_ = true; 

        // If this node is configured with an external router SSID,
        // then connecting to a "parent" means it has connected to that router
        // and should therefore consider itself a root.
        if (!current_config_.wifi_ssid.empty()) {
            this->is_root_ = true;
            ESP_LOGI(MESH_TAG, "MESH_EVENT_PARENT_CONNECTED: Node is configured as ROOT and connected to router. Waiting for IP_EVENT_STA_GOT_IP on WIFI_STA_DEF.");
            // The DHCP client on WIFI_STA_DEF (created by mesh_netifs_init) should obtain an IP.
            // No need to call mesh_netifs_start() here for the root's external connection.
            // The WIFI_STA_DEF netif is already active and trying to get an IP.
        } else {
            // This node is NOT configured with an external router, so any parent connection
            // means it's a child connecting to another mesh node.
            this->is_root_ = false;
            ESP_LOGI(MESH_TAG, "MESH_EVENT_PARENT_CONNECTED: Node is CHILD (not configured for external router). Starting MESH_STA_DEF for IP acquisition from mesh root.");
            ESP_ERROR_CHECK(mesh_netifs_start(false)); // This configures netif_sta as MESH_STA_DEF
        }
        break;
    }
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected_event = (mesh_event_disconnected_t *)event_data;
        ESP_LOGW(MESH_TAG, "MESH_EVENT_PARENT_DISCONNECTED from BSSID: " MACSTR ", Reason: %d",
                 MAC2STR(disconnected_event->bssid), disconnected_event->reason);
        mesh_connected_ = false; // No longer connected to that parent
        ip_acquired_ = false;    // Lost IP that might have been obtained via that parent
        // is_root_ remains false, mesh will try to find a new parent or self-organize.
        // If it becomes root, MESH_EVENT_STARTED will be triggered again.
        // m_mqtt_handler_.stop(); 
        break;
    }
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change_event = (mesh_event_layer_change_t *)event_data;
        ESP_LOGI(MESH_TAG, "MESH_EVENT_LAYER_CHANGE: New layer: %d", layer_change_event->new_layer);
        break;
    }
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *addr_event = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(MESH_TAG, "MESH_EVENT_ROOT_ADDRESS: Root has address on mesh: " IPSTR, IP2STR(&(addr_event->mip.ip4)));
        // This event is informational for child nodes.
        // Root status is primarily determined by esp_mesh_is_root() and external IP for gateway roots.
        break;
    }
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGW(MESH_TAG, "MESH_EVENT_NO_PARENT_FOUND. ESP-MESH will keep trying to find a parent.");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        ESP_LOGI(MESH_TAG, "MESH_EVENT_ROOT_SWITCH_REQ: Root switch requested.");
        // The mesh stack handles this automatically.
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        ESP_LOGI(MESH_TAG, "MESH_EVENT_ROOT_SWITCH_ACK: Root switch acknowledged.");
        this->is_root_ = esp_mesh_is_root(); // Re-evaluate root status
        ESP_LOGI(MESH_TAG, "Root switch ACK. Current is_root status: %s", this->is_root_ ? "true" : "false");
        break;
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
        const char* if_desc = esp_netif_get_desc(event->esp_netif); 
        ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP: Interface Key: '%s', Desc: '%s', IP: " IPSTR, 
                 if_key, if_desc ? if_desc : "N/A", IP2STR(&event->ip_info.ip));
        
        // Check if the event is for the WIFI_STA_DEF interface (external connection)
        // AND if the node believes it's a root (set in MESH_EVENT_PARENT_CONNECTED for router connections)
        if (strcmp(if_key, "WIFI_STA_DEF") == 0 && this->is_root_) { 
            ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP for ROOT on WIFI_STA_DEF (external).");
            ip_acquired_ = true;     

            esp_netif_dns_info_t dns_info;
            uint32_t dns_server_addr = 0;
            if (esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
                dns_server_addr = dns_info.ip.u_addr.ip4.addr;
                ESP_LOGI(MESH_TAG, "Using DNS Server " IPSTR " for mesh children.", IP2STR(&dns_info.ip.u_addr.ip4));
            } else {
                ESP_LOGW(MESH_TAG, "Failed to get DNS info for root's external STA. Using 0.0.0.0 for children's DNS.");
            }
            
            ESP_LOGI(MESH_TAG, "Root has external IP. Starting mesh AP netif (MESH_AP_DEF).");
            ESP_ERROR_CHECK(mesh_netif_start_root_ap(true, dns_server_addr)); 

            ESP_LOGI(MESH_TAG, "Root (external IP OK, mesh AP started). Starting MQTT client.");
            if (!current_config_.mqtt_broker_uri.empty()) {
                m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }
            initialize_sntp_task(); // Initialize SNTP once root has external connectivity

        } else if (strcmp(if_key, "MESH_STA_DEF") == 0 && !this->is_root_) { 
            ESP_LOGI(MESH_TAG, "IP_EVENT_STA_GOT_IP for CHILD on MESH_STA_DEF (internal mesh IP).");
            ip_acquired_ = true;     

            ESP_LOGI(MESH_TAG, "Node (child) got IP on mesh interface. Starting MQTT client (if configured).");
            if (!current_config_.mqtt_broker_uri.empty()) {
                 m_mqtt_handler_.start(current_config_.mqtt_broker_uri.c_str());
            }
            initialize_sntp_task(); // Initialize SNTP for child if it needs time
        } else {
            ESP_LOGW(MESH_TAG, "IP_EVENT_STA_GOT_IP on an unexpected interface/role combination. Key: '%s', Desc: '%s', is_root_: %s", 
                     if_key, if_desc ? if_desc : "N/A", this->is_root_ ? "true" : "false");
        }

    } else if (event_id == IP_EVENT_STA_LOST_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data; 
        const char* if_key_lost = event ? esp_netif_get_ifkey(event->esp_netif) : "UNKNOWN_IF";
        ESP_LOGW(MESH_TAG, "IP_EVENT_STA_LOST_IP on interface %s", if_key_lost);
        
        if (strcmp(if_key_lost, "WIFI_STA_DEF") == 0 && this->is_root_) {
            ESP_LOGW(MESH_TAG, "Lost IP on external STA interface. External connectivity lost for root.");
            // ip_acquired_ for external connection is effectively false.
            // is_root_ might still be true for the mesh, but it's no longer a gateway root.
            // The mesh might elect a new root or this node might continue as a non-gateway root.
            // m_mqtt_handler_.stop(); // If MQTT relies on external IP
        } else if (strcmp(if_key_lost, "MESH_STA_DEF") == 0 && !this->is_root_) {
             ESP_LOGW(MESH_TAG, "Lost IP on MESH_STA interface (child node).");
             ip_acquired_ = false; // Lost mesh IP
             // m_mqtt_handler_.stop();
        }
    } else if (event_id == IP_EVENT_AP_STAIPASSIGNED) { 
        if (this->is_root_) { 
            ip_event_ap_staipassigned_t* assigned_event = (ip_event_ap_staipassigned_t*)event_data;
            ESP_LOGI(MESH_TAG, "Root's Mesh AP (on MESH_AP_DEF) assigned IP: " IPSTR " to a station.", IP2STR(&assigned_event->ip));
        }
    }
}

// --- Interface Methods (publish, subscribe, etc. from your existing code) ---
// ... (Keep your existing publish, subscribe, unsubscribe, isConnected, update, getDeviceId, isRootNode methods) ...
// Ensure isConnected() logic is robust based on mesh_connected_, ip_acquired_, and m_mqtt_handler_.is_disconnected()
bool EspMeshHandler::isConnected() const {
    if (!mesh_initialized_ || !mesh_connected_) {
        ESP_LOGD(MESH_TAG, "isConnected: Basic mesh not up (initialized: %d, connected: %d)", mesh_initialized_, mesh_connected_);
        return false;
    }

    // If MQTT is not configured, connection status depends only on mesh and IP status
    if (current_config_.mqtt_broker_uri.empty()) {
        ESP_LOGD(MESH_TAG, "isConnected (No MQTT URI): IP acquired: %d", ip_acquired_);
        return ip_acquired_; // Needs an IP to be considered connected within the mesh
    }

    // If MQTT is configured, it must also be connected
    bool mqtt_ok = (m_mqtt_handler_.is_disconnected() == 0);
    ESP_LOGD(MESH_TAG, "isConnected (With MQTT URI): IP acquired: %d, MQTT OK: %d", ip_acquired_, mqtt_ok);
    return ip_acquired_ && mqtt_ok;
}

bool EspMeshHandler::publish(const std::string& topic, const void* data, size_t length, bool retain, int qos) {
    if (!isConnected()) {
        ESP_LOGW(MESH_TAG, "Cannot publish to '%s', not fully connected. Mesh: %d, IP: %d, MQTT Connected: %s", 
                 topic.c_str(), mesh_connected_, ip_acquired_, (m_mqtt_handler_.is_disconnected() == 0) ? "yes" : "no");
        return false;
    }
    m_mqtt_handler_.publish_to(topic, data, length, retain, qos);
    return true;
}

bool EspMeshHandler::subscribe(const std::string &topic, comm_message_callback_t callback, int qos) {
    ESP_LOGI(MESH_TAG, "Subscribing to topic: %s", topic.c_str());
    if (m_mqtt_handler_.is_disconnected() == 255) { // 255 means not started
        ESP_LOGE(MESH_TAG, "MQTT handler not started, cannot subscribe to topic: %s", topic.c_str());
        return false;
    }
     if (m_mqtt_handler_.is_disconnected() == 1 && !ip_acquired_) { // Started but not connected, and no IP
        ESP_LOGW(MESH_TAG, "MQTT handler started but not connected, and no IP yet. Subscription to %s might fail or be delayed.", topic.c_str());
        // Proceed with subscription attempt, MQTT client should handle queuing if capable
    }


    comm_message_callback_t captured_user_callback = std::move(callback);
    std::string topic_filter_for_lambda = topic;

    Xasin::MQTT::mqtt_callback xasin_callback =
        [this, captured_user_callback, topic_filter_for_lambda](const Xasin::MQTT::MQTT_Packet& packet) {
        CommReceivedData received_data;
        received_data.topic = topic_filter_for_lambda; 
        received_data.payload = std::vector<uint8_t>(packet.data.begin(), packet.data.end());
        
        std::string source_id_str = "";
        std::string full_topic_str = packet.topic; 
        
        size_t component_start_pos = 0;
        if (!full_topic_str.empty() && full_topic_str[0] == '/') {
            component_start_pos = 1; 
        }

        std::string path_to_parse = full_topic_str.substr(component_start_pos);
        std::string segment;
        std::vector<std::string> segments; 
        std::istringstream segment_stream(path_to_parse); 

        while(std::getline(segment_stream, segment, '/')) {
            segments.push_back(segment);
        }

        if (segments.size() >= 3) { // Assuming /prefix/project/device_id/...
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
        active_subscriptions_[topic] = {sub, captured_user_callback}; // Store the original callback
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
        delete it->second.xasin_sub; // Destructor should handle MQTT unsubscribe
        active_subscriptions_.erase(it);
        return true;
    } else {
        ESP_LOGW(MESH_TAG, "Not subscribed to topic: %s", topic.c_str());
        return false;
    }
}

void EspMeshHandler::update() {
    // Typically not needed for event-driven mesh/mqtt
}

std::string EspMeshHandler::getDeviceId() {
    if (device_id_.empty()) {
        device_id_ = get_device_mac_string(); 
    }
    return device_id_;
}

bool EspMeshHandler::isRootNode() const {
    return is_root_;
}


} // namespace Communication
} // namespace Xasin
