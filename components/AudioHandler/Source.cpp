/*
 * Source.cpp
 *
 *  Created on: 20 Nov 2020
 *      Author: xasin
 */

#include "xasin/audio/Source.h"
#include "xasin/audio/AudioTX.h"

namespace Xasin {
namespace Audio {

Source::Source(TX &handler) : audio_handler(handler) {
	was_started = false;
	is_deletable = false;
}

Source::~Source() {
	audio_handler.remove_source(this);
}

bool Source::process_frame() {
	return false;
}

void Source::boop_playback() {
	audio_handler.boop_thread();
}

// Add mono frame data to the audio_handler's buffer
void Source::add_mono_frame_to_handler(const int16_t *data, uint8_t volume) {
	audio_handler.add_mono_frame(data, volume);
}

bool Source::can_be_deleted() {
	return is_deletable;
}

bool Source::is_finished() {
	return true;
}
bool Source::has_audio() {
	return !is_finished();
}

TickType_t Source::remaining_runtime() {
	return 0;
}

void Source::fade_out() {}

void Source::start(bool deletable) {
	if(was_started)
		return;

	is_deletable = deletable;

	was_started = true;
	audio_handler.insert_source(this);
}

void Source::release() {
	if(is_deletable)
		return;

	is_deletable = true;
	boop_playback();
}

} /* namespace Audio */
} /* namespace Xasin */
