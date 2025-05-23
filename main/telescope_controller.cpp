#include "telescope_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // For strlen, strcpy, etc.
#include <stdio.h>  // For snprintf
#include "xasin/mqtt/Handler.h"

#define MAX_AZ_POSITION 360
#define MAX_EL_POSITION 81
#define MIN_AZ_POSITION 0
#define MIN_EL_POSITION 15

extern Xasin::MQTT::Handler mqtt;

static const char *TAG = "TelescopeCtrl";

TelescopeController::TelescopeController() : initialized(false) {
    // Constructor
}

TelescopeController::~TelescopeController() {
    if (initialized) {
        ESP_LOGI(TAG, "Telescope controller cleaned up.");
    }
}

// Variables to store the actual Az/El values
static float actual_az = 0.0f;
static float actual_el = 0.0f;

static void response_callback(const Xasin::MQTT::MQTT_Packet& data) {
    ESP_LOGI(TAG, "Received response on topic: %s", data.topic.c_str());
    ESP_LOGI(TAG, "Response payload: %.*s", static_cast<int>(data.data.size()), data.data.data());

    // Parse the response to extract Az/El values
    float az, el;
    if (sscanf(data.data.data(), "AZ %f EL %f", &az, &el) == 2) {
        actual_az = az;
        actual_el = el;
    }
}

void TelescopeController::get_actual_azel(float& az, float& el) const {
    az = actual_az;
    el = actual_el;
}

esp_err_t TelescopeController::init() {
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    if (!mqtt.is_disconnected()) {
        mqtt.subscribe_to("telescope/responses", response_callback, 1);
        ESP_LOGI(TAG, "Telescope controller initialized and subscribed to response topic.");
        initialized = true;
    }
    else ESP_LOGE(TAG, "Mqtt handler not connected. Cannot initialize telescope controller.");
    return ESP_OK;
}

esp_err_t TelescopeController::send_command(const char* cmd) {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized, cannot send command.");
        return ESP_FAIL;
    }

    mqtt.publish_to("telescope/AZEL", cmd, strlen(cmd), false, 1);

    ESP_LOGI(TAG, "Published command: %s", cmd);

    return ESP_OK;
}

void TelescopeController::process_joystick_input(const JoystickState_t* joystick_state) {

    // Joystick mapping constants
    const int JOYSTICK_MIN = 0;
    const int JOYSTICK_MAX = 4095;
    const int JOYSTICK_CENTER = (JOYSTICK_MAX + JOYSTICK_MIN) / 2; // 2047

    int az_position = 180; // Starting value
    int el_position = 81;  // Starting value

    // Increment Alt/Az based on joystick offset
    int dx = joystick_state->y - JOYSTICK_CENTER;
    if (abs(dx) > JOYSTICK_DEADZONE) {
        az_position += (dx * MAX_AZ_POSITION) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        // Clamp to limits
        if (az_position > MAX_AZ_POSITION) az_position = MAX_AZ_POSITION;
        if (az_position < MIN_AZ_POSITION) az_position = MIN_AZ_POSITION;
    }

    int dy = joystick_state->x - JOYSTICK_CENTER;
    if (abs(dy) > JOYSTICK_DEADZONE) {
        el_position += (dy * MAX_EL_POSITION) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        if (el_position > MAX_EL_POSITION) el_position = MAX_EL_POSITION;
        if (el_position < MIN_EL_POSITION) el_position = MIN_EL_POSITION;
    }

    // Send command only when a button is pressed
    if (joystick_state->button_pressed) {
        char command_buffer[64];
        snprintf(command_buffer, sizeof(command_buffer), "%d,%d", az_position, el_position);
        send_command(command_buffer);
        ESP_LOGI(TAG, "Joystick mapped to AZEL command: %s", command_buffer);
    }
}

void TelescopeController::update_actual_azel(float az, float el) {
    actual_az = az;
    actual_el = el;
    ESP_LOGI(TAG, "Updated actual Az/El to: %.1f / %.1f", az, el);
}
