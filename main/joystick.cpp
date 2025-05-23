#include "joystick.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h" // For ADC calibration
#include "lvgl.h"        // Added for LVGL integration
#include "esp_wifi.h"    // Added for Wi-Fi functions

static const char *TAG_JOYSTICK = "joystick";

// ADC Calibration
static esp_adc_cal_characteristics_t adc_chars_axis1;
static esp_adc_cal_characteristics_t adc_chars_axis2;
static bool adc_calibrated = false;

// Voltage divider factor for battery sense
// Vin = Vout * (R1 + R2) / R2
// R1 = 100k, R2 = 33k
// Factor = (100 + 33) / 33 = 133 / 33 = 4.030303
#define BATTERY_VOLTAGE_DIVIDER_FACTOR 4.030303f

// LVGL Integration
static lv_indev_t *indev_keypad;
static lv_group_t *g;
static uint32_t last_key_lvgl = 0;
static lv_indev_state_t last_state_lvgl = LV_INDEV_STATE_REL;

// Deadzone and threshold for joystick analog to digital conversion
#define JOYSTICK_DEADZONE_LOW  1000 // Values below this are considered neutral
#define JOYSTICK_DEADZONE_HIGH 3000 // Values above this are considered neutral
#define JOYSTICK_THRESHOLD_LOW   1000  // Lower boundary for active range
#define JOYSTICK_THRESHOLD_HIGH  3000  // Upper boundary for active range


static bool joystick_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) { // Changed return type to bool
    mcp23008_t *current_mcp_dev = (mcp23008_t *)drv->user_data; // RETRIEVE from user_data
    static JoystickState_t joy_state;

    if (!current_mcp_dev) { 
        data->state = LV_INDEV_STATE_REL;
        data->key = 0; // Ensure key is cleared
        return false; // No data to read
    }

    esp_err_t ret = joystick_read_state(current_mcp_dev, &joy_state);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "LVGL: Failed to read joystick state: %s", esp_err_to_name(ret));
        data->state = LV_INDEV_STATE_REL; // Report release on error
        data->key = 0; // Ensure key is cleared
        return false; // No data to read
    }

    uint32_t act_key = 0;
    lv_indev_state_t act_state = LV_INDEV_STATE_REL;

    // Button press
    if (joy_state.button_pressed) {
        act_key = LV_KEY_ENTER;
        act_state = LV_INDEV_STATE_PR;
    }
    // Joystick movements
    else if (joy_state.x < JOYSTICK_THRESHOLD_LOW) { // Up
        act_key = LV_KEY_UP;
        act_state = LV_INDEV_STATE_PR;
    } else if (joy_state.x > JOYSTICK_THRESHOLD_HIGH) { // Down
        act_key = LV_KEY_DOWN;
        act_state = LV_INDEV_STATE_PR;
    } else if (joy_state.y < JOYSTICK_THRESHOLD_LOW) { // Left
        act_key = LV_KEY_LEFT;
        act_state = LV_INDEV_STATE_PR;
    } else if (joy_state.y > JOYSTICK_THRESHOLD_HIGH) { // Right
        act_key = LV_KEY_RIGHT;
        act_state = LV_INDEV_STATE_PR;
    }

    // Logic for press and release
    if (act_state == LV_INDEV_STATE_PR) {
        // A key is currently pressed
        if (last_state_lvgl == LV_INDEV_STATE_REL || last_key_lvgl != act_key) {
            // It's a new press or a different key is pressed
            data->key = act_key;
            data->state = LV_INDEV_STATE_PR;
            last_key_lvgl = act_key;
            last_state_lvgl = LV_INDEV_STATE_PR;
        } else {
            // Key is still held, report pressed state but LVGL handles repeat internally for keypad
            data->key = last_key_lvgl; // Keep reporting the currently pressed key
            data->state = LV_INDEV_STATE_PR;
            // last_state_lvgl remains LV_INDEV_STATE_PR
        }
    } else {
        // No key is currently physically pressed
        if (last_state_lvgl == LV_INDEV_STATE_PR) {
            // A key was pressed before, now it's released
            data->key = last_key_lvgl; // Report release of the last pressed key
            data->state = LV_INDEV_STATE_REL;
            // last_key_lvgl remains the same until a new press
            last_state_lvgl = LV_INDEV_STATE_REL;
        } else {
            // No key pressed now, and no key was pressed before
            data->key = 0; // No key active
            data->state = LV_INDEV_STATE_REL;
            last_state_lvgl = LV_INDEV_STATE_REL;
        }
    }
    return false; // Indicate that there is no more data to read in this call
}

