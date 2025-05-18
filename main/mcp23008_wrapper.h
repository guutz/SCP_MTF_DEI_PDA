#ifndef MCP23008_WRAPPER_H
#define MCP23008_WRAPPER_H

#include "mcp23008.h" // For mcp23008_t and esp_err_t
#include <stdbool.h>   // For bool type
#include <stdint.h>    // For uint8_t

#ifdef __cplusplus
extern "C" {
#endif


#define MCP23008_DEFAULT_IODIR ( \
    (1 << MCP_PIN_JOYSTICK_ENTER)    | \
    (0 << MCP_PIN_BATT_SENSE_SWITCH)| \
    (1 << MCP_PIN_ACCESSORY_C)      | \
    (0 << MCP_PIN_CD4053B_S1)       | \
    (0 << MCP_PIN_CD4053B_S2)       | \
    (0 << MCP_PIN_CD4053B_S3)       | \
    (0 << MCP_PIN_ETH_LED_1)        | \
    (0 << MCP_PIN_ETH_LED_2)          \
)

#define MCP23008_DEFAULT_GPPU ( \
    (1 << MCP_PIN_JOYSTICK_ENTER)    | \
    (0 << MCP_PIN_BATT_SENSE_SWITCH) | \
    (1 << MCP_PIN_ACCESSORY_C)      | \
    (0 << MCP_PIN_CD4053B_S1)       | \
    (0 << MCP_PIN_CD4053B_S2)       | \
    (0 << MCP_PIN_CD4053B_S3)       | \
    (0 << MCP_PIN_ETH_LED_1)        | \
    (0 << MCP_PIN_ETH_LED_2)          \
)

// --- SECOND MCP23008 SUPPORT ---
#define MCP23008_2_DEFAULT_IODIR ( \
    (0 << MCP2_PIN_HAPTIC_MOTOR)    | \
    (1 << MCP2_PIN_GUN_TRIGGER)      | \
    (1 << MCP2_PIN_ACCESSORY_4)      | \
    (0 << MCP2_PIN_CD4053B_S3)       | \
    (0 << MCP2_PIN_CD4053B_S2)       | \
    (0 << MCP2_PIN_CD4053B_S1)       | \
    (0 << MCP2_PIN_ETH_LED_1)        | \
    (0 << MCP2_PIN_ETH_LED_2)          \
)
#define MCP23008_2_DEFAULT_GPPU ( \
    (0 << MCP2_PIN_HAPTIC_MOTOR)    | \
    (1 << MCP2_PIN_GUN_TRIGGER) | \
    (1 << MCP2_PIN_ACCESSORY_4)      | \
    (0 << MCP2_PIN_CD4053B_S1)       | \
    (0 << MCP2_PIN_CD4053B_S2)       | \
    (0 << MCP2_PIN_CD4053B_S3)       | \
    (0 << MCP2_PIN_ETH_LED_1)        | \
    (0 << MCP2_PIN_ETH_LED_2)          \
)

typedef enum {
    MCP_PIN_JOYSTICK_ENTER    = 0, // GPIO Pin 0
    MCP_PIN_BATT_SENSE_SWITCH  = 1, // GPIO Pin 1
    MCP_PIN_ACCESSORY_C  = 2, // GPIO Pin 2
    MCP_PIN_CD4053B_S1 = 3, // GPIO Pin 3
    MCP_PIN_CD4053B_S2 = 4, // GPIO Pin 4
    MCP_PIN_CD4053B_S3   = 5, // GPIO Pin 5
    MCP_PIN_ETH_LED_1   = 6, // GPIO Pin 6
    MCP_PIN_ETH_LED_2  = 7  // GPIO Pin 7
} MCP23008_NamedPin;

typedef enum {
    MCP2_PIN_HAPTIC_MOTOR    = 0, // GPIO Pin 0
    MCP2_PIN_GUN_TRIGGER  = 1, // GPIO Pin 1
    MCP2_PIN_ACCESSORY_4  = 2, // GPIO Pin 2
    MCP2_PIN_CD4053B_S3 = 3, // GPIO Pin 3
    MCP2_PIN_CD4053B_S2 = 4, // GPIO Pin 4
    MCP2_PIN_CD4053B_S1   = 5, // GPIO Pin 5
    MCP2_PIN_ETH_LED_1   = 6, // GPIO Pin 6
    MCP2_PIN_ETH_LED_2  = 7  // GPIO Pin 7
} MCP23008_2_NamedPin;

typedef enum {
    MCP_PIN_DIR_OUTPUT = 0,
    MCP_PIN_DIR_INPUT  = 1
} MCP23008_PinDirection;

/**
 * @brief Initializes the MCP23008 device and configures pin directions and pull-ups with custom defaults.
 *
 * This function first calls the underlying mcp23008_init(), then sets the IODIR (directions)
 * and GPPU (pull-ups) registers to the provided values.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param iodir The IODIR register value (direction defaults).
 * @param gppu The GPPU register value (pull-up defaults).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_init_with_defaults(mcp23008_t *mcp, uint8_t iodir, uint8_t gppu);

/**
 * @brief Initializes the MCP23008 device and configures pin directions and pull-ups (main defaults).
 *
 * This is a wrapper for mcp23008_wrapper_init_with_defaults using MCP23008_DEFAULT_IODIR and MCP23008_DEFAULT_GPPU.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_init(mcp23008_t *mcp);

/**
 * @brief Sets the direction (input or output) for a single named pin.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param pin The MCP23008_NamedPin to configure.
 * @param direction The desired direction (MCP_PIN_DIR_INPUT or MCP_PIN_DIR_OUTPUT).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_set_pin_direction(mcp23008_t *mcp, MCP23008_NamedPin pin, MCP23008_PinDirection direction);

/**
 * @brief Enables or disables the internal pull-up resistor for a single named pin.
 *        The pin should typically be configured as an input.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param pin The MCP23008_NamedPin to configure.
 * @param enabled True to enable the pull-up, false to disable it.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_set_pin_pullup(mcp23008_t *mcp, MCP23008_NamedPin pin, bool enabled);

/**
 * @brief Writes a digital value (high or low) to a single named output pin.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param pin The MCP23008_NamedPin to write to (must be configured as output).
 * @param value The boolean value to write (true for high, false for low).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_write_pin(mcp23008_t *mcp, MCP23008_NamedPin pin, bool value);

/**
 * @brief Reads the digital value of a single named input pin.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param pin The MCP23008_NamedPin to read from (must be configured as input).
 * @param value Pointer to a boolean where the pin's value will be stored (true for high, false for low).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_read_pin(mcp23008_t *mcp, MCP23008_NamedPin pin, bool *value);

/**
 * @brief Toggles the current state of a single named output pin.
 *
 * @param mcp Pointer to the MCP23008 device structure.
 * @param pin The MCP23008_NamedPin to toggle (must be configured as output).
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t mcp23008_wrapper_toggle_pin(mcp23008_t *mcp, MCP23008_NamedPin pin);

/**
 * @brief Checks if an MCP23008 device is present on the I2C bus.
 * @param mcp Pointer to the MCP23008 device struct.
 * @return ESP_OK if present, error code otherwise.
 */
esp_err_t mcp23008_check_present(mcp23008_t *mcp);

#ifdef __cplusplus
}
#endif

#endif // MCP23008_WRAPPER_H
