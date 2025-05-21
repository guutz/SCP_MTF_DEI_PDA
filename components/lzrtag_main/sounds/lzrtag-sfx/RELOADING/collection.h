#include <xasin/audio/ByteCassette.h>
#include "Large_EnergyGun_reload_3_raw.h"
#include "RELOADING_2_large_2_raw.h"
#include "RELOADING_2_middle_2_raw.h"
#include "RELOADING_3_Assault_rifle_med_1_raw.h"
#include "RELOADING_3_Laser_pistol_heavy_3_raw.h"
#include "RELOADING_3_Laser_rifle_heavy_1_raw.h"
#include "RELOADING_3_Sniper_rifle_light_1_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t Large_EnergyGun_reload_3_cassette = XASAUDIO_CASSETTE(sound_data_Large_EnergyGun_reload_3_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_2_large_2_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_2_large_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_2_middle_2_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_2_middle_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_3_Assault_rifle_med_1_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_3_Assault_rifle_med_1_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_3_Laser_pistol_heavy_3_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_3_Laser_pistol_heavy_3_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_3_Laser_rifle_heavy_1_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_3_Laser_rifle_heavy_1_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t RELOADING_3_Sniper_rifle_light_1_cassette = XASAUDIO_CASSETTE(sound_data_RELOADING_3_Sniper_rifle_light_1_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_RELOADING = {
    Large_EnergyGun_reload_3_cassette,
    RELOADING_2_large_2_cassette,
    RELOADING_2_middle_2_cassette,
    RELOADING_3_Assault_rifle_med_1_cassette,
    RELOADING_3_Laser_pistol_heavy_3_cassette,
    RELOADING_3_Laser_rifle_heavy_1_cassette,
    RELOADING_3_Sniper_rifle_light_1_cassette,
};