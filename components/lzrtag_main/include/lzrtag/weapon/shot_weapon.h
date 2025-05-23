#ifndef __LZRTAG_WEAPON_TEST_H__
#define __LZRTAG_WEAPON_TEST_H__

#include "../weapon.h"
#include <vector>

namespace LZRTag {
namespace Weapon {

struct shot_weapon_config {
	int32_t clip_ammo;
	int32_t max_ammo;

	int32_t equip_time;
	int32_t reload_time;

	Xasin::Audio::bytecassette_data_t reload_sfx;
	Xasin::Audio::ByteCassetteCollection shot_sfx;

	TickType_t shot_delay;
	int salve_count;
	TickType_t post_salve_delay;

	bool require_repress;
};

class ShotWeapon : public BaseWeapon {
protected:
friend Handler;

	const shot_weapon_config &config;

	int32_t current_ammo;

	bool can_shoot();
	bool can_reload();

	void reload_start();
	void reload_tick();

	void shot_process();

public:
	ShotWeapon(Handler &handler, const shot_weapon_config &config);

	int32_t get_clip_ammo();
	int32_t get_max_clip_ammo();
	int32_t get_total_ammo();
};

}
}

#endif