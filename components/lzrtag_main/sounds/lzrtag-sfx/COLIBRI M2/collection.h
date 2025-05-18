#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "COLIBRI_M2_shot_1_raw.h"
#include "COLIBRI_M2_shot_2_raw.h"
#include "COLIBRI_M2_shot_3_raw.h"
#include "COLIBRI_M2_shot_4_raw.h"
#include "COLIBRI_M2_shot_5_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t colibri_m2_shot_1_cassette = XASAUDIO_CASSETTE(sound_data_COLIBRI_M2_shot_1_raw, 16000, 153);
static const Xasin::Audio::bytecassette_data_t colibri_m2_shot_2_cassette = XASAUDIO_CASSETTE(sound_data_COLIBRI_M2_shot_2_raw, 16000, 153);
static const Xasin::Audio::bytecassette_data_t colibri_m2_shot_3_cassette = XASAUDIO_CASSETTE(sound_data_COLIBRI_M2_shot_3_raw, 16000, 153);
static const Xasin::Audio::bytecassette_data_t colibri_m2_shot_4_cassette = XASAUDIO_CASSETTE(sound_data_COLIBRI_M2_shot_4_raw, 16000, 153);
static const Xasin::Audio::bytecassette_data_t colibri_m2_shot_5_cassette = XASAUDIO_CASSETTE(sound_data_COLIBRI_M2_shot_5_raw, 16000, 153);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_COLIBRI_M2 = {
    colibri_m2_shot_1_cassette,
    colibri_m2_shot_2_cassette,
    colibri_m2_shot_3_cassette,
    colibri_m2_shot_4_cassette,
    colibri_m2_shot_5_cassette,
};