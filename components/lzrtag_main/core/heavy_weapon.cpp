

#include <lzrtag/weapon/heavy_weapon.h>

namespace LZRTag {
namespace Weapon {

HeavyWeapon::HeavyWeapon(Handler & handler, const heavy_weapon_config &config) 
	: BaseWeapon(handler), config(config)
{
	current_ammo = config.clip_ammo;

	reload_duration = config.reload_time;
	equip_duration = config.equip_time;
}

bool HeavyWeapon::can_shoot()
{
	if (current_ammo <= 0)
		return false;
	if(wants_to_reload)
		return false;

	return true;
}
bool HeavyWeapon::can_reload()
{
	return true;
}

void HeavyWeapon::reload_start() {
	handler.play(config.reload_sfx.file_path);
}
void HeavyWeapon::reload_tick()
{
	current_ammo = config.clip_ammo;
	wants_to_reload = false;
}

void HeavyWeapon::shot_process() {

	if(handler.wait_for_trigger(portMAX_DELAY) != TRIGGER_PRESSED)
		return;
	
	int variant_number = esp_random() % (config.start_sfx.size());
	handler.play(config.start_sfx[variant_number].file_path);
	current_ammo--;
	if (current_ammo == 0)
		wants_to_reload = true;

	vTaskDelay(config.start_delay);

	while(handler.get_btn_state() && handler.can_shoot()) {
		int variant_number = esp_random() % (config.shot_sfx.size());
		handler.play(config.shot_sfx[variant_number].file_path);
		vTaskDelay(config.shot_delay);

		current_ammo--;
		bump_shot_tick();
		if(current_ammo == 0)
			wants_to_reload = true;
	}
}

int32_t HeavyWeapon::get_clip_ammo()
{
	return current_ammo;
}
int32_t HeavyWeapon::get_max_clip_ammo()
{
	return config.clip_ammo;
}
int32_t HeavyWeapon::get_total_ammo()
{
	return config.max_ammo;
}

}
}