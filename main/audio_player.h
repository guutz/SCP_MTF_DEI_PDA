#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dac.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the audio player
 * 
 * @param dac_channel DAC channel to use (DAC_CHANNEL_1 or DAC_CHANNEL_2)
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t audio_player_init(dac_channel_t dac_channel);

/**
 * @brief Play an 8-bit raw PCM audio file from the SD card
 * 
 * This function starts a task that reads the audio file in chunks and outputs 
 * it to the DAC. It returns immediately, while playback continues in the background.
 * 
 * @param file_path Path to the audio file (relative to SD card mount point)
 * @param sample_rate Sample rate of the audio file in Hz (e.g., 8000, 16000, 22050, 44100)
 * @return ESP_OK if playback started successfully, ESP_FAIL otherwise
 */
esp_err_t audio_player_play_file(const char* file_path, uint32_t sample_rate);

/**
 * @brief Stop the currently playing audio
 * 
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t audio_player_stop(void);

/**
 * @brief Check if audio playback is active
 * 
 * @return true if audio is playing, false otherwise
 */
bool audio_player_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PLAYER_H
