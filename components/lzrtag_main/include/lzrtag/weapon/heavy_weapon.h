#ifndef __LZRTAG_WEAPON_HEAVY_H__
#define __LZRTAG_WEAPON_HEAVY_H__

#include "../weapon.h"
#include <vector>
#include <xasin/audio/ByteCassette.h>

namespace LZRTag {
namespace Weapon {

struct heavy_weapon_config {
	int32_t clip_ammo;
	int32_t max_ammo;

	int32_t equip_time;
	int32_t reload_time;

	Xasin::Audio::bytecassette_data_t reload_sfx;

	Xasin::Audio::ByteCassetteCollection start_sfx;
	Xasin::Audio::ByteCassetteCollection shot_sfx;

	TickType_t start_delay;
	TickType_t shot_delay;
};

class HeavyWeapon : public BaseWeapon {
protected:
friend Handler;

	const heavy_weapon_config &config;

	int32_t current_ammo;

	bool can_shoot();
	bool can_reload();

	void reload_start();
	void reload_tick();

	void shot_process();

public:
	HeavyWeapon(Handler &handler, const heavy_weapon_config &config);

	int32_t get_clip_ammo();
	int32_t get_max_clip_ammo();
	int32_t get_total_ammo();
};

}
}

#endif