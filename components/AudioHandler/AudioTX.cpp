#include "xasin/audio/AudioTX.h"
#include "xasin/audio/Source.h"

#include <cstring>
#include <cmath>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "driver/dac.h" // Added for DAC functions
#include "rom/ets_sys.h" // For ets_delay_us
#include "esp_timer.h"   // For esp_timer_get_time

namespace Xasin {
namespace Audio {

void start_audio_task(void *arg) {
	reinterpret_cast<TX *>(arg)->audio_dac_output_task(); // Starts the DAC output task
}

TX::TX() { // Constructor

	audio_task = nullptr;
	processing_task = nullptr;

	audio_config_mutex = xSemaphoreCreateMutex();

	volume_estimate = 0;
	clipping = false;

	state = IDLE;

	calculate_volume = false;
	volume_mod = 255;
}

void TX::calculate_audio_rms() {
	uint64_t rms_sum = 0;

	// audio_buffer is now mono, iterate directly
	for(int16_t sample : audio_buffer) {
		rms_sum += int32_t(sample) * sample;
	}

	rms_sum /= audio_buffer.size(); // Size is now XASAUDIO_TX_FRAME_SAMPLE_NO

	float rms_value = (sqrt(float(rms_sum)) / INT16_MAX) * sqrt(2);
	if(rms_value <= 0.0001)
		volume_estimate = -40;
	else
		volume_estimate = log10(rms_value) * 10;
}

void TX::boop_thread() {
	if(audio_task == nullptr)
		return;

	xTaskNotify(audio_task, 0, eNoAction);
}

void TX::remove_source(Source * source) {
	xSemaphoreTake(audio_config_mutex, portMAX_DELAY);

	ESP_LOGD("XasAudio", "Erasing source 0x%p", source);

	for(auto i = audio_sources.begin(); i != audio_sources.end();) {
		if(*i == source)
			i = audio_sources.erase(i);
		else
			i++;
	}

	ESP_LOGD("XasAudio", "New held count: %d", audio_sources.size());

	xSemaphoreGive(audio_config_mutex);
}

void TX::insert_source(Source * source) {
	xSemaphoreTake(audio_config_mutex, portMAX_DELAY);

	ESP_LOGD("XasAudio", "Adding source 0x%p", source);

	audio_sources.push_back(source);

	ESP_LOGD("XasAudio", "New held count: %d", audio_sources.size());
	xSemaphoreGive(audio_config_mutex);

	xTaskNotify(audio_task, 0, eNoAction);
}

// Removed add_interleaved_frame and add_lr_frame as they are stereo-specific

// Add mono frame data to the buffer
void TX::add_mono_frame(const int16_t *data, uint8_t volume) {
	for(int i=0; i < XASAUDIO_TX_FRAME_SAMPLE_NO; i++) {
		int32_t new_audio_value = audio_buffer[i];
		// Apply volume modification
		new_audio_value += (volume_mod * volume * int32_t(*data)) >> 16;

		// Clipping
		if(new_audio_value > INT16_MAX) {
			clipping = true;
			new_audio_value = INT16_MAX;
		}
		else if(new_audio_value < INT16_MIN) {
			clipping = true;
			new_audio_value = INT16_MIN;
		}

		audio_buffer[i] = int16_t(new_audio_value);
		data++; // Move to the next sample in the input data
	}
}

void TX::audio_dac_output_task() { // Task to continuously output audio samples to the DAC
	int audio_idle_count = 0;
	const uint32_t sample_period_us = 1000000 / CONFIG_XASAUDIO_TX_SAMPLERATE;
	int64_t frame_start_time, elapsed_time_us;

	while(true) {
		// As long as we haven't been idling for a while, continue playback.
		if(audio_idle_count < 0) {
			// Output samples from audio_buffer to DAC
			for (size_t i = 0; i < XASAUDIO_TX_FRAME_SAMPLE_NO; ++i) {
				frame_start_time = esp_timer_get_time();

				// audio_buffer is now mono, directly use the sample
				int16_t sample16 = audio_buffer[i];

				// Convert 16-bit signed sample (-32768 to 32767) to 8-bit unsigned (0 to 255)
				uint8_t dac_sample = (sample16 >> 8) + 128;
				
				// ESP_LOGV("XasAudioDAC", "Sample16: %d, DAC_Sample: %u", sample16, dac_sample); // Verbose logging for debugging

				dac_output_voltage(DAC_CHANNEL_1, dac_sample); // Assuming DAC_CHANNEL_1, defined in init

				elapsed_time_us = esp_timer_get_time() - frame_start_time;
				if (elapsed_time_us < sample_period_us) {
					ets_delay_us(sample_period_us - elapsed_time_us);
				} else {
					// ESP_LOGW("XasAudioDAC", "DAC output lagging: %lld us", elapsed_time_us - sample_period_us);
				}
			}
		}
		// The DAC output has been stopped, simply wait for new data, a new audio source or similar
		else {
			state = IDLE;
			xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		}

		// First things first we need to call the large processing thread
		// to see what needs to be done.
		// For mono buffer, size is XASAUDIO_TX_FRAME_SAMPLE_NO * sizeof(int16_t)
		memset(audio_buffer.data(), 0, audio_buffer.size() * sizeof(int16_t)); 
		state = PROCESSING;
		xTaskNotify(processing_task, 0, eNoAction);
		while(state == PROCESSING)
			xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		ESP_LOGV("XasAudio", "Processing finished.");

		// An audio source reported it has some audio to play
		if(state == RUNNING) {
			// We had put the dac to sleep, unpause it.
			if(audio_idle_count == 0) {
				ESP_LOGD("XasAudio", "Transitioning to running.");
				// DAC is enabled in init, no specific "start" needed here for dac_output_voltage
			}

			// Reset the idle counter.
			audio_idle_count = -10;
		}
		// No more audio was added, increment idle counter
		// We can't immediately stop playing as the buffer must be emptied.
		else if(audio_idle_count < 0) {
			audio_idle_count++;

			// We reached our four block idle time, stop the DAC output
			// This helps conserve energy and processing power
			if(audio_idle_count == 0) {
				ESP_LOGD("XasAudio", "Transitioning to idle.");
				dac_output_voltage(DAC_CHANNEL_1, 128); // Output silence (mid-point for 8-bit DAC)
			}
		}
	}
}

bool TX::largestack_process() {
	if(state != PROCESSING)
		return false;

	ESP_LOGV("XasAudio", "Processing next batch.");

	// We need to get the FreeRTOS Mutex to prevent concurrent modification
	// of the audio sources list.
	xSemaphoreTake(audio_config_mutex, portMAX_DELAY);
	std::vector<Source *> sources_copy = audio_sources;
	xSemaphoreGive(audio_config_mutex);

	bool source_is_playing = false;
	for(auto source : sources_copy) {
		source_is_playing |= source->process_frame();
	}

	if(calculate_volume)
		calculate_audio_rms();
	else
		volume_estimate = 0;

	if(source_is_playing)
		state = RUNNING;
	else
		state = PRE_IDLE;

	xTaskNotify(audio_task, 0, eNoAction);

	for (auto source : sources_copy)
	{
		// FIXME This really should be a std::shared_pointer, it's a perfect
		// use case, I just need to muster the courage to use it :P
		if (source->is_finished() && source->can_be_deleted())
		{
			ESP_LOGD("XasAudio", "Starting delete of 0x%p", source);
			delete source;
		}
	}

	return true;
}

void TX::init(TaskHandle_t processing_task) { // Initializes DAC and audio processing task
	// DAC initialization
    ESP_LOGI("XasAudio", "Initializing DAC for audio output.");
    dac_output_enable(DAC_CHANNEL_1); // Enable DAC output on channel 1 (GPIO25)
    // dac_output_enable(DAC_CHANNEL_2); // Enable DAC output on channel 2 (GPIO26) if stereo DAC needed
    
    // Disable Cosine Wave (CW) generator.
    // The CW generator can interfere with DAC output if not disabled.
    dac_cw_generator_disable(); 

	this->processing_task = processing_task;

	xTaskCreate(start_audio_task, "XasAudio DAC Output", 3*1024, this, 7, &audio_task);
}

float TX::get_volume_estimate() {
	return volume_estimate;
}

}
}
