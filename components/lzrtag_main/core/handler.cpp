

#include <lzrtag/weapon.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

const char *handler_tag = "LZR:WPN:Handler";

namespace LZRTag {
namespace Weapon {

void handler_start_thread_func(void *args) {
	reinterpret_cast<Handler *>(args)->_internal_run_thread();
}

Handler::Handler (Xasin::Audio::TX & audio) : 
	audio(audio), previous_source(nullptr), current_source(nullptr),
	target_weapon(nullptr), current_weapon(nullptr),
	process_task(0), action_start_tick(0),
	last_shot_tick(0),
	trigger_state(false), trigger_state_read(false),
	gun_heat(0),
	on_shot_func(),
	can_shoot_func() {
}

wait_failure_t Handler::wait_for_trigger(TickType_t max_ticks, bool repress_needed) {
	ESP_LOGD(handler_tag, "Waiting for trigger!");

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
	ESP_LOGD(handler_tag, "Waiting for release!");

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
	ESP_LOGD(handler_tag, "Pausing for %d", ticks);

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
	ESP_LOGI(handler_tag, "Gun thread started!");

	while(true) {
		// First things first, check if we have a different
		// weapon that we need to switch to.
		// This includes having no weapon equipped whatsoever, in which
		// case this thread will just pause indefinitely.
		if (target_weapon == nullptr) {
			current_weapon = nullptr;

			ESP_LOGD(handler_tag, "Pausing, no gun");
			xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		}
		// Swapping to a different weapon takes a bit of time, 
		// the target weapon's equip delay is used here.
		// The swap CAN be interrupted, which is why xTaskNotifyWait is used
		else if (current_weapon != target_weapon) {
			if (action_start_tick == 0)
				action_start_tick = xTaskGetTickCount();

			if ((xTaskGetTickCount() - action_start_tick) >= target_weapon->equip_duration) {
				ESP_LOGD(handler_tag, "Equipped weapon!");

				current_weapon = target_weapon;
				action_start_tick = 0;
			}
			else {
				ESP_LOGD(handler_tag, "Pausing, equipping...");
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
				ESP_LOGD(handler_tag, "Reloaded!");

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
			ESP_LOGD(handler_tag, "Handing over to weapon code!");
			current_weapon->shot_process();
		}
		// And as last fallback, if we can't shoot etc., just wait.
		else {
			ESP_LOGD(handler_tag, "Pausing, nothing to do!");
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

}
}
