

#include <lzrtag/weapon/shot_weapon.h>

namespace LZRTag {
namespace Weapon {

ShotWeapon::ShotWeapon(Handler & handler, const shot_weapon_config &config) 
	: BaseWeapon(handler), config(config)
{
	current_ammo = config.clip_ammo;
}

bool ShotWeapon::can_shoot()
{
	if (current_ammo <= 0)
		return false;
	if(wants_to_reload)
		return false;

	return true;
}
bool ShotWeapon::can_reload()
{
	return true;
}

void ShotWeapon::reload_start() {
	handler.play(config.reload_sfx);
}
void ShotWeapon::reload_tick()
{
	current_ammo = config.clip_ammo;
	wants_to_reload = false;
}

/*
while(true) {
		if (handler.wait_for_trigger(portMAX_DELAY) != TRIGGER_PRESSED) {
			if(xTaskGetTickCount() - last_shot < 2500/portTICK_PERIOD_MS)
				vTaskDelay(2000 - xTaskGetTickCount() + last_shot);
			
			return;
		}

		uint32_t passed_ticks = xTaskGetTickCount() - last_shot;
		shot_speed -= std::min<int32_t>(1000, (1000 * passed_ticks) / 4000);
		shot_speed = std::max<int32_t>(shot_speed, 0);

		handler.play(shots);
		last_shot = xTaskGetTickCount();
		shot_speed = std::min(1500, shot_speed + 240);

		current_ammo--;
		if (current_ammo <= 0)
			wants_to_reload = true;

		vTaskDelay(std::max<int32_t>(150, 150*1000 / std::max<int32_t>(shot_speed, 150)) + esp_random() / (UINT32_MAX / 20));
	}*/

void ShotWeapon::shot_process()
{
	TickType_t last_shot = 0;

	if(handler.wait_for_trigger(portMAX_DELAY, config.require_repress) != TRIGGER_PRESSED)
		return;
	
	for(int i = std::max(1, config.salve_count); i != 0; i--) {
		handler.play(config.shot_sfx);
		
		bump_shot_tick();
		current_ammo--;
		if(current_ammo == 0)
			wants_to_reload = true;
		
		vTaskDelay(config.shot_delay);
	}

	if(config.post_salve_delay)
		vTaskDelay(config.post_salve_delay);
}

int32_t ShotWeapon::get_clip_ammo()
{
	return current_ammo;
}
int32_t ShotWeapon::get_max_clip_ammo()
{
	return config.clip_ammo;
}
int32_t ShotWeapon::get_total_ammo()
{
	return config.max_ammo;
}

}
}