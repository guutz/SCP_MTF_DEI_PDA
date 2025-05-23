#include <lzrtag/weapon.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "lzrtag/weapon/handler.h"
#include "lzrtag/player.h"  // Need full definition for implementation
#include <string>

namespace LZRTag {
namespace Weapon {

static const char *LZR_WPN_HANDLER_TAG = "LZR:WPN:Handler"; // Renamed to avoid conflict with handler_tag

void handler_start_thread_func(void *args) {
	reinterpret_cast<Handler *>(args)->_internal_run_thread();
}

Handler::Handler (LZR::Sounds::SoundManager & audio, Xasin::MQTT::Handler* comm_handler, LZR::Player* player) : 
	audio(audio),
	target_weapon(nullptr), current_weapon(nullptr),
	process_task(0), action_start_tick(0),
	last_shot_tick(0), last_ir_arbitration_code_(1),
	trigger_state(false), trigger_state_read(false),
	gun_heat(0),
    ir_tx_(PIN_IR_OUT, RMT_CHANNEL_1), // Initialize IR transmitter
    ir_rx_(PIN_IR_IN, RMT_CHANNEL_2),   // Initialize IR receiver
    comm_handler_(comm_handler),        // Initialize CommHandler
    player_(player),                    // Initialize Player
	on_shot_func(),
	can_shoot_func()
{
}

void Handler::play(const std::string &sound_name) {
	ESP_LOGD(LZR_WPN_HANDLER_TAG, "Playing sound: %s", sound_name.c_str());
	audio.play_audio(sound_name);
}

wait_failure_t Handler::wait_for_trigger(TickType_t max_ticks, bool repress_needed) {
	ESP_LOGD(LZR_WPN_HANDLER_TAG, "Waiting for trigger!");

	if (process_task == 0)
		return INVALID_CONFIG;
	if (xTaskGetCurrentTaskHandle() != process_task)
		return INVALID_CONFIG;

	TickType_t end_tick;
	if (max_ticks == portMAX_DELAY)
		end_tick = portMAX_DELAY;
	else
		end_tick = xTaskGetTickCount() + max_ticks;

	while(true) {
		if(!can_shoot())
			return CANNOT_SHOOT;

		if(trigger_state) {
			if(!repress_needed || !trigger_state_read) {
				trigger_state_read = true;
				return TRIGGER_PRESSED;
			}
		}

		xTaskNotifyWait(0, 0, nullptr, end_tick - xTaskGetTickCount());

		if (xTaskGetTickCount() >= end_tick)
			return TIMEOUT;
	}
}

wait_failure_t Handler::wait_for_trigger_release(TickType_t max_ticks) {
	ESP_LOGD(LZR_WPN_HANDLER_TAG, "Waiting for release!");

	if (process_task == 0)
		return INVALID_CONFIG;
	if (xTaskGetCurrentTaskHandle() != process_task)
		return INVALID_CONFIG;

	TickType_t end_tick;
	if(max_ticks == portMAX_DELAY)
		end_tick = portMAX_DELAY;
	else
		end_tick = xTaskGetTickCount() + max_ticks;

	while (true) {
		if (!can_shoot())
			return CANNOT_SHOOT;

		if (!trigger_state) {
			trigger_state_read = true;
			return TRIGGER_PRESSED;
		}

		xTaskNotifyWait(0, 0, nullptr, end_tick - xTaskGetTickCount());
	
		if (xTaskGetTickCount() >= end_tick)
			return TIMEOUT;
	}
}

wait_failure_t Handler::wait_ticks(TickType_t ticks) {
	ESP_LOGD(LZR_WPN_HANDLER_TAG, "Pausing for %d", ticks);

	if (process_task == 0)
		return INVALID_CONFIG;
	if (xTaskGetCurrentTaskHandle() != process_task)
		return INVALID_CONFIG;

	TickType_t end_tick;
	if (ticks == portMAX_DELAY)
		end_tick = portMAX_DELAY;
	else
		end_tick = xTaskGetTickCount() + ticks;

	while (true) {
		if (!can_shoot())
			return CANNOT_SHOOT;

		xTaskNotifyWait(0, 0, nullptr, end_tick - xTaskGetTickCount());

		if (xTaskGetTickCount() >= end_tick)
			return TIMEOUT;
	}
}

void Handler::boop_thread() {
	if(process_task == 0)
		return;
	
	xTaskNotify(process_task, 0, eNoAction);
}

void Handler::_internal_run_thread() {
	ESP_LOGI(LZR_WPN_HANDLER_TAG, "Gun thread started!");

	while(true) {
		// First things first, check if we have a different
		// weapon that we need to switch to.
		// This includes having no weapon equipped whatsoever, in which
		// case this thread will just pause indefinitely.
		if (target_weapon == nullptr) {
			current_weapon = nullptr;

			ESP_LOGD(LZR_WPN_HANDLER_TAG, "Pausing, no gun");
			xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		}
		// Swapping to a different weapon takes a bit of time, 
		// the target weapon's equip delay is used here.
		// The swap CAN be interrupted, which is why xTaskNotifyWait is used
		else if (current_weapon != target_weapon) {
			if (action_start_tick == 0)
				action_start_tick = xTaskGetTickCount();

			if ((xTaskGetTickCount() - action_start_tick) >= target_weapon->equip_duration) {
				ESP_LOGD(LZR_WPN_HANDLER_TAG, "Equipped weapon!");

				current_weapon = target_weapon;
				action_start_tick = 0;
			}
			else {
				ESP_LOGD(LZR_WPN_HANDLER_TAG, "Pausing, equipping...");
				xTaskNotifyWait(0, 0, nullptr, 
					action_start_tick + target_weapon->equip_duration - xTaskGetTickCount());
			}
		}
		// Now, check if we want to reload. Reloading also takes time, and 
		// similar to swapping weapons, can be interrupted (usually only by a weapon 
		// swap, such as switching to a pistol instead of reloading the primary)
		else if (current_weapon->wants_to_reload && current_weapon->can_reload()) {
			if (action_start_tick == 0) {
				current_weapon->reload_start();
				action_start_tick = xTaskGetTickCount();
			}

			if ((xTaskGetTickCount() - action_start_tick) >= current_weapon->reload_duration) {
				ESP_LOGD(LZR_WPN_HANDLER_TAG, "Reloaded!");

				current_weapon->reload_tick();
				action_start_tick = 0;
			}
			else
				xTaskNotifyWait(0, 0, nullptr,
					action_start_tick + current_weapon->reload_duration - xTaskGetTickCount());
		}
		// Otherwise, if everything is set up (we have the right weapon equipped
		// and it is reloaded and we can shoot), let the weapon code handle things!
		// NOTE: The weapon code must provide delays to wait for a trigger event!
		else if (can_shoot()) {
			ESP_LOGD(LZR_WPN_HANDLER_TAG, "Handing over to weapon code!");
			current_weapon->shot_process();
		}
		// And as last fallback, if we can't shoot etc., just wait.
		else {
			ESP_LOGD(LZR_WPN_HANDLER_TAG, "Pausing, nothing to do!");
			xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		}
	}
}

void Handler::start_thread() {
	if (process_task != 0)
		return;

	xTaskCreate(handler_start_thread_func, "LZR::WPN", 4096, this, 10, &process_task);
}

void Handler::update_btn(bool new_button_state) {
	if (new_button_state == trigger_state)
		return;

	trigger_state = new_button_state;
	trigger_state_read = false;

	boop_thread();
}

void Handler::fx_tick() {
	if (current_weapon == nullptr)
		gun_heat *= 0.95F;
	else
		gun_heat *= (1 - 0.01F); //(1 - current_weapon->gun_heat_decay);
}

bool Handler::was_shot_tick() {
	return (xTaskGetTickCount() - last_shot_tick) < 20;
}
TickType_t Handler::get_last_shot_tick() {
	return last_shot_tick;
}

float Handler::get_gun_heat() {
	return std::min(255.0F, std::max(0.0F, gun_heat));
}

bool Handler::get_btn_state(bool only_fresh) {
	if(only_fresh && trigger_state_read)
		return false;

	trigger_state_read = true;
	return trigger_state;
}

bool Handler::can_shoot() {
	if(current_weapon == nullptr)
		return false;
	if(current_weapon != target_weapon)
		return false;

	if(can_shoot_func && !can_shoot_func())
		return false;

	return current_weapon->can_shoot();
}

bool Handler::infinite_clips() {
	return true;
}
bool Handler::infinite_ammo() {
	return false;
}

void Handler::tempt_reload() {
	if(current_weapon == nullptr)
		return;
	
	current_weapon->tempt_reload();
}
ammo_info_t Handler::get_ammo() {
	if(current_weapon == nullptr)
		return {};

	return current_weapon->get_ammo();
}

void Handler::set_weapon(BaseWeapon *next_weapon) {
	if(next_weapon == target_weapon)
		return;
	
	target_weapon = next_weapon;
	boop_thread();
}

bool Handler::weapon_equipped() {
	if(target_weapon == nullptr)
		return false;

	return current_weapon == target_weapon;
}

void Handler::apply_vibration(float &vibr) {
	if(current_weapon)
		current_weapon->apply_vibration(vibr);
}

// --- IR System Methods ---
void Handler::init_ir_system() {
    if (!comm_handler_ || !player_) {
        ESP_LOGE(LZR_WPN_HANDLER_TAG, "Cannot init IR: CommHandler or Player not set!");
        return;
    }

    ir_tx_.init();
    ir_rx_.init();

    ir_rx_.on_rx = [this](const void *data, uint8_t len, uint8_t channel) {
        if (channel == 129) { // Beacon code
            uint8_t beaconID = *reinterpret_cast<const uint8_t*>(data);
            ESP_LOGD(LZR_WPN_HANDLER_TAG, "Got beacon code: %d", beaconID);

            if (comm_handler_ && !comm_handler_->is_disconnected()) {
                char oBuff[10] = {};
                sprintf(oBuff, "%d", beaconID);
                comm_handler_->publish_to("event/ir_beacon", oBuff, strlen(oBuff));
            }
        } else if (channel >= 130 && channel < 134) { // Hit code
            const uint8_t *dPtr = reinterpret_cast<const uint8_t *>(data);
            if (player_ && dPtr[0] != player_->get_id()) { 
                send_ir_hit_event(dPtr[0], channel - 130);
            }
        }
    };
    ESP_LOGI(LZR_WPN_HANDLER_TAG, "IR System Initialized in Weapon Handler");
}

void Handler::shutdown_ir_system() {
    // ir_tx_.deinit(); // If available
    // ir_rx_.deinit(); // If available
    ir_rx_.on_rx = nullptr; // Clear the callback
    ESP_LOGI(LZR_WPN_HANDLER_TAG, "IR System Shutdown in Weapon Handler");
}

void Handler::send_ir_signal(int8_t cCode) { 
    if (!player_) {
        ESP_LOGE(LZR_WPN_HANDLER_TAG, "Cannot send IR: Player not set!");
        return;
    }

    if (cCode == -1) {
        last_ir_arbitration_code_++;
        if (last_ir_arbitration_code_ >= 4) { // Should be 0, 1, 2, 3
            last_ir_arbitration_code_ = 0;
        }
        cCode = last_ir_arbitration_code_;
    }

    uint8_t shotID = player_->get_id();

    ir_tx_.send(shotID, 130 + cCode);
    ESP_LOGD(LZR_WPN_HANDLER_TAG, "Sent IR signal: shooterID=%d, arbCode=%d", shotID, cCode);

}

void Handler::send_ir_hit_event(uint8_t pID, uint8_t arbCode) {
    if (!comm_handler_ || comm_handler_->is_disconnected()) {
        ESP_LOGW(LZR_WPN_HANDLER_TAG, "Cannot send IR hit event: CommHandler not connected or not set.");
        return;
    }

    auto output = cJSON_CreateObject();
    if (!output) {
        ESP_LOGE(LZR_WPN_HANDLER_TAG, "Failed to create cJSON object for IR hit event.");
        return;
    }

    cJSON_AddNumberToObject(output, "shooterID", pID);
    std::string device_id_str = player_->get_id() == 0 ? "0" : std::to_string(player_->get_id());
    cJSON_AddStringToObject(output, "target", device_id_str.c_str()); // Use .c_str() for const char*
    cJSON_AddNumberToObject(output, "arbCode", arbCode);

    char outStr[128] = {}; // Increased buffer size
    if (cJSON_PrintPreallocated(output, outStr, sizeof(outStr) -1, false)) {
        comm_handler_->publish_to("event/ir_hit", outStr, strlen(outStr));
        ESP_LOGD(LZR_WPN_HANDLER_TAG, "Sent IR hit event: shooterID=%d, target=%s, arbCode=%d", pID, device_id_str.c_str(), arbCode);
    } else {
        ESP_LOGE(LZR_WPN_HANDLER_TAG, "Failed to print cJSON for IR hit event.");
    }

    cJSON_Delete(output);
}

}
}