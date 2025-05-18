#include "weapon.h"

#include <lzrtag-sfx/RELOADING/collection.h>
#include <lzrtag-sfx/SCALPEL-V9/start/collection.h>
#include <lzrtag-sfx/SCALPEL-V9/loop/collection.h>
#include <lzrtag-sfx/SCALPEL-V9/end/collection.h>
#include <lzrtag-sfx/COLIBRI M2/collection.h>

#include <lzrtag-sfx/FN-001-WHIP/collection.h>
#include <lzrtag-sfx/DP-116-STEELFINGER/collection.h>
#include <lzrtag-sfx/SW-554M1/collection.h>

#include <lzrtag-sfx/NICO-6/collection.h>
#include <lzrtag-sfx/NICO-6/charge/collection.h>


static const LZRTag::Weapon::beam_weapon_config scalpel_cfg = {
    6000, 0, 1500,
    
    Large_EnergyGun_reload_3_cassette, 
    collection_SCALPEL_V9_start, collection_SCALPEL_V9_loop, collection_SCALPEL_V9_end
};

static const LZRTag::Weapon::shot_weapon_config colibri_config = {
    12, 24,
    1000, 4000,
    
    RELOADING_3_Laser_pistol_heavy_3_cassette,
    collection_COLIBRI_M2,
    170, 2, 150, true
};

static const LZRTag::Weapon::shot_weapon_config whip_config = {
    32, 128,

    2500, 3500,

    RELOADING_3_Assault_rifle_med_1_cassette,
    collection_FN_001_WHIP,

    130, 0, 0, false
};
static const LZRTag::Weapon::shot_weapon_config steelfinger_config = {
    32, 128,

    2500, 3500,

    RELOADING_3_Laser_rifle_heavy_1_cassette,
    collection_DP_116_STEELFINGER,

    250, 0, 0, false
};
static const LZRTag::Weapon::shot_weapon_config sw_554_config = {
    4, 128,

    2500, 3500,

    RELOADING_3_Sniper_rifle_light_1_cassette,
    collection_SW_554M1,

    1000, 0, 0, true
};

static const LZRTag::Weapon::heavy_weapon_config nico_6_config = {
    50, 200,

    3500, 3500,

    RELOADING_2_large_2_cassette,
    collection_NICO_6_charge,
    collection_NICO_6,

    1300, 350
};