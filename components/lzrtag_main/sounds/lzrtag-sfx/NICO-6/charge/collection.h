#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "NICO-6_shot_1_raw.h"
#include "NICO-6_shot_2_raw.h"
#include "NICO-6_shot_3_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t nico_6_charge_1_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_1_raw, 16000, 255); // Assuming charge sounds map to general NICO-6 shots, adjust if specific charge_raw.h exist
static const Xasin::Audio::bytecassette_data_t nico_6_charge_2_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_charge_3_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_3_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_NICO_6_charge = {
    nico_6_charge_1_cassette,
    nico_6_charge_2_cassette,
    nico_6_charge_3_cassette,
};