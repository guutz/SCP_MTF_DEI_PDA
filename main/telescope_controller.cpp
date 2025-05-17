#include "telescope_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // For strlen, strcpy, etc.
#include <stdio.h>  // For snprintf

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
        uart_driver_delete(TELESCOPE_UART_NUM);
        ESP_LOGI(TAG, "UART driver deleted");
    }
}

esp_err_t TelescopeController::init() {
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    uart_config_t uart_config = {
        .baud_rate = TELESCOPE_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB, // Or UART_SCLK_REF_TICK based on ESP-IDF version/config
    };

    ESP_LOGI(TAG, "Installing UART driver");
    esp_err_t err = uart_driver_install(TELESCOPE_UART_NUM, 256 * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Configuring UART parameters");
    err = uart_param_config(TELESCOPE_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(err));
        uart_driver_delete(TELESCOPE_UART_NUM);
        return err;
    }

    ESP_LOGI(TAG, "Setting UART pins");
    err = uart_set_pin(TELESCOPE_UART_NUM, TELESCOPE_TXD_PIN, TELESCOPE_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        uart_driver_delete(TELESCOPE_UART_NUM);
        return err;
    }
    
    ESP_LOGI(TAG, "UART Initialized on TX: %d, RX: %d", TELESCOPE_TXD_PIN, TELESCOPE_RXD_PIN);

    // Clear UART buffer (similar to python script sending 8x \r)
    char dummy_buffer[32];
    for (int i = 0; i < 8; ++i) {
        uart_write_bytes(TELESCOPE_UART_NUM, "\r", 1);
        delay_ms(3);
    }
    // Attempt to read any remaining junk
    uart_read_bytes(TELESCOPE_UART_NUM, (uint8_t*)dummy_buffer, sizeof(dummy_buffer) -1, pdMS_TO_TICKS(100));


    startup_sequence();

    initialized = true;
    ESP_LOGI(TAG, "Telescope controller initialized successfully.");
    return ESP_OK;
}

esp_err_t TelescopeController::send_command(const char* cmd, bool add_cr, int send_delay_ms, int recv_delay_ms) {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized, cannot send command.");
        return ESP_FAIL;
    }

    char full_cmd[64]; // Assuming commands are not excessively long
    strcpy(full_cmd, cmd);
    if (add_cr) {
        strcat(full_cmd, "\r");
    }

    int len = strlen(full_cmd);
    int bytes_written = uart_write_bytes(TELESCOPE_UART_NUM, full_cmd, len);
    if (bytes_written != len) {
        ESP_LOGE(TAG, "Failed to write full command. Expected %d, wrote %d for: %s", len, bytes_written, cmd);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Sent: %s", cmd); // Log original command without \r for clarity
    if (send_delay_ms > 0) {
        delay_ms(send_delay_ms);
    }

    // Response reading is handled separately by read_response or directly after this call
    // The recv_delay_ms here is more of a post-send delay before the next action by the caller
    if (recv_delay_ms > 0) {
        delay_ms(recv_delay_ms);
    }
    return ESP_OK;
}

esp_err_t TelescopeController::read_response(char* buffer, size_t buffer_len, int timeout_ms) {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized, cannot read response.");
        return ESP_FAIL;
    }
    if (buffer == nullptr || buffer_len == 0) {
        ESP_LOGE(TAG, "Invalid buffer for reading response.");
        return ESP_ERR_INVALID_ARG;
    }

    // Read until \r, \n, or timeout
    // This is a simplified version. A more robust implementation would handle partial reads
    // and different line endings more gracefully.
    int bytes_read = uart_read_bytes(TELESCOPE_UART_NUM, (uint8_t*)buffer, buffer_len - 1, pdMS_TO_TICKS(timeout_ms));
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        // Strip trailing \r and \n
        for (int i = bytes_read - 1; i >= 0; --i) {
            if (buffer[i] == '\r' || buffer[i] == '\n') {
                buffer[i] = '\0';
            } else {
                break;
            }
        }
        ESP_LOGI(TAG, "Recv: %s (len: %d)", buffer, strlen(buffer));
    } else if (bytes_read == 0) {
        buffer[0] = '\0'; // Empty response
        ESP_LOGI(TAG, "Recv: (empty, timeout)");
    } else { // bytes_read < 0 indicates error
        buffer[0] = '\0';
        ESP_LOGE(TAG, "UART read error");
        return ESP_FAIL;
    }
    return ESP_OK;
}


