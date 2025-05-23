#ifndef LASER_TAG_H
#define LASER_TAG_H

#include "mcp23008_wrapper.h"
#include <stdbool.h>
#include <stdint.h>
#include <vector>
#include "lzrtag/PatternModeHandler.h"
#include "xasin/BatteryManager.h" 
#include "xasin/neocontroller.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/animatorThread.h"
#include "lzrtag/player.h"
#include "lzrtag/core_defs.h" // Added
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// LZRTag mode entry/exit
bool laser_tag_mode_enter(void);
void laser_tag_mode_exit(void);

// Extern for the gun MCP23008 device
extern mcp23008_t gun_gpio_extender;


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus


extern Xasin::Communication::EspMeshHandler g_mesh_handler;
extern Xasin::Audio::TX audioManager;
void gun_mux_switch_init(void);

namespace LaserTagGame {
    extern LZR::Player* player;
    extern LZRTag::Weapon::Handler* weaponHandler;
    extern std::vector<LZRTag::Weapon::BaseWeapon*> weapons;
    extern TaskHandle_t housekeepingTask;
    extern LZR::Animator* animator; // Added

    // Added from setup.h/cpp
    extern LZRTag_CORE_WEAPON_STATUS main_weapon_status; // Changed to use new enum
    extern Housekeeping::BatteryManager battery;
    extern Xasin::NeoController::NeoController* rgbController;
    extern TaskHandle_t audioProcessingTask;

    void init();
    void init_player_task(void *param);

    // New system management functions
    void setup_audio_system();
    void shutdown_audio_system();
    void setup_effects_system();
    void shutdown_effects_system();
    void setup_ping_handling();
    void shutdown_ping_handling();
    extern PatternModeHandler* patternModeHandler;

    void tick(); // Call this from your animation loop or main tick
}

extern std::string g_device_id;

#endif

#endif // LASER_TAG_H
