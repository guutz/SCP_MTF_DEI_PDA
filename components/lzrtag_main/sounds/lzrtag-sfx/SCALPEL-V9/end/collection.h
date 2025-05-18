#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "SCALPEL_V9_shot_1_end_raw.h"
#include "SCALPEL_V9_shot_3_end_raw.h"
#include "SCALPEL_V9_shot_4_end_raw.h"
#include "SCALPEL_V9_shot_5_end_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t scalpel_v9_shot_1_end_cassette = XASAUDIO_CASSETTE(sound_data_SCALPEL_V9_shot_1_end_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t scalpel_v9_shot_3_end_cassette = XASAUDIO_CASSETTE(sound_data_SCALPEL_V9_shot_3_end_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t scalpel_v9_shot_4_end_cassette = XASAUDIO_CASSETTE(sound_data_SCALPEL_V9_shot_4_end_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t scalpel_v9_shot_5_end_cassette = XASAUDIO_CASSETTE(sound_data_SCALPEL_V9_shot_5_end_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_SCALPEL_V9_end = {
    scalpel_v9_shot_1_end_cassette,
    scalpel_v9_shot_3_end_cassette,
    scalpel_v9_shot_4_end_cassette,
    scalpel_v9_shot_5_end_cassette,
};