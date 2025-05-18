#include "laser_tag.h"
#include "cd4053b_wrapper.h"
#include "esp_log.h"
#include "mcp23008_wrapper.h"
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
    } else {
        ESP_LOGW(TAG_LASER, "Gun MCP23008 not detected at 0x21. Skipping init.");
    }
}

// --- LZRTag Game State (scaffold) ---
namespace LaserTagGame {
    // Forward declarations for types from LZRTag repo
    class Player;
    namespace Weapon {
        class Handler;
        class BaseWeapon;
    }
    // Game state objects (to be filled in with actual types/constructors)
    static Player* player = nullptr;
    static Weapon::Handler* weaponHandler = nullptr;
    static std::vector<Weapon::BaseWeapon*> weapons;
    static TaskHandle_t housekeepingTask = nullptr;
    // ...add more as needed (audio, animation, etc.)
}

// --- LZRTag mode entry point ---
bool laser_tag_mode_enter(void) {
    if (mcp23008_check_present(&gun_gpio_extender) != ESP_OK) {
        ESP_LOGW(TAG_LASER, "Cannot enter LZRTag mode: Gun MCP23008 not detected!");
        return false;
    }
    gun_mux_switch_init();
    // --- Game logic setup ---
    // TODO: Initialize player, weapon handler, weapons, etc.
    // TODO: Start housekeeping and other game tasks
    // Example:
    // LaserTagGame::player = new Player(...);
    // LaserTagGame::weaponHandler = new Weapon::Handler(...);
    // xTaskCreate(housekeeping_thread, ...);
    ESP_LOGI(TAG_LASER, "LZRTag mode entered.");
    return true;
}

// --- LZRTag mode exit/teardown ---
void laser_tag_mode_exit(void) {
    // TODO: Stop/delete tasks, clean up objects, reset state
    // Example:
    // if (LaserTagGame::housekeepingTask) vTaskDelete(LaserTagGame::housekeepingTask);
    // delete LaserTagGame::player; LaserTagGame::player = nullptr;
    // delete LaserTagGame::weaponHandler; LaserTagGame::weaponHandler = nullptr;
    // for (auto* w : LaserTagGame::weapons) delete w;
    // LaserTagGame::weapons.clear();
    ESP_LOGI(TAG_LASER, "LZRTag mode exited.");
}
