#include "setup.h"
#include "laser_tag.h"
#include "cd4053b_wrapper.h"
#include "esp_log.h"
#include "mcp23008_wrapper.h"
#include "xasin/audio.h"
#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/weapon/shot_weapon.h"
#include "lzrtag/weapon/heavy_weapon.h"
#include "lzrtag/weapon/beam_weapon.h"
#include "lzrtag/weapon_defs.h"
#include "xasin/mqtt/Handler.h" // For MQTT handler
#include "lzrtag/mcp_access.h"
#include <cstring>
#include <vector> 

#include "lzrtag/animatorThread.h" 
#include "driver/ledc.h"
#include "lzrtag/vibrationHandler.h" // If needed, ensure it's available

#include "cJSON.h"
#include "persistent_state.h"

static const char *TAG_LASER = "laser_tag";

mcp23008_t gun_gpio_extender = {
    .port = I2C_NUM_0,
    .address = 0x27, // Gun MCP23008 address
    .current = 0
};

// --- LZRTag Game State (real types) ---

namespace LaserTagGame {
    LZR::Player* player = nullptr;
    LZRTag::Weapon::Handler* weaponHandler = nullptr;
    std::vector<LZRTag::Weapon::BaseWeapon*> weapons;
    TaskHandle_t initPlayerTask = nullptr;
    TaskHandle_t housekeepingTask = nullptr;

    Housekeeping::BatteryManager battery; // Default constructor

    // Forward declare internal helper for ping
    static void send_ping_req_internal();
}


void gun_mux_switch_init(void) {
    if (mcp23008_check_present(&gun_gpio_extender) == ESP_OK) {
        ESP_LOGI(TAG_LASER, "Gun MCP23008 detected at 0x27. Initializing...");
        ESP_ERROR_CHECK(mcp23008_wrapper_init_with_defaults(&gun_gpio_extender, MCP23008_2_DEFAULT_IODIR, MCP23008_2_DEFAULT_GPPU));
        ESP_LOGI(TAG_LASER, "Initializing gun mux switch (CD4053B) paths.");
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S2_PATH_B0_IR_RX);
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S1_PATH_A0_ACCESSORY_1);
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S3_PATH_C0_IR_TX);
        ESP_ERROR_CHECK(mcp23008_wrapper_set_pin_direction(&gun_gpio_extender, (MCP23008_NamedPin)MCP2_PIN_HAPTIC_MOTOR, MCP_PIN_DIR_OUTPUT));
    } else {
        ESP_LOGW(TAG_LASER, "Gun MCP23008 not detected at 0x27. Skipping init.");
    }
}

static void housekeeping_thread(void* args) {
    TickType_t nextHWTick = xTaskGetTickCount();
    const TickType_t pingIntervalTicks = 1800; // From setup.cpp (approx 1.8 seconds if Tick is 1ms)

    while (true) {
        if (xTaskGetTickCount() >= nextHWTick) {
            
            LaserTagGame::send_ping_req_internal();
            nextHWTick += pingIntervalTicks;
        }
        
        // Original delay + any other game logic
        vTaskDelay(pdMS_TO_TICKS(100)); // Reduced delay to allow more frequent checks if needed, adjust as necessary
    }
}

// Internal function to send ping request
static void LaserTagGame::send_ping_req_internal() {
    if (mqtt.is_disconnected()) return; 
	uint32_t outData = xTaskGetTickCount();

	mqtt.publish_to("ping_signal", &outData, sizeof(outData), false, 0);
    ESP_LOGI(TAG_LASER, "Ping request sent via mqtt.");
}

void LaserTagGame::setup_ping_handling() {
    // Ensure LaserTagGame::mqtt (mqtt) is initialized and connected before subscribing.
    // This is typically handled by the main mesh initialization logic.
    if (!mqtt.is_disconnected()) {

        mqtt.subscribe_to("ping_signal",
            [](const Xasin::MQTT::MQTT_Packet &message) { // Corrected callback signature
            
            // Access topic and payload from the message object
            const std::string& topic = message.topic; // Assuming message.topic is std::string, if needed
            
            const char* payload_data = message.data.data(); // Assuming message.data is std::string 
            size_t payload_size = message.data.size(); // Assuming message.data is std::string

            auto system_info = cJSON_CreateObject();
            auto battery_json = cJSON_AddObjectToObject(system_info, "battery");

            cJSON_AddNumberToObject(battery_json, "mv", LaserTagGame::battery.current_mv());
            cJSON_AddNumberToObject(battery_json, "percentage", LaserTagGame::battery.current_capacity());

            if (payload_size >= sizeof(uint32_t)) {
                 uint32_t sent_ticks = 0;
                 memcpy(&sent_ticks, payload_data, sizeof(uint32_t)); 
                 cJSON_AddNumberToObject(system_info, "ping_ticks", xTaskGetTickCount() - sent_ticks);
            }
            cJSON_AddNumberToObject(system_info, "heap", esp_get_free_heap_size());

            char * json_print = cJSON_PrintUnformatted(system_info); 
            cJSON_Delete(system_info);

            if (json_print) {
                mqtt.publish_to("get/_ping", json_print, strlen(json_print), false, 1);
                cJSON_free(json_print);
            }
        });
        ESP_LOGI(TAG_LASER, "Ping handling (subscription via mqtt) initialized.");
    } else {
        ESP_LOGE(TAG_LASER, "Failed to initialize ping handling: mqtt not connected.");
    }
}

