#ifndef __LZRTAG_WEAPON_BEAM_H__
#define __LZRTAG_WEAPON_BEAM_H__

#include "../weapon.h"
#include <vector>
#include <xasin/audio/ByteCassette.h>

namespace LZRTag {
namespace Weapon {

struct beam_weapon_config {
	uint32_t beam_runtime;
	uint32_t beam_total_battery;

	TickType_t beam_start_delay;

	Xasin::Audio::bytecassette_data_t reload_sound;

	Xasin::Audio::ByteCassetteCollection start_sounds;
	Xasin::Audio::ByteCassetteCollection loop_sounds;
	Xasin::Audio::ByteCassetteCollection end_sounds;
};

class BeamWeapon : public BaseWeapon
{
protected:
	friend Handler;

	const beam_weapon_config &config;

	int32_t current_charge;

	bool can_shoot();
	bool can_reload();

	void reload_start();
	void reload_tick();

	void shot_process();

public:
	BeamWeapon(Handler &handler, const beam_weapon_config &cfg);

	int32_t get_clip_ammo();
	int32_t get_max_clip_ammo();
	int32_t get_total_ammo();
};

} // namespace Weapon
} // namespace LZRTag

#endif