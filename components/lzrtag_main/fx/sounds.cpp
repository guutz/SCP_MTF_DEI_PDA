/*
 * sounds.cpp
 *
 *  Created on: 30 Mar 2019
 *      Author: xasin
 */

#include "xasin/audio.h"
#include "lzrtag/sounds.h"
#include "xasin/mqtt/Handler.h"
#include "driver/dac.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "sd_raw_access.h"
#include <string.h>

#define TAG "audio_player"

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

#define AUDIO_BUFFER_SIZE 8192

// Structure for passing parameters to the audio task
typedef struct {
    char* file_path;      // Path to the audio file
    uint32_t sample_rate; // Sample rate in Hz
} audio_task_params_t;

static TaskHandle_t s_audio_task_handle = NULL;
static SemaphoreHandle_t s_audio_mutex = NULL;
static volatile bool s_is_playing = false;
static volatile bool s_stop_requested = false;
static dac_channel_t s_dac_channel = DAC_CHANNEL_1; // Default to DAC channel 1

void audio_player_task(void *pvParameters);

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

SoundManager::SoundManager(Xasin::MQTT::Handler* commHandler)
    : comm_handler_(commHandler) {}

void SoundManager::play_audio(const std::string& aName) {
    std::string file_path;
    uint32_t sample_rate = DEFAULT_FX_SAMPLERATE;

    if (aName == "GAME START")
        file_path = GAME_START_SOUND_PATH;
    else if (aName == "KILL SCORE")
        file_path = KILL_SCORE_SOUND_PATH;
    else if (aName == "MINOR SCORE")
        file_path = MINOR_SCORE_SOUND_PATH;
    else if (aName == "DEATH")
        file_path = OWN_DEATH_SOUND_PATH;
    else if (aName == "HIT")
        file_path = OWN_HIT_SOUND_PATH;
    else if (aName == "CLICK")
        file_path = EMPTY_CLICK_SOUND_PATH;
    else if (aName == "RELOAD FULL")
        file_path = RELOAD_FULL_SOUND_PATH;
    else if (aName == "DENY")
        file_path = DENYBEEP_SOUND_PATH;
    else if (aName == "ride") {
        file_path = "DEI/sounds/ride16k.raw";
        sample_rate = 16000;
    }
    else {
        file_path = aName;
    }

    esp_err_t err = audio_player_play_file(file_path.c_str(), sample_rate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play audio file: %s", file_path.c_str());
    }
}

void SoundManager::init() {
    // Initialize the audio player with the default DAC channel
    esp_err_t err = audio_player_init(DAC_CHANNEL_2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio player");
        return;
    }

    // Subscribe to MQTT topic for sound commands
    comm_handler_->subscribe_to("Sound/#", [this](Xasin::MQTT::MQTT_Packet data) {
        // Only expect the payload to be the sound name
        play_audio(std::string(data.data.begin(), data.data.end()));
    });
}

esp_err_t audio_player_init(dac_channel_t dac_channel) {
    if (dac_channel != DAC_CHANNEL_1 && dac_channel != DAC_CHANNEL_2) {
        ESP_LOGE(TAG, "Invalid DAC channel. Must be DAC_CHANNEL_1 or DAC_CHANNEL_2");
        return ESP_ERR_INVALID_ARG;
    }
    s_dac_channel = dac_channel;
    esp_err_t ret = dac_output_enable(s_dac_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable DAC output: %s", esp_err_to_name(ret));
        return ret;
    }
    if (s_audio_mutex == NULL) {
        s_audio_mutex = xSemaphoreCreateMutex();
        if (s_audio_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create audio mutex");
            return ESP_FAIL;
        }
    }
    ESP_LOGI(TAG, "Audio player initialized with DAC channel %d", s_dac_channel);
    return ESP_OK;
}

