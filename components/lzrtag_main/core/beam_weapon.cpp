

#include <lzrtag/weapon/beam_weapon.h>

namespace LZRTag
{
namespace Weapon
{


BeamWeapon::BeamWeapon(Handler &handler, const beam_weapon_config &cfg) 
	: BaseWeapon(handler), config(cfg), current_charge(cfg.beam_runtime) {
}

bool BeamWeapon::can_shoot()
{
	if (current_charge <= 0)
		return false;
	if (wants_to_reload)
		return false;

	return true;
}
bool BeamWeapon::can_reload()
{
	return true;
}

void BeamWeapon::reload_start()
{
	handler.play(config.reload_sound);
}
void BeamWeapon::reload_tick()
{
	current_charge = config.beam_runtime;
	wants_to_reload = false;
}

void BeamWeapon::shot_process() {
	if(handler.wait_for_trigger() != TRIGGER_PRESSED)
		return;

	int variant_number = esp_random() % (config.start_sounds.size());

	auto last_source = handler.play(config.start_sounds[variant_number]);
	vTaskDelay(config.beam_start_delay);

	while(true) {
		if (handler.wait_for_trigger_release(30/portTICK_PERIOD_MS) != TIMEOUT)
			break;
		
		current_charge -= 30;
		bump_shot_tick();

		if(last_source->remaining_runtime() < 30)
			last_source = handler.play(config.loop_sounds[variant_number]);
	}

	if(current_charge < 100)
		wants_to_reload = true;

	last_source->fade_out();

	vTaskDelay(handler.play(config.end_sounds[variant_number])->remaining_runtime());
}

int32_t BeamWeapon::get_clip_ammo()
{
	return current_charge;
}
int32_t BeamWeapon::get_max_clip_ammo()
{
	return config.beam_runtime;
}
int32_t BeamWeapon::get_total_ammo()
{
	return -1;
}

} // namespace Weapon
} // namespace LZRTag