void lvgl_joystick_input_init(mcp23008_t *mcp_dev) {
    if (!mcp_dev) {
        ESP_LOGE(TAG_JOYSTICK, "MCP23008 device pointer is NULL for LVGL joystick init.");
        return;
    }

    static lv_indev_drv_t indev_drv_joystick; // Make it static or global
    lv_indev_drv_init(&indev_drv_joystick);
    indev_drv_joystick.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_joystick.read_cb = joystick_read_cb;
    indev_drv_joystick.user_data = mcp_dev; // ASSIGN mcp_dev to user_data
    indev_keypad = lv_indev_drv_register(&indev_drv_joystick);

    if (!indev_keypad) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to register LVGL joystick input device.");
        return;
    }
     ESP_LOGI(TAG_JOYSTICK, "LVGL Joystick input device registered.");

    g = lv_group_create();
    if (!g) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to create LVGL group for joystick.");
        // Consider how to handle this error, perhaps by not setting the group.
        return;
    }
    lv_indev_set_group(indev_keypad, g);
    ESP_LOGI(TAG_JOYSTICK, "LVGL Joystick group created and assigned.");
}

lv_group_t *lvgl_joystick_get_group(void) {
    return g;
}


esp_err_t joystick_init(void) {
    esp_err_t ret;

    // Configure ADC1 for Joystick Axis 1 / Battery Sense (GPIO32 - ADC1_CHANNEL_4)
    ret = adc1_config_width(ADC_WIDTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to configure ADC1 width: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = adc1_config_channel_atten(JOYSTICK_AXIS1_ADC_CHANNEL, ADC_ATTENUATION);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to configure ADC1 channel %d attenuation: %s", JOYSTICK_AXIS1_ADC_CHANNEL, esp_err_to_name(ret));
        return ret;
    }

    // If Y-axis is on ADC1, configure its channel attenuation here as well.
    ret = adc1_config_channel_atten(JOYSTICK_AXIS2_ADC_CHANNEL, ADC_ATTENUATION);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to configure ADC1 channel %d for Y-axis attenuation: %s", JOYSTICK_AXIS2_ADC_CHANNEL, esp_err_to_name(ret));
        return ret;
    }


    // ADC Calibration (optional, but recommended for accuracy)
    // This attempts to characterize the ADC based on Vref.
    // You might need to burn eFuse Vref value for better results.
    esp_err_t cal_ret_axis1 = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF);
    if (cal_ret_axis1 == ESP_OK) {
        ESP_LOGI(TAG_JOYSTICK, "ADC1 eFuse Vref is available");
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTENUATION, ADC_WIDTH, 0, &adc_chars_axis1); // 0 for default Vref
        adc_calibrated = true;
    } else if (cal_ret_axis1 == ESP_ERR_NOT_SUPPORTED || cal_ret_axis1 == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAG_JOYSTICK, "ADC1 eFuse Vref not supported or invalid version, using default Vref for characterization");
         // Fallback: Use default VREF if eFuse VREF is not available
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTENUATION, ADC_WIDTH, ESP_ADC_CAL_VAL_DEFAULT_VREF, &adc_chars_axis1);
        adc_calibrated = true; // Still mark as calibrated for voltage conversion
    } else {
        ESP_LOGE(TAG_JOYSTICK, "ADC1 eFuse Vref calibration failed: %s", esp_err_to_name(cal_ret_axis1));
        adc_calibrated = false;
    }
    
    esp_err_t cal_ret_axis2 = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF);
    if (cal_ret_axis2 == ESP_OK) {

        ESP_LOGI(TAG_JOYSTICK, "ADC1 eFuse Vref is available (for Y-axis)");
        // If Y-axis is on ADC1, this characterization might be redundant if JOYSTICK_AXIS1_ADC_UNIT is also ADC_UNIT_1.
        // We assume adc_chars_axis2 is intended for the second axis regardless of unit.
        esp_adc_cal_characterize(JOYSTICK_AXIS2_ADC_UNIT, ADC_ATTENUATION, ADC_WIDTH, 0, &adc_chars_axis2); 

         // adc_calibrated = true; // adc_calibrated is shared, ensure this logic is sound if one unit calibrates and other doesn't
    } else if (cal_ret_axis2 == ESP_ERR_NOT_SUPPORTED || cal_ret_axis2 == ESP_ERR_INVALID_VERSION) {

        ESP_LOGW(TAG_JOYSTICK, "ADC1 eFuse Vref not supported or invalid version for Y-axis, using default Vref for characterization");
        esp_adc_cal_characterize(JOYSTICK_AXIS2_ADC_UNIT, ADC_ATTENUATION, ADC_WIDTH, ESP_ADC_CAL_VAL_DEFAULT_VREF, &adc_chars_axis2);

    } else {
        ESP_LOGE(TAG_JOYSTICK, "ADC2 eFuse Vref calibration failed: %s", esp_err_to_name(cal_ret_axis2));
        // adc_calibrated = false; // If ADC1 calibrated but ADC2 didn't, how to handle? For now, joystick_read might return raw if this specific unit failed.
    }


    ESP_LOGI(TAG_JOYSTICK, "Joystick and ADC initialized.");
    return ESP_OK;
}

