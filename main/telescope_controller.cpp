#include "telescope_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // For strlen, strcpy, etc.
#include <stdio.h>  // For snprintf
#include "EspMeshHandler.h"

#define MAX_AZ_POSITION 360
#define MAX_EL_POSITION 81
#define MIN_AZ_POSITION 0
#define MIN_EL_POSITION 15

extern Xasin::Communication::EspMeshHandler g_mesh_handler;

static const char *TAG = "TelescopeCtrl";

// Helper function to introduce delays
static void delay_ms(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

TelescopeController::TelescopeController() : initialized(false), brakes_are_off(false) {
    // Constructor
}

TelescopeController::~TelescopeController() {
    if (initialized) {
        ESP_LOGI(TAG, "Telescope controller cleaned up.");
    }
}

static void response_callback(const Xasin::Communication::CommReceivedData& data) {
    ESP_LOGI(TAG, "Received response on topic: %s", data.topic.c_str());
    ESP_LOGI(TAG, "Response payload: %.*s", static_cast<int>(data.payload.size()), data.payload.data());
    // Process the response as needed, e.g., store it in a buffer or trigger an event.
}

esp_err_t TelescopeController::init() {
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    if (!g_mesh_handler.isConnected()) {
        ESP_LOGE(TAG, "Mesh handler not connected. Cannot initialize telescope controller.");
        return ESP_FAIL;
    }

    // Subscribe to the response topic
    bool sub_success = g_mesh_handler.subscribe("telescope/responses", response_callback, 1);
    if (!sub_success) {
        ESP_LOGE(TAG, "Failed to subscribe to response topic.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Telescope controller initialized using mesh handler.");
    initialized = true;
    return ESP_OK;
}

esp_err_t TelescopeController::send_command(const char* cmd, bool add_cr, int send_delay_ms, int recv_delay_ms) {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized, cannot send command.");
        return ESP_FAIL;
    }

    std::string full_cmd(cmd);
    if (add_cr) {
        full_cmd += "\r";
    }

    bool success = g_mesh_handler.publish("telescope/commands", full_cmd.c_str(), full_cmd.length(), false, 1);
    if (!success) {
        ESP_LOGE(TAG, "Failed to publish command: %s", cmd);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published command: %s", cmd);
    if (send_delay_ms > 0) {
        delay_ms(send_delay_ms);
    }

    return ESP_OK;
}

esp_err_t TelescopeController::read_response(char* buffer, size_t buffer_len, int timeout_ms) {
    ESP_LOGW(TAG, "read_response is not implemented for mesh handler. Use a subscription callback instead.");
    return ESP_ERR_NOT_SUPPORTED;
}

void TelescopeController::startup_sequence() {
    ESP_LOGI(TAG, "Starting telescope startup sequence...");

    const char* reset_cmds[] = {"2A01RST", "2A01RST", "2A02RST", "2A03RST"};
    for (const char* cmd : reset_cmds) {
        send_command(cmd, true, 8, 0);
    }
    ESP_LOGI(TAG, "Reset sequence sent.");

    const char* enable_cmds[] = {"2A01EN1", "2A02EN1", "2A03EN1"};
    for (const char* cmd : enable_cmds) {
        send_command(cmd, true, 8, 0);
    }
    ESP_LOGI(TAG, "Amplifiers enabled.");

    send_command("VER", true, 8, 0);
    send_command("STS", true, 8, 0);

    ESP_LOGI(TAG, "Startup sequence complete.");
    brakes_are_off = false;
}

void TelescopeController::brakes_off() {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized.");
        return;
    }

    ESP_LOGI(TAG, "Turning brakes OFF.");
    send_command("ABF", true, 8, 0);
    send_command("EBF", true, 8, 0);
    brakes_are_off = true;
    ESP_LOGI(TAG, "Brake OFF commands sent.");
}

void TelescopeController::process_joystick_input(const JoystickState_t* joystick_state) {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized. Cannot process joystick.");
        return;
    }

    if (!brakes_are_off) {
        ESP_LOGI(TAG, "Brakes are not off. Attempting to turn off brakes first.");
        brakes_off();
        if (!brakes_are_off) { // Check again if brakes_off succeeded (basic check)
             ESP_LOGE(TAG, "Failed to turn off brakes. Cannot move telescope.");
             return;
        }
    }

    // Joystick mapping constants
    const int JOYSTICK_MIN = 0;
    const int JOYSTICK_MAX = 4095;
    const int JOYSTICK_CENTER = (JOYSTICK_MAX + JOYSTICK_MIN) / 2; // 2047

    int az_position = 0;
    int el_position = 0;

    // Map joystick X (azimuth)
    int dx = joystick_state->y - JOYSTICK_CENTER;
    if (abs(dx) > JOYSTICK_DEADZONE) {
        az_position = (dx * MAX_AZ_POSITION) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        // Clamp to limits
        if (az_position > MAX_AZ_POSITION) az_position = MAX_AZ_POSITION;
        if (az_position < MIN_AZ_POSITION) az_position = MIN_AZ_POSITION;
    } else {
        az_position = 0;
    }

    // Map joystick Y (elevation)
    int dy = joystick_state->x - JOYSTICK_CENTER;
    if (abs(dy) > JOYSTICK_DEADZONE) {
        el_position = (dy * MAX_EL_POSITION) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        if (el_position > MAX_EL_POSITION) el_position = MAX_EL_POSITION;
        if (el_position < MIN_EL_POSITION) el_position = MIN_EL_POSITION;
    } else {
        el_position = 0;
    }

    char command_buffer[64];
    snprintf(command_buffer, sizeof(command_buffer), "AZEL,%d,%d", az_position, el_position);
    send_command(command_buffer);

    ESP_LOGI(TAG, "Joystick mapped to AZEL command: %s", command_buffer);
}

// Placeholder for other methods if needed
// int TelescopeController::map_joystick_to_speed(int joystick_val, int max_speed) {
//     // Implementation for mapping
//     return 0;
// }
