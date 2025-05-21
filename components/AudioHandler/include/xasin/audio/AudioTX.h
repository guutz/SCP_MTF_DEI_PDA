#ifndef XASIN_AUDIO_TX_H
#define XASIN_AUDIO_TX_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <memory>

#include <array>
#include <vector>

#include "driver/dac.h"

#define XASAUDIO_TX_FRAME_SAMPLE_NO ((CONFIG_XASAUDIO_TX_SAMPLERATE * CONFIG_XASAUDIO_TX_FRAMELENGTH)/1000)

namespace Xasin {
namespace Audio {

class Source;

class TX {
public:
	enum audio_tx_state_t {
		RUNNING,			// The audio handler is active and playing
		PROCESSING,			// The audio handler is waiting on processed data
		PRE_IDLE,				// There is no audio to be played from any source
		IDLE,
	};

private:
	TaskHandle_t audio_task; // Handle to the small DAC output task.
	TaskHandle_t processing_task; // Handle to a large-stack processing task,
		// useful to decode audio frames or perform other intensive operations.

	SemaphoreHandle_t audio_config_mutex;

	float volume_estimate;

	void calculate_audio_rms();

protected:
friend Source;

	void boop_thread();
	void remove_source(Source * source);
	void insert_source(Source * source);

	audio_tx_state_t state;

	// This buffer represents 20ms of samples at the configured sample rate, 16 bit,
	// and is used to exchange data between the DAC output task and the processing
	// task. 20ms is a common length for audio processing frames.
	// The data here represents mono audio samples.
	std::array<int16_t, XASAUDIO_TX_FRAME_SAMPLE_NO> audio_buffer; // Changed to mono

	std::vector<Source*> audio_sources; // Ensure this line is present and not commented

	bool clipping;

	// These functions will always add exactly one frame (e.g., 20ms) of the given mono data
	// buffer to the audio buffer. volume can be used to reduce the volume
	// of the given sample (independently of the global volume).
	// This function handles potential saturation.
	void add_mono_frame(const int16_t *data, uint8_t volume = 255);

public:
	bool calculate_volume;
	uint8_t volume_mod;

	void audio_dac_output_task(); // Handles sending audio data to the DAC
	bool largestack_process();

	TX(); // Constructor
	TX(const TX&) = delete;

	void init(TaskHandle_t processing_task); // Initialize DAC and audio task

	float get_volume_estimate();
	bool had_clipping();

	template<class T> Source * play(const T &sample, bool auto_delete = true);
};

}
}

#endif