esp_err_t joystick_read_state(mcp23008_t *mcp, JoystickState_t *state) {
    if (!state || !mcp) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    int adc_raw;

    // Read Joystick X (Axis 1)
    ret = cd4053b_select_named_path(mcp, CD4053B_S1_PATH_A0_JOYSTICK_AXIS1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to select CD4053B path for Joystick X: %s", esp_err_to_name(ret));
        return ret;
    }
    // Brief delay to allow multiplexer to settle if necessary, and for ADC input capacitance
    vTaskDelay(pdMS_TO_TICKS(1)); 

    adc_raw = adc1_get_raw(JOYSTICK_AXIS1_ADC_CHANNEL);
    if (adc_raw == -1) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to read ADC1 for Joystick X");
        return ESP_FAIL; // Or a more specific ADC error
    }
    state->x = adc_raw;

    adc_raw = adc1_get_raw(JOYSTICK_AXIS2_ADC_CHANNEL);
    if (adc_raw == -1) {
        ESP_LOGW(TAG_JOYSTICK, "Failed to read ADC1 for Joystick Y. Setting Y to 0.");
        state->y = 0; // Set Y to 0 on ADC1 read failure
        // Do not return error, continue to read button
    } else {
        state->y = adc_raw;
    }

    // Read Joystick Button
    bool button_val;
    ret = mcp23008_wrapper_read_pin(mcp, MCP_PIN_JOYSTICK_ENTER, &button_val);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to read joystick button: %s", esp_err_to_name(ret));
        return ret;
    }
    state->button_pressed = !button_val; // Assuming button is active LOW (pressed = 0)

    // ESP_LOGD(TAG_JOYSTICK, "Joystick state: X=%d, Y=%d, Btn=%d", state->x, state->y, state->button_pressed);
    return ESP_OK;
}

esp_err_t battery_read_voltage(mcp23008_t *mcp, float *voltage) {
    if (!voltage || !mcp) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    int adc_raw;

    // Enable battery sense circuit by setting MCP_PIN_BATT_SENSE_SWITCH HIGH
    ret = mcp23008_wrapper_write_pin(mcp, MCP_PIN_BATT_SENSE_SWITCH, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to enable battery sense switch: %s", esp_err_to_name(ret));
        return ret;
    }

    // Select Battery Sense Path on CD4053B
    ret = cd4053b_select_named_path(mcp, CD4053B_S1_PATH_A1_BATT_SENSE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to select CD4053B path for Battery Sense: %s", esp_err_to_name(ret));
        // Attempt to disable battery sense switch even if path selection failed
        mcp23008_wrapper_write_pin(mcp, MCP_PIN_BATT_SENSE_SWITCH, false);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Allow multiplexer to settle

    adc_raw = adc1_get_raw(BATTERY_SENSE_ADC_CHANNEL);

    // Disable battery sense circuit by setting MCP_PIN_BATT_SENSE_SWITCH LOW
    // Do this regardless of ADC read success to ensure it's turned off.
    esp_err_t disable_ret = mcp23008_wrapper_write_pin(mcp, MCP_PIN_BATT_SENSE_SWITCH, false);
    if (disable_ret != ESP_OK) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to disable battery sense switch: %s", esp_err_to_name(disable_ret));
        // Potentially overwrite original error if this one is more critical, or log both
        // For now, we prioritize returning the ADC read or path selection error if it occurred.
    }

    if (adc_raw == -1) {
        ESP_LOGE(TAG_JOYSTICK, "Failed to read ADC1 for Battery Sense");
        return ESP_FAIL; // Or a more specific ADC error
    }

    if (adc_calibrated) {
        uint32_t millivolts = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars_axis1);
        *voltage = (float)millivolts / 1000.0f * BATTERY_VOLTAGE_DIVIDER_FACTOR;
        // ESP_LOGD(TAG_JOYSTICK, "Battery raw: %d, mV_adc: %d, V_batt: %.2fV", adc_raw, millivolts, *voltage);
    } else {
        ESP_LOGW(TAG_JOYSTICK, "ADC not calibrated, returning raw scaled value for battery voltage. Accuracy may be low.");
        // Basic conversion if no calibration data (less accurate)
        // Assuming Vref is around 3.3V for ADC_ATTEN_DB_11
        // This is a rough estimate. Calibration is much better.
        float v_adc_approx = (adc_raw / 4095.0f) * 3.3f; // Approximation
        *voltage = v_adc_approx * BATTERY_VOLTAGE_DIVIDER_FACTOR;
    }
    return ESP_OK;
}
