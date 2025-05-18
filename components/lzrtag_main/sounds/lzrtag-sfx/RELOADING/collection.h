#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
// Note: This collection previously had RELOADING_SOUND_1, _2, _3.
// Based on directory listing, these specific files don't exist with that exact naming for _raw.h.
// Assuming general reload sounds might be used or specific ones like Large_EnergyGun_reload_3_raw.h etc.
// For now, leaving placeholders. User needs to confirm which reload sounds to use here.
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