#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "DP-116-STEELFINGER_shot_1_raw.h"
#include "DP-116-STEELFINGER_shot_2_raw.h"
#include "DP-116-STEELFINGER_shot_3_raw.h"
#include "DP-116-STEELFINGER_shot_4_raw.h"
#include "DP-116-STEELFINGER_shot_5_raw.h"
#include "DP-116-STEELFINGER_shot_6_raw.h"
#include "DP-116-STEELFINGER_shot_7_raw.h"
#include "DP-116-STEELFINGER_shot_8_raw.h"
#include "DP-116-STEELFINGER_shot_9_raw.h"
#include "DP-116-STEELFINGER_shot_10_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_1_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_1_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_2_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_3_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_3_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_4_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_4_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_5_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_5_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_6_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_6_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_7_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_7_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_8_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_8_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_9_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_9_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t dp_116_steelfinger_shot_10_cassette = XASAUDIO_CASSETTE(sound_data_DP_116_STEELFINGER_shot_10_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_DP_116_STEELFINGER = {
    dp_116_steelfinger_shot_1_cassette,
    dp_116_steelfinger_shot_2_cassette,
    dp_116_steelfinger_shot_3_cassette,
    dp_116_steelfinger_shot_4_cassette,
    dp_116_steelfinger_shot_5_cassette,
    dp_116_steelfinger_shot_6_cassette,
    dp_116_steelfinger_shot_7_cassette,
    dp_116_steelfinger_shot_8_cassette,
    dp_116_steelfinger_shot_9_cassette,
    dp_116_steelfinger_shot_10_cassette,
};