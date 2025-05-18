#include <xasin/audio/ByteCassette.h>

// Include the new raw audio data headers
#include "NICO-6_shot_clean_1_raw.h"
#include "NICO-6_shot_clean_2_raw.h"
#include "NICO-6_shot_clean_3_raw.h"
#include "NICO-6_shot_clean_5_raw.h"
#include "NICO-6_shot_clean_6_raw.h"
#include "NICO-6_shot_clean_7_raw.h"
#include "NICO-6_shot_clean_8_raw.h"
#include "NICO-6_shot_clean_10_raw.h"
#include "NICO-6_shot_clean_11_raw.h"
#include "NICO-6_shot_clean_12_raw.h"

// Define bytecassette_data_t for each sound
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_1_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_1_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_2_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_2_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_3_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_3_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_5_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_5_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_6_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_6_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_7_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_7_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_8_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_8_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_10_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_10_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_11_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_11_raw, 16000, 255);
static const Xasin::Audio::bytecassette_data_t nico_6_shot_clean_12_cassette = XASAUDIO_CASSETTE(sound_data_NICO_6_shot_clean_12_raw, 16000, 255);

// Use ByteCassetteCollection
static const Xasin::Audio::ByteCassetteCollection collection_NICO_6 = {
    nico_6_shot_clean_1_cassette,
    nico_6_shot_clean_2_cassette,
    nico_6_shot_clean_3_cassette,
	nico_6_shot_clean_5_cassette,
	nico_6_shot_clean_6_cassette,
	nico_6_shot_clean_7_cassette,
	nico_6_shot_clean_8_cassette,
	nico_6_shot_clean_10_cassette,
	nico_6_shot_clean_11_cassette,
	nico_6_shot_clean_12_cassette,
};