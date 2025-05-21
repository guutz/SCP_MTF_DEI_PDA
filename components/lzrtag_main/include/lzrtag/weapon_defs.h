#include "weapon.h"
#include "xasin/audio/ByteCassette.h"

#define WEAPON_CASSETTE(path) XASAUDIO_CASSETTE("DEI/lzrtag-sfx/" path, DEFAULT_SAMPLERATE, DEFAULT_VOLUME)


// Example samplerate and volume - adjust as needed
#define DEFAULT_SAMPLERATE 441000
#define DEFAULT_VOLUME 255

static const Xasin::Audio::ByteCassetteCollection collection_SCALPEL_V9_end = {
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_1_end.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_3_end.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_4_end.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_5_end.wav")
};

static const Xasin::Audio::ByteCassetteCollection collection_SCALPEL_V9_start = {
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_1_start.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_3_start.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_4_start.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_5_start.wav")
};

static const Xasin::Audio::ByteCassetteCollection collection_SCALPEL_V9_loop = {
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_1_loop.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_3_loop.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_4_loop.wav"),
    WEAPON_CASSETTE("SCALPEL-V9/SCALPEL_V9_shot_5_loop.wav")
};

static const LZRTag::Weapon::beam_weapon_config scalpel_cfg = {
    6000, 0, 1500,
    WEAPON_CASSETTE("RELOADING/Large_EnergyGun_reload_3.wav"),
    collection_SCALPEL_V9_start, collection_SCALPEL_V9_loop, collection_SCALPEL_V9_end,
};

static const Xasin::Audio::ByteCassetteCollection collection_FN_001_WHIP = {
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_1.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_2.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_3.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_4.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_5.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_6.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_7.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_8.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_9.wav"),
    WEAPON_CASSETTE("FN-001-WHIP/FN-001-WHIP_shot_10.wav")
};

static const LZRTag::Weapon::shot_weapon_config colibri_config = {
    12, 24,
    1000, 4000,
    
    WEAPON_CASSETTE("RELOADING/RELOADING_3_Laser_pistol_heavy_3.wav"),
    collection_FN_001_WHIP,
    170, 2, 150, true
};

static const Xasin::Audio::ByteCassetteCollection collection_COLIBRI_M2 = {
    WEAPON_CASSETTE("COLIBRI M2/COLIBRI_M2_shot_1.wav"),
    WEAPON_CASSETTE("COLIBRI M2/COLIBRI_M2_shot_2.wav"),
    WEAPON_CASSETTE("COLIBRI M2/COLIBRI_M2_shot_3.wav"),
    WEAPON_CASSETTE("COLIBRI M2/COLIBRI_M2_shot_4.wav"),
    WEAPON_CASSETTE("COLIBRI M2/COLIBRI_M2_shot_5.wav")
};

static const LZRTag::Weapon::shot_weapon_config whip_config = {
    32, 128,

    2500, 3500,

    WEAPON_CASSETTE("RELOADING/RELOADING_3_Assault_rifle_med_1.wav"),
    collection_COLIBRI_M2,

    130, 0, 0, false
};

static const Xasin::Audio::ByteCassetteCollection collection_DP_116_STEELFINGER = {
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_1.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_2.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_3.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_4.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_5.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_6.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_7.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_8.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_9.wav"),
    WEAPON_CASSETTE("DP-116-STEELFINGER/DP-116-STEELFINGER_shot_10.wav")
};

static const LZRTag::Weapon::shot_weapon_config steelfinger_config = {
    32, 128,

    2500, 3500,

    WEAPON_CASSETTE("RELOADING/RELOADING_3_Laser_rifle_heavy_1.wav"),
    collection_DP_116_STEELFINGER,
    250, 0, 0, false
};

static const Xasin::Audio::ByteCassetteCollection collection_SW_554 = {
    WEAPON_CASSETTE("SW-554/SW-554_shot_1.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_2.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_3.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_4.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_5.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_6.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_7.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_8.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_9.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_10.wav"),
    WEAPON_CASSETTE("SW-554/SW-554_shot_11.wav")
};

static const LZRTag::Weapon::shot_weapon_config sw_554_config = {
    4, 128,

    2500, 3500,

    WEAPON_CASSETTE("RELOADING/RELOADING_3_Sniper_rifle_light_1.wav"),
    collection_SW_554,

    1000, 0, 0, true
};

static const Xasin::Audio::ByteCassetteCollection collection_NICO_6 = {
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_1.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_2.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_3.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_5.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_6.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_7.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_8.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_10.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_11.wav"),
    WEAPON_CASSETTE("NICO-6/NICO-6_shot_clean_12.wav")
};

static const Xasin::Audio::ByteCassetteCollection collection_NICO_6_charge = {
    WEAPON_CASSETTE("NICO-6/charge/NICO-6_shot_1.wav"),
    WEAPON_CASSETTE("NICO-6/charge/NICO-6_shot_2.wav"),
    WEAPON_CASSETTE("NICO-6/charge/NICO-6_shot_3.wav"),
    WEAPON_CASSETTE("NICO-6/charge/NICO-6_shot_4.wav")
};

static const LZRTag::Weapon::heavy_weapon_config nico_6_config = {
    50, 200,

    3500, 3500,

    WEAPON_CASSETTE("RELOADING/RELOADING_2_large_2.wav"),
    collection_NICO_6_charge,
    collection_NICO_6,

    1300, 350
};