void TelescopeController::startup_sequence() {
    ESP_LOGI(TAG, "Starting telescope startup sequence...");
    char response_buffer[128]; // Buffer for responses

    // Resetting telescope
    const char* reset_cmds[] = {"2A01RST", "2A01RST", "2A02RST", "2A03RST"};
    for (const char* cmd : reset_cmds) {
        send_command(cmd, true, 8, 0); // 8ms send delay, recv handled by read_response
        read_response(response_buffer, sizeof(response_buffer), 105); // 105ms recv delay/timeout
    }
    ESP_LOGI(TAG, "Reset sequence sent.");

    // Enable amplifiers
    const char* enable_cmds[] = {"2A01EN1", "2A02EN1", "2A03EN1"};
    for (const char* cmd : enable_cmds) {
        send_command(cmd, true, 8, 0);
        read_response(response_buffer, sizeof(response_buffer), 5); // 5ms recv delay/timeout
    }
    ESP_LOGI(TAG, "Amplifiers enabled.");

    // Check software
    // Delay 5ms (handled by send_command's recv_delay or an explicit delay if needed)
    delay_ms(5); 
    send_command("VER", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5);
    
    send_command("VER", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5);
    if (strstr(response_buffer, "Ver") != nullptr) {
        ESP_LOGI(TAG, "Servo controller OK: %s", response_buffer);
    } else {
        ESP_LOGW(TAG, "Servo controller version check failed or unexpected response: %s", response_buffer);
    }

    // STS commands (expecting length 20, but we don't check strictly here)
    send_command("STS", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5);
    send_command("STS", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5);

    // Other checks from notes (simplified for now)
    send_command("2R01", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5); // Expect non-empty

    send_command("2A01EN", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5); // Expect len >= 5 for amps OK

    send_command("7VDC", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5); // Expect non-empty for "Feb" OK

    send_command("ERR", true, 8, 0);
    read_response(response_buffer, sizeof(response_buffer), 5);

    ESP_LOGI(TAG, "Startup sequence complete.");
    // According to Python, brakes might be on. We might need to turn them off
    // if we want to move immediately after startup.
    // For now, let's assume they need to be explicitly turned off before movement.
    brakes_are_off = false; // Explicitly state brakes are not confirmed off yet.
}

void TelescopeController::brakes_off() {
    if (!initialized) {
        ESP_LOGE(TAG, "Controller not initialized.");
        return;
    }
    // In Python, get_info() is called before sending brake commands.
    // For simplicity, we'll just send the commands. A more robust version
    // would check status first.
    ESP_LOGI(TAG, "Turning brakes OFF.");
    char response_buffer[32];
    send_command("ABF", true, 8, 0); // Azimuth Brake Off
    read_response(response_buffer, sizeof(response_buffer), 100); // Allow some time for response/action
    send_command("EBF", true, 8, 0); // Elevation Brake Off
    read_response(response_buffer, sizeof(response_buffer), 100);
    brakes_are_off = true; // Assume successful for now
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
    
    // Ensure telescope is not in tracking mode if we are sending direct speed commands
    // send_command("TON", true, 8, 5); // TON might be for "tracking on", SPA for "stop/park axis"
                                     // The python script uses TON before AZL (point)
                                     // For direct speed control (AZV, ELV), SPA might be more appropriate to ensure it's not fighting a track
    // Let's assume for now that being out of tracking mode is handled or not an issue.
    // Or send SPA to be sure:
    // send_command("SPA", true, 8, 5); // Stop Previous Activity / Park Axis

    // Joystick mapping constants
    const int JOYSTICK_MIN = 0;
    const int JOYSTICK_MAX = 4095;
    const int JOYSTICK_CENTER = (JOYSTICK_MAX + JOYSTICK_MIN) / 2; // 2047
    // Use your existing deadzone value

    int az_speed = 0;
    int el_speed = 0;

    // Map joystick X (azimuth)
    int dx = joystick_state->y - JOYSTICK_CENTER;
    if (abs(dx) > JOYSTICK_DEADZONE) {
        az_speed = (dx * MAX_AZ_SPEED) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        // Clamp to limits
        if (az_speed > MAX_AZ_SPEED) az_speed = MAX_AZ_SPEED;
        if (az_speed < -MAX_AZ_SPEED) az_speed = -MAX_AZ_SPEED;
    } else {
        az_speed = 0;
    }

    // Map joystick Y (elevation)
    int dy = joystick_state->x - JOYSTICK_CENTER;
    if (abs(dy) > JOYSTICK_DEADZONE) {
        el_speed = (dy * MAX_EL_SPEED) / (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
        if (el_speed > MAX_EL_SPEED) el_speed = MAX_EL_SPEED;
        if (el_speed < -MAX_EL_SPEED) el_speed = -MAX_EL_SPEED;
    } else {
        el_speed = 0;
    }

    char command_buffer[32];
    if (az_speed != 0) {
        snprintf(command_buffer, sizeof(command_buffer), "AZV,%d", az_speed);
        send_command(command_buffer);
    } else {
        send_command("AZV,0"); 
    }

    if (el_speed != 0) {
        snprintf(command_buffer, sizeof(command_buffer), "ELV,%d", el_speed);
        send_command(command_buffer);
    } else {
        send_command("ELV,0");
    }
    
    // ESP_LOGD(TAG, "Joystick X: %d -> AzSpeed: %d, Joystick Y: %d -> ElSpeed: %d", joystick_state->x, az_speed, joystick_state->y, el_speed);
}

// Placeholder for other methods if needed
// int TelescopeController::map_joystick_to_speed(int joystick_val, int max_speed) {
//     // Implementation for mapping
//     return 0;
// }