// --- LZRTag mode entry point ---
bool laser_tag_mode_enter(void) {
    // Load player info using MenuPersistentState
    PersistentState::MenuPersistentState menuState;

    if (!g_device_id.empty()) {
        ESP_LOGI(TAG_LASER, "Loaded device_id: %s", g_device_id.c_str());
    } else {
        g_device_id = "0";
        ESP_LOGW(TAG_LASER, "Failed to load player info, using default device_id=0");
    }

    if (mcp23008_check_present(&gun_gpio_extender) != ESP_OK) {
        ESP_LOGW(TAG_LASER, "Cannot enter LZRTag mode: Gun MCP23008 not detected!");
        return false;
    }
    gun_mux_switch_init(); // This was already here, but ensure it's before lzrtag_set_mcp23008_instance
    lzrtag_set_mcp23008_instance(&gun_gpio_extender);

    // --- Game logic setup ---
    LaserTagGame::init(); // Sets up LaserTagGame::mqtt (mqtt) and player init task

    LaserTagGame::setup_ping_handling(); // Setup MQTT for pings

    // Construct Weapon Handler with Audio, CommHandler (mqtt), and Player instance
    if (!LaserTagGame::player) {
        ESP_LOGE(TAG_LASER, "Player not initialized before weapon handler creation!");
        return false;
    }
    LaserTagGame::weaponHandler = new LZRTag::Weapon::Handler(g_sound_manager, &mqtt, LaserTagGame::player);

    // Initialize IR system within the weapon handler
    LaserTagGame::weaponHandler->init_ir_system();

    LaserTagGame::weaponHandler->can_shoot_func = []() {
        return LaserTagGame::player ? LaserTagGame::player->can_shoot() : false;
    };
    LaserTagGame::weaponHandler->on_shot_func = []() { 
        LaserTagGame::weaponHandler->send_ir_signal(); 
    };

    LaserTagGame::weaponHandler->start_thread();

    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, colibri_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, whip_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, steelfinger_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, sw_554_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::HeavyWeapon(*LaserTagGame::weaponHandler, nico_6_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::BeamWeapon(*LaserTagGame::weaponHandler, scalpel_cfg));
    
    // Housekeeping task (now includes ping logic)
    if (!LaserTagGame::housekeepingTask) { // Create only if not already running
        xTaskCreate(housekeeping_thread, "Housekeeping", 4096, nullptr, 5, &LaserTagGame::housekeepingTask);
    }

    // From setup.cpp: LZR::FX::target_mode = LZR::PLAYER_DECIDED; after a delay
    // This could be handled by a delayed task or a state machine within the game logic
    // For now, setting it after a short delay as in original setup()
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay for systems to settle

    ESP_LOGI(TAG_LASER, "LZRTag mode entered and systems initialized.");
    return true;
}

// --- LZRTag mode exit/teardown ---
void laser_tag_mode_exit(void) {
    if (LaserTagGame::initPlayerTask) {
        vTaskDelete(LaserTagGame::initPlayerTask);
        LaserTagGame::initPlayerTask = nullptr;
    }

    if (LaserTagGame::housekeepingTask) {
        vTaskDelete(LaserTagGame::housekeepingTask);
        LaserTagGame::housekeepingTask = nullptr;
    }
    // Orderly shutdown of systems
    if (LaserTagGame::weaponHandler) {
        LaserTagGame::weaponHandler->shutdown_ir_system(); // Call new shutdown method
    }

    // Original cleanup
    delete LaserTagGame::player; LaserTagGame::player = nullptr;
    delete LaserTagGame::weaponHandler; LaserTagGame::weaponHandler = nullptr;
    for (auto* w : LaserTagGame::weapons) delete w;
    LaserTagGame::weapons.clear();

    ESP_LOGI(TAG_LASER, "LZRTag mode exited.");
}

void LaserTagGame::init_player_task(void *param) {
    ESP_LOGI(TAG_LASER, "Player initialization task started.");

    // Wait for the mesh handler to be started
    // while (!mqtt.isConnected()) {
    //     ESP_LOGI(TAG_LASER, "Waiting for EspMeshHandler to connect...");
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
    // ESP_LOGI(TAG_LASER, "EspMeshHandler is connected. Initializing player.");

    // Use g_device_id instead of hardcoded "0"
    LaserTagGame::player = new LZR::Player(g_device_id, mqtt);
    LaserTagGame::player->init();
    ESP_LOGI(TAG_LASER, "Player object initialized.");

    // Now that player is initialized, if weaponHandler was created and waiting, it can be fully utilized.
    // However, weaponHandler is created in laser_tag_mode_enter, which might be called before this task completes.
    // Consider a state or flag to ensure player is ready before weaponHandler fully operates if there are race conditions.

    // Task finished, delete itself
    vTaskDelete(NULL);
}

void LaserTagGame::init() {
    ESP_LOGI(TAG_LASER, "LaserTagGame::init() called.");

    // Create a task to initialize the player
    // This is done because player initialization might depend on network connectivity
    // which is established by mqtt, and mqtt.start() is called
    // from wifi_init_task which runs separately.
    xTaskCreate(LaserTagGame::init_player_task, "init_player_task", 4096, NULL, 5, &initPlayerTask);
    ESP_LOGI(TAG_LASER, "Player initialization task created.");
}

// Call this from your animation loop or main tick
void LaserTagGame::tick() {
    if (LaserTagGame::player) {
        LaserTagGame::player->tick();
    }
}
