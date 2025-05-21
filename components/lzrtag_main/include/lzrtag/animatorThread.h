/*
 * animatorThread.h
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#ifndef MAIN_FX_ANIMATORTHREAD_H_
#define MAIN_FX_ANIMATORTHREAD_H_

#include "xasin/neocontroller.h" // Full definition needed for Animator::ColorSet
#include "lzrtag/LZRConfig.h"
#include "lzrtag/core_defs.h" // Includes LZRTag_CORE_WEAPON_STATUS and common forward declarations
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector> // For std::vector

#include "lzrtag/patterns/ShotFlicker.h"
#include "lzrtag/patterns/VestPattern.h"
#include "lzrtag/patterns/BasePattern.h"
#include "lzrtag/pattern_types.h"

// Specific forward declarations not in core_defs.h (if any) would go here.
// For now, core_defs.h should cover the ones we need for Animator class pointers.

#define VEST_LEDS WS2812_NUMBER-1 // Consider if this should be a class member or constructor param

namespace LZR {

using namespace Xasin::NeoController;

struct ColorSet {
	Color muzzleFlash;
	Color muzzleHeat;

	Color vestBase;
	Color vestShotEnergy;
};

struct FXSet {
    float minBaseGlow;
    float maxBaseGlow;
    float waverAmplitude;
    float waverPeriod;
    float waverPositionShift;
};

class Animator {
public:
    // Constructor and Destructor
    Animator(
        LZR::Player* player_ptr,
        LZRTag::Weapon::Handler* weapon_handler_ptr,
        std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_ptr,
        Xasin::NeoController::NeoController* rgb_controller_ptr,
        Housekeeping::BatteryManager* battery_ptr,
        Xasin::Communication::CommHandler* mqtt_ptr,
        LZRTag_CORE_WEAPON_STATUS* main_weapon_status_ptr
    );
    ~Animator();

    // Public interface
    void start_animation_task();
    void set_pattern_mode(LZR::pattern_mode_t mode); // Added
    const LZR::ColorSet& get_buffered_colors() const { return buffered_colors_; }
    LZR::ColorSet& get_buffered_colors() { return buffered_colors_; }

private:
    // Dependencies (pointers to external objects)
    LZR::Player* player_;
    LZRTag::Weapon::Handler* weapon_handler_;
    std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_;
    Xasin::NeoController::NeoController* rgb_controller_;
    Housekeeping::BatteryManager* battery_;
    Xasin::Communication::CommHandler* mqtt_;
    LZRTag_CORE_WEAPON_STATUS* main_weapon_status_;

    // Internal State
    ColorSet current_colors_;
    ColorSet buffered_colors_;
    FXSet current_fx_;
    FXSet buffered_fx_;

    // FX Patterns (owned by Animator)
    LZR::FX::ShotFlicker* vest_shot_pattern_;
    LZR::FX::VestPattern* vest_hit_marker_;
    LZR::FX::VestPattern* vest_death_marker_;
    LZR::FX::VestPattern* vest_marked_marker_;
    std::vector<LZR::FX::BasePattern*> vest_patterns_; // Stores pointers to the above patterns

    // Task Management
    TaskHandle_t animation_task_handle_;
    static void animation_task_entry(void *instance); // Static entry for FreeRTOS
    void animation_task_runner();                     // Instance method for task logic

    // Internal Helper Methods (formerly free functions or FX namespace functions)
    void setup_vest_patterns_internal();
    void status_led_tick_internal();
    void vibr_motor_tick_internal();
    void vest_tick_internal();
    
    void fx_init_internal();          // Manages initialization of FX system
    void fx_mode_tick_internal();     // Manages FX mode transitions and ticks

    // Internal FX state
    volatile pattern_mode_t fx_target_mode_; 
};

} /* namespace LZR */

#endif /* MAIN_FX_ANIMATORTHREAD_H_ */
