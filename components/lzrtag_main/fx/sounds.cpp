/*
 * sounds.cpp
 *
 *  Created on: 30 Mar 2019
 *      Author: xasin
 */

#include "xasin/audio.h"
#include "xasin/neocontroller.h"
#include "lzrtag/sounds.h"
#include "CommHandler.h"

#define GAME_START_SOUND_PATH "DEI/lzrtag-sfx/game_start.wav"
#define KILL_SCORE_SOUND_PATH "DEI/lzrtag-sfx/kill_score.wav"
#define MINOR_SCORE_SOUND_PATH "DEI/lzrtag-sfx/minor_score.wav"
#define OWN_DEATH_SOUND_PATH "DEI/lzrtag-sfx/own_death.wav"
#define OWN_HIT_SOUND_PATH "DEI/lzrtag-sfx/own_hit.wav"
#define EMPTY_CLICK_SOUND_PATH "DEI/lzrtag-sfx/empty_click.wav"
#define RELOAD_FULL_SOUND_PATH "DEI/lzrtag-sfx/reload_full.wav"
#define DENYBEEP_SOUND_PATH "DEI/lzrtag-sfx/denybeep.wav"


#define DEFAULT_FX_SAMPLERATE 44100
#define DEFAULT_FX_VOLUME_GAME_START (40000 >> 8)
#define DEFAULT_FX_VOLUME_KILL_SCORE (15000 >> 8)
#define DEFAULT_FX_VOLUME_MINOR_SCORE (23000 >> 8)
#define DEFAULT_FX_VOLUME_OWN_DEATH (40000 >> 8)
#define DEFAULT_FX_VOLUME_OWN_HIT (20000 >> 8)
#define DEFAULT_FX_VOLUME_EMPTY_CLICK (15000 >> 8)
#define DEFAULT_FX_VOLUME_RELOAD_FULL (12000 >> 8)
#define DEFAULT_FX_VOLUME_DENYBEEP (20000 >> 8)


using namespace Xasin::NeoController;

namespace LZR {
namespace Sounds {

auto cassette_game_start = XASAUDIO_CASSETTE(GAME_START_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_GAME_START);
auto cassette_kill_scored = XASAUDIO_CASSETTE(KILL_SCORE_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_KILL_SCORE);
auto cassette_minor_score = XASAUDIO_CASSETTE(MINOR_SCORE_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_MINOR_SCORE);
auto cassette_death = XASAUDIO_CASSETTE(OWN_DEATH_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_OWN_DEATH);
auto cassette_hit = XASAUDIO_CASSETTE(OWN_HIT_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_OWN_HIT);
auto cassette_click = XASAUDIO_CASSETTE(EMPTY_CLICK_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_EMPTY_CLICK);
auto cassette_reload_full = XASAUDIO_CASSETTE(RELOAD_FULL_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_RELOAD_FULL);
auto cassette_deny = XASAUDIO_CASSETTE(DENYBEEP_SOUND_PATH, DEFAULT_FX_SAMPLERATE, DEFAULT_FX_VOLUME_DENYBEEP);

SoundManager::SoundManager(Xasin::Audio::TX& audioManager, Xasin::Communication::CommHandler* commHandler)
    : audioManager_(audioManager), comm_handler_(commHandler) {}

void SoundManager::play_audio(const std::string& aName) {
    if(aName == "GAME START")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_game_start);
    else if(aName == "KILL SCORE")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_kill_scored);
    else if(aName == "MINOR SCORE")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_minor_score);
    else if(aName == "DEATH")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_death);
    else if(aName == "HIT")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_hit);
    else if(aName == "CLICK")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_click);
    else if(aName == "RELOAD FULL")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_reload_full);
    else if(aName == "DENY")
        Xasin::Audio::ByteCassette::play(audioManager_, cassette_deny);
}

void SoundManager::init() {
    comm_handler_->subscribe("Sound/#", [this](Xasin::Communication::CommReceivedData data) {
        // Only expect the payload to be the sound name
        play_audio(std::string(data.payload.begin(), data.payload.end()));
    });
}

} // namespace Sounds
} // namespace LZR
