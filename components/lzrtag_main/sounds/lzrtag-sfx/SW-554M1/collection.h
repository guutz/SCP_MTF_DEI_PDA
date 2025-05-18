#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "SW-554M1_shot_01_raw.h"
#include "SW-554M1_shot_02_raw.h"
#include "SW-554M1_shot_03_raw.h"
#include "SW-554M1_shot_04_raw.h"
#include "SW-554M1_shot_05_raw.h"
#include "SW-554M1_shot_06_raw.h"
#include "SW-554M1_shot_07_raw.h"
#include "SW-554M1_shot_08_raw.h"
#include "SW-554M1_shot_09_raw.h"
#include "SW-554M1_shot_10_raw.h"
#include "SW-554M1_shot_11_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_1_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_01_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_2_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_02_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_3_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_03_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_4_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_04_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_5_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_05_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_6_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_06_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_7_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_07_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_8_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_08_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_9_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_09_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_10_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_10_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t sw_554m1_shot_11_cassette =
    XASAUDIO_CASSETTE(sound_data_SW_554M1_shot_11_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_SW_554M1 = {
    sw_554m1_shot_1_cassette,
    sw_554m1_shot_2_cassette,
    sw_554m1_shot_3_cassette,
    sw_554m1_shot_4_cassette,
    sw_554m1_shot_5_cassette,
    sw_554m1_shot_6_cassette,
    sw_554m1_shot_7_cassette,
    sw_554m1_shot_8_cassette,
    sw_554m1_shot_9_cassette,
    sw_554m1_shot_10_cassette,
    sw_554m1_shot_11_cassette,
};