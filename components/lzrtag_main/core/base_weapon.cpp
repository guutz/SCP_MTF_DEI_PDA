

#include <lzrtag/weapon.h>

namespace LZRTag {
namespace Weapon {

BaseWeapon::BaseWeapon(Handler &handler) 
	: handler(handler),
	equip_duration(2000 / portTICK_PERIOD_MS),
	wants_to_reload(false), reload_duration(1000 / portTICK_PERIOD_MS) {

}

bool BaseWeapon::can_shoot() {
	return false;
}
bool BaseWeapon::can_reload() {
	return false;
}

void BaseWeapon::bump_shot_tick() {
	handler.gun_heat = std::min(280.0F, handler.gun_heat + 30.0F);
	handler.last_shot_tick = xTaskGetTickCount();

	if(handler.on_shot_func)
		handler.on_shot_func();
}

void BaseWeapon::reload_start() {}
void BaseWeapon::reload_tick() {}

void BaseWeapon::shot_process() {
	xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
}

int32_t BaseWeapon::get_clip_ammo() {
	return 1;
}
int32_t BaseWeapon::get_max_clip_ammo() {
	return 1;
}

int32_t BaseWeapon::get_total_ammo() {
	return 1;
}

ammo_info_t BaseWeapon::get_ammo() {
	return {
		get_clip_ammo(),
		get_max_clip_ammo(),
		get_total_ammo(),
	};
}

void BaseWeapon::tempt_reload() {
	if(!can_reload())
		return;
	
	wants_to_reload = true;
	handler.boop_thread();
}

void BaseWeapon::apply_vibration(float &vibr) {
	auto shot_time = xTaskGetTickCount() - handler.get_last_shot_tick();

	static float intensity = 0;

	intensity *= 0.94;

	if(shot_time < 40/portTICK_PERIOD_MS)
		vibr = 1;
	else if(shot_time < 150/portTICK_PERIOD_MS) {
		intensity = 0.4;
		vibr = 0;
	}

	vibr = (1-intensity) * vibr + intensity;
}

}
}