esp_err_t audio_player_play_file(const char* file_path, uint32_t sample_rate) {
    if (file_path == NULL) {
        ESP_LOGE(TAG, "File path is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (sample_rate == 0) {
        ESP_LOGE(TAG, "Sample rate must be greater than 0");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_raw_file_exists(file_path)) {
        ESP_LOGE(TAG, "File not found: %s", file_path);
        return ESP_ERR_NOT_FOUND;
    }
    audio_player_stop();
    char* file_path_copy = strdup(file_path);
    if (file_path_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for file path");
        return ESP_ERR_NO_MEM;
    }
    audio_task_params_t* params = (audio_task_params_t*)malloc(sizeof(audio_task_params_t));
    if (params == NULL) {
        free(file_path_copy);
        ESP_LOGE(TAG, "Failed to allocate memory for task parameters");
        return ESP_ERR_NO_MEM;
    }
    params->file_path = file_path_copy;
    params->sample_rate = sample_rate;
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_is_playing = true;
        s_stop_requested = false;
        xSemaphoreGive(s_audio_mutex);
    }
    BaseType_t task_created = xTaskCreate(
        audio_player_task,
        "audio_player",
        4096,
        params,
        tskIDLE_PRIORITY + 1,
        &s_audio_task_handle
    );
    if (task_created != pdPASS) {
        free(file_path_copy);
        free(params);
        ESP_LOGE(TAG, "Failed to create audio player task");
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            xSemaphoreGive(s_audio_mutex);
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Started playback of file: %s at %uHz", file_path, sample_rate);
    return ESP_OK;
}

esp_err_t audio_player_stop(void) {
    if (!s_is_playing || s_audio_task_handle == NULL) {
        return ESP_OK;
    }
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_stop_requested = true;
        xSemaphoreGive(s_audio_mutex);
    }
    const TickType_t max_wait = pdMS_TO_TICKS(1000);
    TickType_t start_time = xTaskGetTickCount();
    while (s_is_playing && ((xTaskGetTickCount() - start_time) < max_wait)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (s_is_playing) {
        vTaskDelete(s_audio_task_handle);
        s_audio_task_handle = NULL;
    }
    dac_output_voltage(s_dac_channel, 128);
    ESP_LOGI(TAG, "Audio playback stopped");
    return ESP_OK;
}

bool audio_player_is_playing(void) {
    bool playing = false;
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        playing = s_is_playing;
        xSemaphoreGive(s_audio_mutex);
    }
    return playing;
}

void audio_player_task(void *pvParameters) {
    audio_task_params_t* params = (audio_task_params_t*)pvParameters;
    if (params == NULL) {
        vTaskDelete(NULL);
        return;
    }
    char* file_path = params->file_path;
    uint32_t sample_rate = params->sample_rate;
    if (file_path == NULL) {
        free(params);
        vTaskDelete(NULL);
        return;
    }
    FILE* audio_file = sd_raw_fopen(file_path, "rb");
    if (audio_file == NULL) {
        free(params);
        vTaskDelete(NULL);
        return;
    }
    const TickType_t sample_period_us = 1000000 / sample_rate;
    uint8_t* audio_buffer = (uint8_t*)malloc(AUDIO_BUFFER_SIZE);
    if (audio_buffer == NULL) {
        sd_raw_fclose(audio_file);
        free(params);
        vTaskDelete(NULL);
        return;
    }
    size_t bytes_read;
    while (!s_stop_requested && (bytes_read = fread(audio_buffer, 1, AUDIO_BUFFER_SIZE, audio_file)) > 0) {
        for (size_t i = 0; i < bytes_read; ++i) {
            dac_output_voltage(s_dac_channel, audio_buffer[i]);
            ets_delay_us(sample_period_us);
        }
    }
    free(audio_buffer);
    sd_raw_fclose(audio_file);
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_is_playing = false;
        xSemaphoreGive(s_audio_mutex);
    }
    free(params);
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
}

} // namespace Sounds
} // namespace LZR