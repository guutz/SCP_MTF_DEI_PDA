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

#include "lzrtag/sounds/game_start.h"

#include "lzrtag/sounds/kill_score.h"
#include "lzrtag/sounds/minor_score.h"

#include "lzrtag/sounds/own_death.h"
#include "lzrtag/sounds/own_hit.h"

#include "lzrtag/sounds/empty_click.h"
#include "lzrtag/sounds/reload_full.h"

#include "lzrtag/sounds/denybeep.h"



using namespace Xasin::NeoController;

namespace LZR {
namespace Sounds {

auto cassette_game_start	= XASAUDIO_CASSETTE(sound_game_start, 44100,  40000 >> 8);

auto cassette_kill_scored 	= XASAUDIO_CASSETTE(sound_kill_score, 44100,  15000 >> 8);
auto cassette_minor_score	= XASAUDIO_CASSETTE(sound_minor_score, 44100,  23000 >> 8);

auto cassette_death 		= XASAUDIO_CASSETTE(sound_own_death, 44100,  40000 >> 8);
auto cassette_hit			= XASAUDIO_CASSETTE(sound_own_hit, 44100,  20000 >> 8);

auto cassette_click 	   = XASAUDIO_CASSETTE(empty_click, 44100,  15000 >> 8);
auto cassette_reload_full = XASAUDIO_CASSETTE(reload_full, 44100,  12000 >> 8);

auto cassette_deny = XASAUDIO_CASSETTE(raw_denybeep, 44100,  20000 >> 8);


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
