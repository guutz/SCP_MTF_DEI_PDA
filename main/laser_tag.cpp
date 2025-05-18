#include "laser_tag.h"
#include "cd4053b_wrapper.h"
#include "esp_log.h"
#include "mcp23008_wrapper.h"
#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/weapon/shot_weapon.h"
#include "lzrtag/weapon/heavy_weapon.h"
#include "lzrtag/weapon/beam_weapon.h"
#include "xasin/mqtt/Handler.h"
#include "lzrtag/mcp_access.h"
#include <cstring>
#include <vector> // Include vector for std::vector usage

mcp23008_t gun_gpio_extender = {
    .port = I2C_NUM_0,
    .address = 0x21, // Gun MCP23008 address
    .current = 0
};

static const char *TAG_LASER = "laser_tag";

void gun_mux_switch_init(void) {
    if (mcp23008_check_present(&gun_gpio_extender) == ESP_OK) {
        ESP_LOGI(TAG_LASER, "Gun MCP23008 detected at 0x21. Initializing...");
        ESP_ERROR_CHECK(mcp23008_wrapper_init_with_defaults(&gun_gpio_extender, MCP23008_2_DEFAULT_IODIR, MCP23008_2_DEFAULT_GPPU));
        ESP_LOGI(TAG_LASER, "Initializing gun mux switch (CD4053B) paths.");
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S2_PATH_B0_IR_RX);
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S1_PATH_A0_ACCESSORY_1);
        cd4053b_select_gun_path(&gun_gpio_extender, GUN_MUX_S3_PATH_C0_IR_TX);
        ESP_ERROR_CHECK(mcp23008_wrapper_set_pin_direction(&gun_gpio_extender, (MCP23008_NamedPin)MCP2_PIN_HAPTIC_MOTOR, MCP_PIN_DIR_OUTPUT));
    } else {
        ESP_LOGW(TAG_LASER, "Gun MCP23008 not detected at 0x21. Skipping init.");
    }
}

// --- LZRTag Game State (real types) ---

namespace LaserTagGame {
    LZR::Player* player = nullptr;
    LZRTag::Weapon::Handler* weaponHandler = nullptr;
    std::vector<LZRTag::Weapon::BaseWeapon*> weapons;
    Xasin::MQTT::Handler* mqtt = nullptr;
    TaskHandle_t housekeepingTask = nullptr;
    // ...add more as needed (animator, etc.)
}

// Example housekeeping thread stub (implement as needed)
static void housekeeping_thread(void* args) {
    while (true) {
        // TODO: Add housekeeping/game logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// --- LZRTag mode entry point ---
bool laser_tag_mode_enter(void) {
    if (mcp23008_check_present(&gun_gpio_extender) != ESP_OK) {
        ESP_LOGW(TAG_LASER, "Cannot enter LZRTag mode: Gun MCP23008 not detected!");
        return false;
    }
    gun_mux_switch_init();
    lzrtag_set_mcp23008_instance(&gun_gpio_extender);

    // --- Game logic setup ---
    LaserTagGame::mqtt = new Xasin::MQTT::Handler();
    LaserTagGame::player = new LZR::Player("", *LaserTagGame::mqtt);
    LaserTagGame::weaponHandler = new LZRTag::Weapon::Handler();
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, colibri_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, whip_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, steelfinger_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::ShotWeapon(*LaserTagGame::weaponHandler, sw_554_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::HeavyWeapon(*LaserTagGame::weaponHandler, nico_6_config));
    LaserTagGame::weapons.push_back(new LZRTag::Weapon::BeamWeapon(*LaserTagGame::weaponHandler, scalpel_cfg));
    xTaskCreate(housekeeping_thread, "Housekeeping", 3*1024, nullptr, 10, &LaserTagGame::housekeepingTask);
    ESP_LOGI(TAG_LASER, "LZRTag mode entered.");
    return true;
}

// --- LZRTag mode exit/teardown ---
void laser_tag_mode_exit(void) {
    if (LaserTagGame::housekeepingTask) {
        vTaskDelete(LaserTagGame::housekeepingTask);
        LaserTagGame::housekeepingTask = nullptr;
    }
    delete LaserTagGame::player; LaserTagGame::player = nullptr;
    delete LaserTagGame::weaponHandler; LaserTagGame::weaponHandler = nullptr;
    delete LaserTagGame::mqtt; LaserTagGame::mqtt = nullptr;
    for (auto* w : LaserTagGame::weapons) delete w;
    LaserTagGame::weapons.clear();
    ESP_LOGI(TAG_LASER, "LZRTag mode exited.");
}
