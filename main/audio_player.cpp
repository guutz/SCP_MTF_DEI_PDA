#include "audio_player.h"
#include "sd_raw_access.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include <string.h>

#define TAG "audio_player"

#define AUDIO_BUFFER_SIZE 1024

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

// Audio playback task function prototype
static void audio_player_task(void *pvParameters);

esp_err_t audio_player_init(dac_channel_t dac_channel) {
    if (dac_channel != DAC_CHANNEL_1 && dac_channel != DAC_CHANNEL_2) {
        ESP_LOGE(TAG, "Invalid DAC channel. Must be DAC_CHANNEL_1 or DAC_CHANNEL_2");
        return ESP_ERR_INVALID_ARG;
    }
    
    s_dac_channel = dac_channel;
    
    // Initialize DAC
    esp_err_t ret = dac_output_enable(s_dac_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable DAC output: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create mutex if it doesn't exist
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
    
    // First stop any currently playing audio
    audio_player_stop();
    
    // Allocate memory for task parameters (copy of file_path and sample rate)
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
    
    // Mark as playing and reset stop flag before starting task
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_is_playing = true;
        s_stop_requested = false;
        xSemaphoreGive(s_audio_mutex);
    }
    
    // Create audio playback task
    BaseType_t task_created = xTaskCreate(
        audio_player_task,
        "audio_player",
        4096,  // Stack size
        params, // Parameters
        tskIDLE_PRIORITY + 1, // Priority
        &s_audio_task_handle  // Task handle
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
        return ESP_OK; // Nothing to stop
    }
    
    // Request task to stop
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_stop_requested = true;
        xSemaphoreGive(s_audio_mutex);
    }
    
    // Wait for task to finish (with timeout)
    const TickType_t max_wait = pdMS_TO_TICKS(1000); // 1 second timeout
    TickType_t start_time = xTaskGetTickCount();
    
    while (s_is_playing && 
           ((xTaskGetTickCount() - start_time) < max_wait)) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
    
    // If task didn't stop gracefully, delete it
    if (s_is_playing) {
        vTaskDelete(s_audio_task_handle);
        s_audio_task_handle = NULL;
        
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            xSemaphoreGive(s_audio_mutex);
        }
    }
    
    // Set DAC output to mid-point (silence)
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

// Audio player task function
static void audio_player_task(void *pvParameters) {
    audio_task_params_t* params = (audio_task_params_t*)pvParameters;
    if (params == NULL) {
        ESP_LOGE(TAG, "Task parameters are NULL");
        
        // Mark as not playing and free resources
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            s_stop_requested = false;
            xSemaphoreGive(s_audio_mutex);
        }
        vTaskDelete(NULL);
        return;
    }
    
    char* file_path = params->file_path;
    uint32_t sample_rate = params->sample_rate;
    
    if (file_path == NULL) {
        ESP_LOGE(TAG, "File path is NULL in task parameters");
        
        // Mark as not playing and free resources
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            s_stop_requested = false;
            xSemaphoreGive(s_audio_mutex);
        }
        
        // Free parameters
        free(params);
        
        vTaskDelete(NULL);
        return;
    }
    
    // Open the file
    FILE* audio_file = sd_raw_fopen(file_path, "rb");
    if (audio_file == NULL) {
        ESP_LOGE(TAG, "Failed to open audio file: %s", file_path);
        
        // Mark as not playing and free resources
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            s_stop_requested = false;
            xSemaphoreGive(s_audio_mutex);
        }
        
        // Free parameters
        if (params->file_path != NULL) {
            free(params->file_path);
        }
        free(params);
        
        vTaskDelete(NULL);
        return;
    }
    
    // Calculate delay between samples based on sample rate
    const TickType_t sample_period_us = 1000000 / sample_rate; // Period in microseconds
    
    // Allocate buffer for audio data
    uint8_t* audio_buffer = (uint8_t*)malloc(AUDIO_BUFFER_SIZE);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        sd_raw_fclose(audio_file);
        
        // Mark as not playing and free resources
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            s_is_playing = false;
            s_stop_requested = false;
            xSemaphoreGive(s_audio_mutex);
        }
        
        // Free parameters
        if (params->file_path != NULL) {
            free(params->file_path);
        }
        free(params);
        
        vTaskDelete(NULL);
        return;
    }
    
    // Main playback loop
    size_t bytes_read;
    bool stop_requested = false;
    
    while (true) {
        // Check if stop was requested
        if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            stop_requested = s_stop_requested;
            xSemaphoreGive(s_audio_mutex);
        }
        
        if (stop_requested) {
            ESP_LOGI(TAG, "Stop requested, ending playback");
            break;
        }
        
        // Read a chunk of audio data
        bytes_read = sd_raw_fread(audio_buffer, 1, AUDIO_BUFFER_SIZE, audio_file);
        
        // If we reached the end of the file or got an error, stop playback
        if (bytes_read == 0) {
            ESP_LOGI(TAG, "End of audio file reached");
            break;
        }
        
        // Output each sample to the DAC with proper timing
        int64_t start_time, elapsed_time;
        
        for (size_t i = 0; i < bytes_read; i++) {
            start_time = esp_timer_get_time();
            
            // Output the sample to the DAC (8-bit unsigned PCM)
            dac_output_voltage(s_dac_channel, audio_buffer[i]);
            
            // Check if stop was requested every 32 samples
            if ((i & 0x1F) == 0) { // Check every 32 samples
                if (xSemaphoreTake(s_audio_mutex, 0) == pdTRUE) {
                    stop_requested = s_stop_requested;
                    xSemaphoreGive(s_audio_mutex);
                    
                    if (stop_requested) {
                        ESP_LOGI(TAG, "Stop requested during playback");
                        break;
                    }
                }
            }
            
            // Calculate time spent and delay appropriately
            elapsed_time = esp_timer_get_time() - start_time;
            int32_t delay_time = sample_period_us - elapsed_time;
            
            if (delay_time > 0) {
                // Use ets_delay_us for short delays
                ets_delay_us(delay_time);
            }
        }
        
        if (stop_requested) {
            break;
        }
    }
    
    // Cleanup audio resources
    free(audio_buffer);
    sd_raw_fclose(audio_file);
    
    // Mark as not playing
    if (xSemaphoreTake(s_audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        s_is_playing = false;
        s_stop_requested = false;
        xSemaphoreGive(s_audio_mutex);
    }
    
    // Free task parameters
    if (params != NULL) {
        if (params->file_path != NULL) {
            free(params->file_path);
        }
        free(params);
    }
    
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
}
