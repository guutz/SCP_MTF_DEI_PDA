/*
 * animatorThread.cpp
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#include "lzrtag/animatorThread.h"

// Required full definitions for dependencies and owned objects
#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/weapon/base_weapon.h"
#include "xasin/BatteryManager.h"
#include "xasin/mqtt/Handler.h"

#include "lzrtag/mcp_access.h"      // For lzrtag_get_trigger, lzrtag_set_vibrate_motor
#include "lzrtag/vibrationHandler.h" // For vibrator_tick()

#include <cmath> // For sinf, powf, or C versions math.h
#include "esp_log.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

namespace LZR {

// Constructor
Animator::Animator(
    LZR::Player* player_ptr,
    LZRTag::Weapon::Handler* weapon_handler_ptr,
    std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_ptr,
    Housekeeping::BatteryManager* battery_ptr,
    Xasin::MQTT::Handler* mqtt_ptr
) : player_(player_ptr),
    weapon_handler_(weapon_handler_ptr),
    weapons_(weapons_ptr),
    battery_(battery_ptr),
    mqtt_(mqtt_ptr),
    animation_task_handle_(nullptr),
    vibr_motor_count_old_(0)
{
    ESP_LOGI("Animator", "Animator initialized");
}

// Destructor
Animator::~Animator() {
    if (animation_task_handle_ != nullptr) {
        vTaskDelete(animation_task_handle_);
        animation_task_handle_ = nullptr;
    }

    ESP_LOGI("Animator", "Animator destroyed");
}

// Public method to start the animation task
void Animator::start_animation_task() {
    xTaskCreatePinnedToCore(
        Animator::animation_task_entry, // Static entry function
        "AnimatorTask",               // Task name
        4 * 1024,                       // Stack size
        this,                           // Parameter (pointer to this instance)
        10,                             // Priority
        &animation_task_handle_,        // Task handle
        1                               // Core ID
    );
    ESP_LOGI("Animator", "Animation task started");
}

// Static task entry function (Trampoline)
void Animator::animation_task_entry(void *instance) {
    if (instance != nullptr) {
        static_cast<Animator*>(instance)->animation_task_runner();
    }
}

// Main animation loop (replaces old animation_thread free function)
void Animator::animation_task_runner() {
    ESP_LOGI("Animator", "Animation task runner started");
    while (true) {
        if (!player_ || !weapon_handler_ || !weapons_ || !battery_ ) {
            ESP_LOGE("Animator", "Core dependency missing in task runner!");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int g_num = player_->get_gun_num();
        if (g_num > 0 && g_num <= static_cast<int>(weapons_->size())) {
            weapon_handler_->set_weapon((*weapons_)[g_num-1]); // g_num is 1-indexed
        }

        weapon_handler_->update_btn(!lzrtag_get_trigger());
        weapon_handler_->fx_tick();

        if (player_->should_reload) {
            weapon_handler_->tempt_reload();
            player_->should_reload = false;
        }
        
        auto ammo_info = weapon_handler_->get_ammo();
        player_->set_gun_ammo(ammo_info.current_ammo, ammo_info.clipsize, ammo_info.total_ammo);


        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
}


void Animator::vibr_motor_tick_internal() {
    // Logic from old vibr_motor_tick()
    // Use member variables: weapon_handler_, player_, vibr_motor_count_old_
    if (!weapon_handler_ || !player_) return; // Guard

    // vibrator_tick(); // Assuming this is a global or static function from vibrationHandler.h
    // This function seems to be empty or not doing what's expected based on its name.
    // The actual vibration logic is below.

    // The old code had a 'return;' here, effectively disabling the rest.
    // If that's intentional, keep it. Otherwise, remove to enable vibration.
    // return; 

    vibr_motor_count_old_++;
    bool vibrOn = false;

    if((xTaskGetTickCount() - weapon_handler_->get_last_shot_tick()) < pdMS_TO_TICKS(60))
        vibrOn = true;
    else if(player_->is_hit() && player_->is_dead())
        vibrOn = (vibr_motor_count_old_ & 0b1);
    else if(player_->is_hit())
        vibrOn = (vibr_motor_count_old_ & 0b1010) == 0;
    else if(player_->get_heartbeat())
        vibrOn = ((0b101 & (xTaskGetTickCount()/75)) == 0);
    else if(player_->should_vibrate())
        vibrOn = true;

    lzrtag_set_vibrate_motor(vibrOn);
}

}