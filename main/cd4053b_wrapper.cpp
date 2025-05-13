#include "cd4053b_wrapper.h"
#include "mcp23008_wrapper.h" // For mcp23008_wrapper_write_pin and MCP23008_NamedPin
#include "esp_log.h"          // For ESP_LOGx macros

static const char *TAG_CD4053B = "cd4053b_wrapper";

esp_err_t cd4053b_set_switch_state(mcp23008_t *mcp, CD4053B_ControllableSwitch sw, bool state) {
    if (!mcp) {
        ESP_LOGE(TAG_CD4053B, "MCP23008 device pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    MCP23008_NamedPin control_pin;

    switch (sw) {
        case CD4053B_SWITCH_S1:
            control_pin = MCP_PIN_CD4053B_S1;
            break;
        case CD4053B_SWITCH_S2:
            control_pin = MCP_PIN_CD4053B_S2;
            break;
        case CD4053B_SWITCH_S3:
            control_pin = MCP_PIN_CD4053B_S3;
            break;
        default:
            ESP_LOGE(TAG_CD4053B, "Invalid CD4053B switch specified: %d", sw);
            return ESP_ERR_INVALID_ARG;
    }

    // ESP_LOGI(TAG_CD4053B, "Setting switch %d (MCP pin %d) to state %s", sw, control_pin, state ? "HIGH (channel 1)" : "LOW (channel 0)");
    return mcp23008_wrapper_write_pin(mcp, control_pin, state);
}

esp_err_t cd4053b_select_named_path(mcp23008_t *mcp, CD4053B_NamedPath path) {
    if (!mcp) {
        ESP_LOGE(TAG_CD4053B, "MCP23008 device pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    MCP23008_NamedPin control_pin;
    bool pin_state; // false for LOW (path 0), true for HIGH (path 1)

    switch (path) {
        // --- Switch S1 Paths ---
        case CD4053B_S1_PATH_A0_JOYSTICK_AXIS1:
            control_pin = MCP_PIN_CD4053B_S1;
            pin_state = false; // Selects path 0 for S1
            break;
        case CD4053B_S1_PATH_A1_BATT_SENSE:
            control_pin = MCP_PIN_CD4053B_S1;
            pin_state = true;  // Selects path 1 for S1
            break;

        // --- Switch S2 Paths ---
        case CD4053B_S2_PATH_B0_ACCESSORY_A:
            control_pin = MCP_PIN_CD4053B_S2;
            pin_state = false; // Selects path 0 for S2
            break;
        case CD4053B_S2_PATH_B1_ACCESSORY_B:
            control_pin = MCP_PIN_CD4053B_S2;
            pin_state = true;  // Selects path 1 for S2
            break;

        // --- Switch S3 Paths ---
        case CD4053B_S3_PATH_C0_JOYSTICK_AXIS2:
            control_pin = MCP_PIN_CD4053B_S3;
            pin_state = false; // Selects path 0 for S3
            break;
        case CD4053B_S3_PATH_C1_ACCESSORY_S2:
            control_pin = MCP_PIN_CD4053B_S3;
            pin_state = true;  // Selects path 1 for S3
            break;

        default:
            ESP_LOGE(TAG_CD4053B, "Invalid CD4053B named path specified: %d", path);
            return ESP_ERR_INVALID_ARG;
    }

    // ESP_LOGI(TAG_CD4053B, "Selecting path %d: MCP pin %d to state %s", path, control_pin, pin_state ? "HIGH" : "LOW");
    return mcp23008_wrapper_write_pin(mcp, control_pin, pin_state);
}
