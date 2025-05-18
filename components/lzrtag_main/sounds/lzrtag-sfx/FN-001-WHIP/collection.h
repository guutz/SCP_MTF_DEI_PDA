#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "FN-001-WHIP_shot_1_raw.h"
#include "FN-001-WHIP_shot_2_raw.h"
#include "FN-001-WHIP_shot_3_raw.h"
#include "FN-001-WHIP_shot_4_raw.h"
#include "FN-001-WHIP_shot_5_raw.h"
#include "FN-001-WHIP_shot_6_raw.h"
#include "FN-001-WHIP_shot_7_raw.h"
#include "FN-001-WHIP_shot_8_raw.h"
#include "FN-001-WHIP_shot_9_raw.h"
#include "FN-001-WHIP_shot_10_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_1_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_1_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_2_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_3_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_3_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_4_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_4_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_5_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_5_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_6_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_6_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_7_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_7_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_8_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_8_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_9_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_9_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t fn_001_whip_shot_10_cassette = XASAUDIO_CASSETTE(sound_data_FN_001_WHIP_shot_10_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_FN_001_WHIP = {
    fn_001_whip_shot_1_cassette,
    fn_001_whip_shot_2_cassette,
    fn_001_whip_shot_3_cassette,
    fn_001_whip_shot_4_cassette,
    fn_001_whip_shot_5_cassette,
    fn_001_whip_shot_6_cassette,
    fn_001_whip_shot_7_cassette,
    fn_001_whip_shot_8_cassette,
    fn_001_whip_shot_9_cassette,
    fn_001_whip_shot_10_cassette,
};