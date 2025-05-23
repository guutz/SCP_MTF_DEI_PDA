/*
 * animatorThread.h
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#ifndef MAIN_FX_ANIMATORTHREAD_H_
#define MAIN_FX_ANIMATORTHREAD_H_

#include "lzrtag/weapon/base_weapon.h" // For LZRTag::Weapon::BaseWeapon
#include "lzrtag/weapon/handler.h"    // For LZRTag::Weapon::Handler
#include "xasin/BatteryManager.h"
#include "lzrtag/LZRConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector> // For std::vector

// Forward declarations
namespace LZR {
    class Player;
}

namespace LZRTag {
    namespace Weapon {
        class Handler;
        class BaseWeapon;
    }
}

namespace Housekeeping {
    class BatteryManager;
}

namespace Xasin {
    namespace MQTT {
        class Handler;
    }
}

namespace LZR {

class Animator {
public:
    // Constructor and Destructor
    Animator(
        LZR::Player* player_ptr,
        LZRTag::Weapon::Handler* weapon_handler_ptr,
        std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_ptr,
        Housekeeping::BatteryManager* battery_ptr,
        Xasin::MQTT::Handler* mqtt_ptr
    );
    ~Animator();

    // Public interface
    void start_animation_task();

private:
    // Dependencies (pointers to external objects)
    LZR::Player* player_;
    LZRTag::Weapon::Handler* weapon_handler_;
    std::vector<LZRTag::Weapon::BaseWeapon*>* weapons_;
    Housekeeping::BatteryManager* battery_;
    Xasin::MQTT::Handler* mqtt_;

    // Internal State

    // Task Management
    TaskHandle_t animation_task_handle_;
    static void animation_task_entry(void *instance); // Static entry for FreeRTOS
    void animation_task_runner();                     // Instance method for task logic
    void vibr_motor_tick_internal();

    TickType_t vibr_motor_count_old_; // State for vibr_motor_tick_internal
};

} /* namespace LZR */

#endif /* MAIN_FX_ANIMATORTHREAD_H_ */