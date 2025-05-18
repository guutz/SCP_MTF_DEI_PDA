/*
 * Source.h
 *
 *  Created on: 20 Nov 2020
 *      Author: xasin
 */

#ifndef ESP32_AUDIOHANDLER_SOURCE_H_
#define ESP32_AUDIOHANDLER_SOURCE_H_

#include <stdint.h>

#include <freertos/FreeRTOS.h>

namespace Xasin {
namespace Audio {

class TX;

class Source {
private:
	bool was_started;
	bool is_deletable;

protected:
friend TX;

	TX &audio_handler;

	virtual bool process_frame();
	void boop_playback();

	// This function will always add exactly one frame (e.g., 20ms) of the given mono data
	// buffer to the audio_handler's internal buffer. volume can be used to reduce the volume
	// of the given sample (independently of the global volume).
	// This function expects 16-bit signed mono audio data.
	void add_mono_frame_to_handler(const int16_t *data, uint8_t volume = 255);

public:
	Source(TX &handler);
	virtual ~Source();

	bool can_be_deleted();

	virtual bool is_finished();
	virtual bool has_audio();

	virtual TickType_t remaining_runtime();

	virtual void fade_out();

	void start(bool deletable = true);
	void release();
};

} /* namespace Audio */
} /* namespace Xasin */

#endif /* ESP32_AUDIOHANDLER_SOURCE_H_ */
