#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "mcp23008_wrapper.h" // For mcp23008_t and MCP23008_NamedPin
#include "cd4053b_wrapper.h" // For cd4053b_select_named_path and CD4053B_NamedPath
#include "lvgl.h" // Added for LVGL integration

#ifdef __cplusplus
extern "C" {
#endif

// ADC Configuration
#define ADC_ATTENUATION ADC_ATTEN_DB_11
#define ADC_WIDTH       ADC_WIDTH_BIT_12

// Define this macro in your build system or here to use ADC1 for Joystick Y-axis
#define USE_ADC1_FOR_JOYSTICK_Y

// Joystick ADC Channels
// GPIO32 is ADC1_CHANNEL_4
#define JOYSTICK_AXIS1_ADC_UNIT    ADC_UNIT_1
#define JOYSTICK_AXIS1_ADC_CHANNEL ADC1_CHANNEL_4 // Connected via CD4053B S1 (Y0) to GPIO32

#ifdef USE_ADC1_FOR_JOYSTICK_Y
// GPIO34 is ADC1_CHANNEL_6
#define JOYSTICK_AXIS2_ADC_UNIT    ADC_UNIT_1
#define JOYSTICK_AXIS2_ADC_CHANNEL ADC1_CHANNEL_6 // Connected via CD4053B S3 (Y2) to GPIO34
#else
// GPIO25 is ADC2_CHANNEL_8
#define JOYSTICK_AXIS2_ADC_UNIT    ADC_UNIT_2
#define JOYSTICK_AXIS2_ADC_CHANNEL ADC2_CHANNEL_8 // Connected via CD4053B S3 (Y2) to GPIO25
#endif

// Battery Sense ADC Channel (shares S1 common pin with Joystick Axis 1)
#define BATTERY_SENSE_ADC_UNIT   ADC_UNIT_1
#define BATTERY_SENSE_ADC_CHANNEL ADC1_CHANNEL_4 // Connected via CD4053B S1 (Y0) to GPIO32

typedef struct {
    int x;
    int y;
    bool button_pressed;
} JoystickState_t;

/**
 * @brief Initializes ADC for joystick and battery measurements.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t joystick_init(void);

/**
 * @brief Reads the current state of the joystick (X, Y axes and button).
 *
 * @param mcp Pointer to the MCP23008 device structure for reading the button.
 * @param[out] state Pointer to JoystickState_t structure to store the joystick state.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t joystick_read_state(mcp23008_t *mcp, JoystickState_t *state);

/**
 * @brief Reads the battery voltage.
 *
 * @param mcp Pointer to the MCP23008 device structure (if needed for controlling battery sense switch, though current plan uses CD4053B).
 * @param[out] voltage Pointer to a float where the battery voltage will be stored.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t battery_read_voltage(mcp23008_t *mcp, float *voltage);

/**
 * @brief Initializes the joystick input for LVGL.
 *
 * @param mcp_dev Pointer to the initialized MCP23008 device structure.
 *                This is needed if joystick_read_state requires it.
 */
void lvgl_joystick_input_init(mcp23008_t *mcp_dev);

/**
 * @brief Gets the LVGL input group associated with the joystick.
 *
 * @return Pointer to the LVGL group.
 */
lv_group_t *lvgl_joystick_get_group(void);

#ifndef USE_ADC1_FOR_JOYSTICK_Y
/**
 * @brief Enables/disables Wi-Fi to prioritize ADC2 for dual-axis joystick readings.
 *
 * This function should be called when ADC2 is used for one of the joystick axes
 * and Wi-Fi interference is a concern.
 *
 * @param prioritize_dual_axis True to stop Wi-Fi and prioritize ADC2, false to restore Wi-Fi.
 */
void set_joystick_dual_axis_priority(bool prioritize_dual_axis);
#endif // USE_ADC1_FOR_JOYSTICK_Y

#ifdef __cplusplus
}
#endif

#endif // JOYSTICK_H
