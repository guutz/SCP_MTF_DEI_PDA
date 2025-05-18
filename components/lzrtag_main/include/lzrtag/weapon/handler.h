#ifndef __LZRTAG_WEAPON_HANDLER_H__
#define __LZRTAG_WEAPON_HANDLER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <xasin/audio/ByteCassette.h>
#include "xasin/audio/ByteCassette.h"
#include <functional>
#include <vector>

namespace LZRTag {
namespace Weapon {

enum wait_failure_t {
	TRIGGER_PRESSED,
	TIMEOUT,
	CANNOT_SHOOT,
	INVALID_CONFIG,
};

struct ammo_info_t {
	int32_t current_ammo;
	int32_t clipsize;
	int32_t  total_ammo;
};

class BaseWeapon;

class AudioSource {
public:
    int remaining_runtime() { return 100; } // Dummy value
    void fade_out() {}
};

class Handler {
protected:
friend BaseWeapon;

	Xasin::Audio::TX &audio;
	Xasin::Audio::Source * previous_source;
	Xasin::Audio::Source * current_source;

	// Current weapon points to the weapon instance that is actively being used.
	// target_weapon points to the weapon the user wants to use. If the pointers
	// are different, a weapon switch will be initiated.
	BaseWeapon *target_weapon;
	BaseWeapon *current_weapon;

	// FreeRTOS Task Handle pointing to the internal weapon shot management
	// thread. Mainly used for xTaskNotify to update on button press or
	// weapon change.
	TaskHandle_t process_task;

	// First tick at which an action (reloading, weapon switch,
	// forced weapon cooldown) occurs. Used for time-keeping.
	TickType_t  action_start_tick;

	TickType_t last_shot_tick;

	// Current status of the trigger button. True means "shoot"
	bool trigger_state;
	// "New" flag for the trigger button state. Allows weapons to only wait
	// for the initial button press, and no re-trigger until the button is released.
	bool trigger_state_read;

	float gun_heat;

public:
	Handler(Xasin::Audio::TX & audio);
	void _internal_run_thread();
	void start_thread();
	AudioSource* play(int sound_id);
	AudioSource* play(const std::vector<int>& sound_ids);
	AudioSource* play(const Xasin::Audio::ByteCassetteCollection& sfx);
	AudioSource* play(const Xasin::Audio::bytecassette_data_t& sfx);

	wait_failure_t wait_for_trigger(TickType_t max_ticks = portMAX_DELAY, bool repress_needed = false);
	wait_failure_t wait_for_trigger_release(TickType_t max_ticks = portMAX_DELAY);
	wait_failure_t wait_ticks(TickType_t ticks);

	void boop_thread();
	void update_btn(bool new_button_state);
	void fx_tick();
	float get_gun_heat();
	bool was_shot_tick();
	TickType_t get_last_shot_tick();
	std::function<void (void)> on_shot_func;
	bool get_btn_state(bool only_fresh = false);
	bool can_shoot();
	std::function<bool (void)> can_shoot_func;
	bool infinite_clips();
	bool infinite_ammo();
	void tempt_reload();
	ammo_info_t get_ammo();
	void set_weapon(BaseWeapon *next_weapon);
	bool weapon_equipped();
	void apply_vibration(float &vibr);

};

}
}

#endif
