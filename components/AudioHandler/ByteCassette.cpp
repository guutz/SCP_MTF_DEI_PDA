/*
 * BitCasette.cpp
 *
 *  Created on: 20 Nov 2020
 *      Author: xasin
 */

#include <xasin/audio/AudioTX.h>
#include <xasin/audio/ByteCassette.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>
#include "sd_raw_access.h" // For SD card functions

namespace Xasin {
namespace Audio {

ByteCassette::ByteCassette(TX &audio_handler,
		const char *file_path, uint32_t samprate) // Modified constructor
	: Source(audio_handler),
	  sound_file(nullptr), // Initialize sound_file to nullptr
	  file_size(0),
	  current_file_pos(0),
	  per_sample_increase((samprate << 16)/CONFIG_XASAUDIO_TX_SAMPLERATE),
	  data_samplerate(samprate) {

	sound_file = sd_raw_fopen(file_path, "rb"); // Open file in read-binary mode
	if (sound_file) {
		file_size = sd_raw_get_file_size(file_path);
		if (file_size <= 0) { // Check for valid file size
			ESP_LOGE("ByteCassette", "Failed to get valid size for file: %s", file_path);
			sd_raw_fclose(sound_file);
			sound_file = nullptr;
		}
	} else {
		ESP_LOGE("ByteCassette", "Failed to open sound file: %s", file_path);
	}
	sample_position_counter = 0;
	volume = 255;
}

ByteCassette::ByteCassette(TX &handler, const bytecassette_data_t &cassette)
	: ByteCassette(handler, cassette.file_path, // Use file_path
			cassette.data_samplerate) {

	if(cassette.volume != 0)
		volume = cassette.volume;
}

ByteCassette::~ByteCassette() {
	if (sound_file) { // Close the file if it was opened
		sd_raw_fclose(sound_file);
		sound_file = nullptr;
	}
}

bool ByteCassette::process_frame() {
	if(!sound_file || current_file_pos >= file_size) // Check if file is open and not at EOF
		return false;

	std::array<int16_t, XASAUDIO_TX_FRAME_SAMPLE_NO> temp_buffer = {};
	uint8_t prev_byte_val = 0;
	uint8_t next_byte_val = 0;

	// Read the first byte for prev_sample if not at the beginning
	if (current_file_pos > 0) {
		sd_raw_fseek(sound_file, current_file_pos -1, SEEK_SET);
		sd_raw_fread(&prev_byte_val, 1, 1, sound_file);
	} else {
		// If at the beginning, read current byte for both prev and next initially
		sd_raw_fseek(sound_file, current_file_pos, SEEK_SET);
		sd_raw_fread(&prev_byte_val, 1, 1, sound_file);
		next_byte_val = prev_byte_val; // Initialize next_byte_val
	}


	for(int i=0; i<XASAUDIO_TX_FRAME_SAMPLE_NO; i++) {
		// Ensure we don't read past the end of the file for the next sample
		if(current_file_pos + 1 >= file_size) {
			// If current_file_pos is the last byte, use it as next_sample as well or handle as end of sound
			if (current_file_pos < file_size) { // If there's still one byte to read
			    sd_raw_fseek(sound_file, current_file_pos, SEEK_SET);
				sd_raw_fread(&next_byte_val, 1, 1, sound_file);
			} else { // No more bytes to read
				current_file_pos = file_size; // Mark as finished
				break;
			}
		} else {
			// Read the next byte for next_sample
			sd_raw_fseek(sound_file, current_file_pos + 1, SEEK_SET);
			sd_raw_fread(&next_byte_val, 1, 1, sound_file);
		}


		int32_t prev_sample_val_proc = int32_t(prev_byte_val) - 0x80;
		int32_t next_sample_val_proc = int32_t(next_byte_val) - 0x80;

		sample_position_counter += per_sample_increase;
		uint16_t sample_fraction = (sample_position_counter & 0xFFFF);

		temp_buffer[i] = (((0xFFFF - sample_fraction) * prev_sample_val_proc) + (sample_fraction * next_sample_val_proc)) >> 8;

		// Advance file position based on how many full samples were consumed
		long samples_consumed = sample_position_counter >> 16;
		current_file_pos += samples_consumed;
		sample_position_counter &= 0xFFFF; // Keep the fractional part

		if (current_file_pos >= file_size) {
			break; // End of file reached
		}

		// Update prev_byte_val for the next iteration
		prev_byte_val = next_byte_val; // Current next becomes next iteration\'s previous
	}

	add_mono_frame_to_handler(temp_buffer.data(), volume);

	return current_file_pos < file_size; // Continue if not at EOF
}

void ByteCassette::play(TX &handler, const bytecassette_data_t &cassette) {
	auto temp = new ByteCassette(handler, cassette);
	temp->start(true);

	ESP_LOGD("Audio", "Newly created source is %p", temp);
}

void ByteCassette::play(TX &handler, const ByteCassetteCollection &cassettes) {
	if(cassettes.size() == 0)
		return;

	play(handler, cassettes.at(esp_random()%cassettes.size()));
}

template<>
Source * TX::play(const bytecassette_data_t &sample, bool auto_delete) {
	auto new_sound = new ByteCassette(*this, sample);
	new_sound->start(auto_delete);

	return new_sound;
}

bool ByteCassette::is_finished() {
	return !sound_file || current_file_pos >= file_size; // Check if file is open and not at EOF
}

} /* namespace Audio */
} /* namespace Xasin */
