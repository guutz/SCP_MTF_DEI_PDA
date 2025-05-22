/*
 * animatorThread.cpp
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#include "lzrtag/core_defs.h"
#include "lzrtag/animatorThread.h"

// Required full definitions for dependencies and owned objects
#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/weapon/base_weapon.h"
#include "xasin/BatteryManager.h"
#include "CommHandler.h" // Corrected path

// FX Pattern includes
#include "lzrtag/patterns/ShotFlicker.h"
#include "lzrtag/patterns/VestPattern.h"
#include "lzrtag/patterns/BasePattern.h"

#include "lzrtag/mcp_access.h"      // For lzrtag_get_trigger, lzrtag_set_vibrate_motor
#include "lzrtag/colorSets.h" // For NUM_TEAM_COLORS and NUM_BRIGHTNESS_LEVELS
#include "lzrtag/vibrationHandler.h" // For VibrationHandler

#include "driver/ledc.h"
#include <cmath> // For sinf, powf, or C versions math.h
#include "esp_log.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// Define the global/static teamColors and brightnessLevels if they are not in colorSets.h
// For example:
// LZR::ColorSet teamColors[] = { /* ...initializers... */ };
// LZR::FXSet brightnessLevels[] = { /* ...initializers... */ };
// If they are in colorSets.h, ensure it's included and they are accessible.


