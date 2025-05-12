#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "mcp23008_wrapper.h" // For mcp23008_t and MCP23008_NamedPin
#include "cd4053b_wrapper.h" // For cd4053b_select_named_path and CD4053B_NamedPath

#ifdef __cplusplus
extern "C" {
#endif

// ADC Configuration
#define ADC_ATTENUATION ADC_ATTEN_DB_11
#define ADC_WIDTH       ADC_WIDTH_BIT_12

// Joystick ADC Channels (assuming GPIOs are mapped to these ADC channels)
// GPIO32 is ADC1_CHANNEL_4
// GPIO25 is ADC2_CHANNEL_8
#define JOYSTICK_AXIS1_ADC_UNIT  ADC_UNIT_1
#define JOYSTICK_AXIS1_ADC_CHANNEL ADC1_CHANNEL_4 // Connected via CD4053B S1 (Y0) to GPIO32
#define JOYSTICK_AXIS2_ADC_UNIT  ADC_UNIT_2
#define JOYSTICK_AXIS2_ADC_CHANNEL ADC2_CHANNEL_8 // Connected via CD4053B S3 (Y2) to GPIO25

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

#ifdef __cplusplus
}
#endif

#endif // JOYSTICK_H
