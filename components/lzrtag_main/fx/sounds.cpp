/*
 * sounds.cpp
 *
 *  Created on: 30 Mar 2019
 *      Author: xasin
 */

#include "lzrtag/sounds.h"

#include "lzrtag/sounds/game_start.h"

#include "lzrtag/sounds/kill_score.h"
#include "lzrtag/sounds/minor_score.h"

#include "lzrtag/sounds/own_death.h"
#include "lzrtag/sounds/own_hit.h"

#include "lzrtag/sounds/empty_click.h"
#include "lzrtag/sounds/reload_full.h"

#include "lzrtag/sounds/denybeep.h"

#include "lzrtag/setup.h"

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

void play_audio(std::string aName) {
	if(aName == "GAME START")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_game_start);
	else if(aName == "KILL SCORE")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_kill_scored);
	else if(aName == "MINOR SCORE")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_minor_score);
	else if(aName == "DEATH")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_death);
	else if(aName == "HIT")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_hit);
	else if(aName == "CLICK")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_click);
	else if(aName == "RELOAD FULL")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_reload_full);
	else if(aName == "DENY")
		Xasin::Audio::ByteCassette::play(audioManager, cassette_deny);
}

void init() {
	mqtt.subscribe_to("Sound/#", [](Xasin::MQTT::MQTT_Packet data) {
		if(data.topic == "File")
			play_audio(data.data);
// 		else if(data.topic == "Note") {
// #pragma pack(1)
// 			struct note_data_t {
// 				uint32_t frequency;
// 				uint32_t volume;
// 				uint32_t duration;
// 			} note = *reinterpret_cast<const note_data_t*>(data.data.data());
// #pragma pack(0)
//
// 			if(note.duration < 5000
// 				LZR::audioManager.insert_sample(new TriangleWave(note.frequency, note.volume, note.duration));
// 		}
	});
}

}
}
