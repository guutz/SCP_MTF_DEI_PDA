/*
 * BitCasette.h
 *
 *  Created on: 20 Nov 2020
 *      Author: xasin
 */

#ifndef ESP32_AUDIOHANDLER_BITCASETTE_H_
#define ESP32_AUDIOHANDLER_BITCASETTE_H_

#include <xasin/audio/Source.h>
#include <stdint.h>
#include <vector>
#include <stdio.h> // Added for FILE*

#define XASAUDIO_CASSETTE(path, samplerate, volume) ((const Xasin::Audio::bytecassette_data_t){path, samplerate, volume})

namespace Xasin {
namespace Audio {

struct bytecassette_data_t {
	const char *file_path; // Changed from data_start and data_end
	uint32_t data_samplerate;
	uint8_t volume;
};

typedef std::vector<bytecassette_data_t> ByteCassetteCollection;

class ByteCassette: public Source {
private:
	FILE* sound_file; // Changed from const uint8_t *current_sample
	long file_size; // To store the size of the sound file
	long current_file_pos; // To track the current read position in the file

	// const uint8_t * const sample_end; // No longer needed

	uint32_t sample_position_counter;
	const uint32_t per_sample_increase;

protected:
	bool process_frame();

public:
	const uint32_t data_samplerate;

	uint8_t volume;

	ByteCassette(TX &handler, const char *file_path, // Changed from sample_ptr and sample_end
			uint32_t intended_samplerate);
	ByteCassette(TX &handler, const bytecassette_data_t &cassette);

	static void play(TX &handler, const bytecassette_data_t &cassette);
	static void play(TX &handler, const ByteCassetteCollection &cassette);

	~ByteCassette();

	bool is_finished();
};

} /* namespace Audio */
} /* namespace Xasin */

#endif /* ESP32_AUDIOHANDLER_BITCASETTE_H_ */