namespace LZR {

// Static VibrationHandler pointer for Animator
static VibrationHandler* vibration_handler_ = nullptr;

// Constructor
Animator::Animator(
    LZR::Player* player_ptr,
    LZRTag::Weapon::Handler* weapon_handler_ptr,
    std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_ptr,
    Xasin::NeoController::NeoController* rgb_controller_ptr,
    Housekeeping::BatteryManager* battery_ptr,
    Xasin::Communication::CommHandler* mqtt_ptr,
    LZRTag_CORE_WEAPON_STATUS* main_weapon_status_ptr
) : player_(player_ptr),
    weapon_handler_(weapon_handler_ptr),
    weapons_(weapons_ptr),
    rgb_controller_(rgb_controller_ptr),
    battery_(battery_ptr),
    mqtt_(mqtt_ptr),
    main_weapon_status_(main_weapon_status_ptr),
    animation_task_handle_(nullptr),
    fx_target_mode_(OFF) // Default to OFF or some initial state
{
    // Initialize ColorSet and FXSet members (assuming teamColors and brightnessLevels are accessible)
    // If teamColors/brightnessLevels are not global/static, this needs adjustment
    // For example, if they are defined in lzrtag/colorSets.h and are global:
    if (LZR::NUM_TEAM_COLORS > 0) {
        current_colors_ = LZR::teamColors[0];
        buffered_colors_ = LZR::teamColors[0];
    }
    if (LZR::NUM_BRIGHTNESS_LEVELS > 0) {
        current_fx_ = LZR::brightnessLevels[0];
        buffered_fx_ = LZR::brightnessLevels[0];
    }


    // Instantiate FX Patterns
    // VEST_LEDS is defined in animatorThread.h
    vest_shot_pattern_ = new LZR::FX::ShotFlicker(VEST_LEDS, VEST_LEDS, weapon_handler_);
    // Fix: set_buffered_colors expects a non-const pointer, so remove const
    vest_shot_pattern_->set_buffered_colors(&buffered_colors_);
    vest_hit_marker_ = new LZR::FX::VestPattern();
    vest_death_marker_ = new LZR::FX::VestPattern();
    vest_marked_marker_ = new LZR::FX::VestPattern();

    vest_patterns_.push_back(vest_shot_pattern_);
    vest_patterns_.push_back(vest_hit_marker_);
    vest_patterns_.push_back(vest_death_marker_);
    vest_patterns_.push_back(vest_marked_marker_);

    // Call internal setup methods
    setup_vest_patterns_internal();

    // Instantiate VibrationHandler if not already
    if (!vibration_handler_)
        vibration_handler_ = new VibrationHandler(player_, weapon_handler_);

    ESP_LOGI("Animator", "Animator initialized");
}

// Destructor
Animator::~Animator() {
    if (animation_task_handle_ != nullptr) {
        vTaskDelete(animation_task_handle_);
        animation_task_handle_ = nullptr;
    }

    // Delete owned FX pattern objects
    // Safely delete, assuming vest_patterns_ only stores pointers also owned directly by members
    delete vest_shot_pattern_;
    delete vest_hit_marker_;
    delete vest_death_marker_;
    delete vest_marked_marker_;
    
    vest_patterns_.clear(); // Clear the vector of pointers

    // Delete VibrationHandler if it exists
    if (vibration_handler_) {
        delete vibration_handler_;
        vibration_handler_ = nullptr;
    }

    ESP_LOGI("Animator", "Animator destroyed");
}

// Public method to start the animation task
void Animator::start_animation_task() {
    xTaskCreatePinnedToCore(
        Animator::animation_task_entry, // Static entry function
        "AnimatorTask",               // Task name
        4096,                       // Stack size
        this,                           // Parameter (pointer to this instance)
        5,                             // Priority
        &animation_task_handle_,        // Task handle
        1                               // Core ID
    );
    ESP_LOGI("Animator", "Animation task started");
}

// Public method to set the pattern mode
void Animator::set_pattern_mode(LZR::pattern_mode_t mode) {
    fx_target_mode_ = mode;
    ESP_LOGI("Animator", "Pattern mode set to: %d", static_cast<int>(mode));
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
        if (!player_ || !weapon_handler_ || !weapons_ || !rgb_controller_ || !battery_ || !main_weapon_status_) {
            ESP_LOGE("Animator", "Core dependency missing in task runner!");
            ESP_LOGE("Animator", "Missing player: %p, weapon_handler: %p, weapons: %p, rgb_controller: %p, battery: %p, main_weapon_status: %p",
                     player_, weapon_handler_, weapons_, rgb_controller_, battery_, main_weapon_status_);
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

        status_led_tick_internal();

        if (*main_weapon_status_ == LZRTag_WPN_STAT_NOMINAL) {
            player_->tick();

            // Update current_colors_ and current_fx_ based on player state
            // This assumes teamColors and brightnessLevels are accessible arrays
            unsigned int team_idx = player_->get_team();
            if (team_idx < LZR::NUM_TEAM_COLORS) {
                 current_colors_ = LZR::teamColors[team_idx];
            }
            // Use get_brightness() accessor instead of direct member access
            unsigned int brightness_idx = static_cast<unsigned int>(player_->get_brightness());
            if (brightness_idx < LZR::NUM_BRIGHTNESS_LEVELS) {
                current_fx_ = LZR::brightnessLevels[brightness_idx];
            }
            vibr_motor_tick_internal();
        } else {
            lzrtag_set_vibrate_motor(false);
        }

        vest_tick_internal();

        // Muzzle color swap (r and g)
        Xasin::NeoController::Color newMuzzleColor = rgb_controller_->colors[0];
        Xasin::NeoController::Color actualMuzzle = Xasin::NeoController::Color();
        actualMuzzle.r = newMuzzleColor.g;
        actualMuzzle.g = newMuzzleColor.r;
        actualMuzzle.b = newMuzzleColor.b;
        actualMuzzle.alpha = newMuzzleColor.alpha; // Preserve alpha
        rgb_controller_->colors[0] = actualMuzzle;

        rgb_controller_->update();

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
}


// --- Internal Helper Methods ---

void Animator::setup_vest_patterns_internal() {
    if (!vest_hit_marker_ || !vest_death_marker_ || !vest_marked_marker_) return;

    vest_hit_marker_->pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
    vest_hit_marker_->pattern_p1_length = 1.3*255;
    vest_hit_marker_->pattern_period = 255*VEST_LEDS;
    vest_hit_marker_->pattern_p2_length = 255*VEST_LEDS;
    vest_hit_marker_->pattern_trap_percent = (1<<16) * 0.3;

    vest_hit_marker_->time_func = LZR::FX::time_func_t::LINEAR;
    vest_hit_marker_->timefunc_p1_period = 0.5*600;
    vest_hit_marker_->timefunc_period = vest_hit_marker_->timefunc_p1_period;

    vest_hit_marker_->overlayColor = Xasin::NeoController::Color(0xFFFFFF);
    vest_hit_marker_->overlayColor.alpha = 50;

    *vest_death_marker_ = *vest_hit_marker_;
    vest_death_marker_->pattern_period  = 2*255;
    vest_death_marker_->pattern_p2_length = 1*255;
    vest_death_marker_->overlayColor.alpha = 80;
    vest_death_marker_->timefunc_period = 0.2*600;
    vest_death_marker_->timefunc_p1_period = 0.2*600;

    vest_marked_marker_->pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
    vest_marked_marker_->pattern_period = 255 * (VEST_LEDS);
    vest_marked_marker_->pattern_p2_length = vest_marked_marker_->pattern_period - 255;
    vest_marked_marker_->pattern_p1_length = 2 * 255;
    vest_marked_marker_->pattern_trap_percent = 0.5 * (1 << 16);
    vest_marked_marker_->time_func = LZR::FX::time_func_t::TRAPEZ;
    vest_marked_marker_->timefunc_p1_period = 600 * 1.6;
    vest_marked_marker_->timefunc_period = vest_marked_marker_->timefunc_p1_period;
    vest_marked_marker_->timefunc_trap_percent = 0.9 * (1 << 16);
}

// Placeholder for old set_bat_pwr - this was a local static in old file,
// now part of status_led_tick_internal or a private helper if complex enough.
static void set_bat_pwr_leds(uint8_t level, uint8_t brightness = 255) {
    uint8_t gLevel = 255 - static_cast<uint8_t>(std::pow(255 - 2.55*level, 2)/255.0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255 - static_cast<uint8_t>((255-gLevel)*std::pow(brightness/255.0, 2)));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 255 - static_cast<uint8_t>(gLevel*std::pow(brightness/255.0, 2)));

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void Animator::status_led_tick_internal() {
    // Logic from old status_led_tick()
    // Use member variables: *main_weapon_status_, battery_, mqtt_
    if (!battery_ || !main_weapon_status_) return; // Guard

    float conIndB = 0;
    float batBrightness = 1;

    switch(*main_weapon_status_) {
    case LZRTag_WPN_STAT_CHARGING: {
        int cycleTime = xTaskGetTickCount() % 1200;
        batBrightness = 0.6f; // Always use a fixed brightness for CHARGING state
        break;
    }
    case LZRTag_WPN_STAT_DISCHARGED:
        if(xTaskGetTickCount()%300 < 150) {
            batBrightness = 0.1f;
            conIndB = 0.5f;
        }
    break;
    default: // NOMINAL or INITIALIZING
        if(mqtt_ && !mqtt_->isConnected()) 
            conIndB = (0.3f + 0.3f*sinf(xTaskGetTickCount()/2300.0f * M_PI)); // Used global sinf
        else if(mqtt_ && mqtt_->isConnected())
            conIndB = xTaskGetTickCount()%800 < 600 ? 1.0f : 0.5f;
        else // mqtt_ is null or is_disconnected is other value
            conIndB = xTaskGetTickCount()%800 < 400 ? 1.0f : 0.0f;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 255 - static_cast<uint8_t>(pow(conIndB ,2)*255.0f));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    set_bat_pwr_leds(battery_->current_capacity(), static_cast<uint8_t>(batBrightness*255.0f));
}

void Animator::vibr_motor_tick_internal() {
    // Use VibrationHandler for vibration logic only
    if (vibration_handler_) {
        vibration_handler_->vibrator_tick();
    }
}

// Define COLOR_FADE and FX_FADE macros if they are simple enough,
// or convert them to inline functions/methods.
// These were used in the old vest_tick.
// For simplicity, I'll replicate them here, but helper methods might be cleaner.
#define LZR_ANIMATOR_COLOR_FADE(buffered_color, current_color, alpha_divisor) \
    (buffered_color).merge_transition(current_color, alpha_divisor)
// FX_FADE was: bufferedFX.fxName = (bufferedFX.fxName * (1-alpha) + currentFX.fxName * alpha)
// This needs to be applied per-member of FXSet.

void Animator::vest_tick_internal() {
    if (!weapon_handler_ || !rgb_controller_ || !player_) return;

    // Vest color fading (using helper or direct merge_transition)
    buffered_colors_.muzzleFlash.merge_transition(current_colors_.muzzleFlash, 5000);
    buffered_colors_.muzzleHeat.merge_transition(current_colors_.muzzleHeat, 5000);
    buffered_colors_.vestBase.merge_transition(current_colors_.vestBase, 2000);
    buffered_colors_.vestShotEnergy.merge_transition(current_colors_.vestShotEnergy, 5000);

    // FX Fading - apply per member
    float fx_alpha = 0.05f;
    buffered_fx_.minBaseGlow = buffered_fx_.minBaseGlow * (1.0f - fx_alpha) + current_fx_.minBaseGlow * fx_alpha;
    buffered_fx_.maxBaseGlow = buffered_fx_.maxBaseGlow * (1.0f - fx_alpha) + current_fx_.maxBaseGlow * fx_alpha;
    buffered_fx_.waverAmplitude = buffered_fx_.waverAmplitude * (1.0f - fx_alpha) + current_fx_.waverAmplitude * fx_alpha;
    buffered_fx_.waverPeriod = buffered_fx_.waverPeriod * (1.0f - fx_alpha) + current_fx_.waverPeriod * fx_alpha;
    buffered_fx_.waverPositionShift = buffered_fx_.waverPositionShift * (1.0f - fx_alpha) + current_fx_.waverPositionShift * fx_alpha;

    fx_mode_tick_internal();
}

void Animator::fx_mode_tick_internal() {
    if (!player_ || !rgb_controller_) return;

    Xasin::NeoController::Color baseColor = buffered_colors_.vestBase;
    float glowMin = buffered_fx_.minBaseGlow / 255.0f;
    float glowMax = buffered_fx_.maxBaseGlow / 255.0f;
    float waver = 0.0f;

    if (buffered_fx_.waverAmplitude > 0.001f) {
        waver = buffered_fx_.waverAmplitude * 
                       sinf(xTaskGetTickCount() * buffered_fx_.waverPeriod + buffered_fx_.waverPositionShift);
    }

    float totalGlow = glowMin + (glowMax - glowMin) * (0.5f + 0.5f * waver);
    baseColor.alpha = static_cast<uint8_t>(totalGlow * 255.0f);

    // Apply base color to all vest LEDs
    for (uint16_t i = 0; i < VEST_LEDS; i++) {
        rgb_controller_->colors[i+1] = baseColor; // +1 to skip muzzle LED
    }

    // Apply active patterns (shot, hit, death, marked)
    for (auto* pattern : vest_patterns_) {
        if (pattern) {
            pattern->tick();
        }
    }

    // Handle fx_target_mode_ specific animations
    switch(fx_target_mode_) {
        case LZR::pattern_mode_t::OFF:
            for (uint16_t i = 0; i < VEST_LEDS; i++) {
                rgb_controller_->colors[i+1] = Xasin::NeoController::Color(0x000000); // All off
            }
            break;
        case LZR::pattern_mode_t::IDLE:
            // Idle is handled by the baseColor and waver above
            break;
        default:
            break;
    }

    // If shot pattern is active, just tick it (if needed)
    if (vest_shot_pattern_) {
        vest_shot_pattern_->tick();
    }
}
}