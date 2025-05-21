#ifndef CD4053B_WRAPPER_H
#define CD4053B_WRAPPER_H

#include "mcp23008_wrapper.h" // For mcp23008_t and MCP23008_NamedPin
#include <stdbool.h>         // For bool type
#include "esp_err.h"          // For esp_err_t

#ifdef __cplusplus
extern "C" {
#endif

// Enum to identify each of the three SPDT switches within the CD4053B
// These correspond to the control select pins S1, S2, S3.
typedef enum {
    CD4053B_SWITCH_S1, // Controlled by MCP_PIN_CD4053B_S1
    CD4053B_SWITCH_S2, // Controlled by MCP_PIN_CD4053B_S2
    CD4053B_SWITCH_S3  // Controlled by MCP_PIN_CD4053B_S3
} CD4053B_ControllableSwitch;

/**
 * @brief Sets the state of one of the CD4053B's SPDT switches.
 *
 * Each switch is controlled by a corresponding select pin (S1, S2, or S3)
 * connected to the MCP23008.
 * - state = false (LOW): Connects the common terminal to the '0' path (e.g., Y0 for S1).
 * - state = true (HIGH): Connects the common terminal to the '1' path (e.g., Y1 for S1).
 *
 * The MCP23008 pins (MCP_PIN_CD4053B_S1, S2, S3) must have been previously
 * configured as outputs using mcp23008_wrapper_init() or mcp23008_wrapper_set_pin_direction().
 *
 * @param mcp Pointer to the initialized MCP23008 device structure.
 * @param sw The specific CD4053B switch to control (CD4053B_SWITCH_S1, _S2, or _S3).
 * @param state The desired state for the switch's select pin (false for LOW/channel 0, true for HIGH/channel 1).
 * @return ESP_OK on success, or an error code from the underlying MCP23008 driver on failure.
 */
esp_err_t cd4053b_set_switch_state(mcp23008_t *mcp, CD4053B_ControllableSwitch sw, bool state);

// IMPORTANT: Customize these path names for your specific hardware connections!
// Each CD4053B has three SPDT switches (controlled by S1, S2, S3 pins on MCP23008).
// Each switch can select one of two paths (typically labeled 0 and 1 on datasheets).
// LOW on the select pin (Sx) usually selects path 0. HIGH on Sx usually selects path 1.
typedef enum {
    // Paths for Switch 1 (controlled by MCP_PIN_CD4053B_S1)
    CD4053B_S1_PATH_A0_JOYSTICK_AXIS1, // Selects path 0 for S1 (MCP_PIN_CD4053B_S1 = LOW)
    CD4053B_S1_PATH_A1_BATT_SENSE, // Selects path 1 for S1 (MCP_PIN_CD4053B_S1 = HIGH)

    // Paths for Switch 2 (controlled by MCP_PIN_CD4053B_S2)
    // Replace "S2_PATH_0_DESCRIPTION" and "S2_PATH_1_DESCRIPTION"
    CD4053B_S2_PATH_B0_ACCESSORY_A, // Selects path 0 for S2 (MCP_PIN_CD4053B_S2 = LOW)
    CD4053B_S2_PATH_B1_ACCESSORY_B, // Selects path 1 for S2 (MCP_PIN_CD4053B_S2 = HIGH)

    // Paths for Switch 3 (controlled by MCP_PIN_CD4053B_S3)
    // Replace "S3_PATH_0_DESCRIPTION" and "S3_PATH_1_DESCRIPTION"
    CD4053B_S3_PATH_C0_JOYSTICK_AXIS2, // Selects path 0 for S3 (MCP_PIN_CD4053B_S3 = LOW)
    CD4053B_S3_PATH_C1_ACCESSORY_S2, // Selects path 1 for S3 (MCP_PIN_CD4053B_S3 = HIGH)

} CD4053B_NamedPath;

/**
 * @brief Selects a specific named path on the CD4053B multiplexer.
 *
 * This function configures the appropriate MCP23008 control pin (S1, S2, or S3)
 * to either LOW or HIGH to select the desired path on one of the CD4053B's
 * internal SPDT switches.
 *
 * The MCP23008 pins (MCP_PIN_CD4053B_S1, S2, S3) must have been previously
 * configured as outputs.
 *
 * @param mcp Pointer to the initialized MCP23008 device structure.
 * @param path The descriptive CD4053B_NamedPath to select.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t cd4053b_select_named_path(mcp23008_t *mcp, CD4053B_NamedPath path);

// --- Main board mux paths ---
typedef enum {
    MAIN_MUX_S1_PATH_A0_JOYSTICK_AXIS1, // S1, path 0 (MCP_PIN_CD4053B_S1 = LOW)
    MAIN_MUX_S1_PATH_A1_BATT_SENSE,     // S1, path 1 (MCP_PIN_CD4053B_S1 = HIGH)
    MAIN_MUX_S2_PATH_B0_ACCESSORY_A,    // S2, path 0 (MCP_PIN_CD4053B_S2 = LOW)
    MAIN_MUX_S2_PATH_B1_ACCESSORY_B,    // S2, path 1 (MCP_PIN_CD4053B_S2 = HIGH)
    MAIN_MUX_S3_PATH_C0_JOYSTICK_AXIS2, // S3, path 0 (MCP_PIN_CD4053B_S3 = LOW)
    MAIN_MUX_S3_PATH_C1_ACCESSORY_S2    // S3, path 1 (MCP_PIN_CD4053B_S3 = HIGH)
} MainMuxPath;

/**
 * @brief Selects a specific named path on the main CD4053B multiplexer.
 * @param mcp Pointer to the initialized MCP23008 device structure (main).
 * @param path The descriptive MainMuxPath to select.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t cd4053b_select_main_path(mcp23008_t *mcp, MainMuxPath path);

// --- Gun board mux paths ---
typedef enum {
    GUN_MUX_S1_PATH_A0_ACCESSORY_1, // S1, path 0 (MCP2_PIN_CD4053B_S1 = LOW)
    GUN_MUX_S1_PATH_A1_ACCESSORY_2, // S1, path 1 (MCP2_PIN_CD4053B_S1 = HIGH)
    GUN_MUX_S2_PATH_B0_IR_RX, // S2, path 0 (MCP2_PIN_CD4053B_S2 = LOW)
    GUN_MUX_S2_PATH_B1_NEOPIXEL, // S2, path 1 (MCP2_PIN_CD4053B_S2 = HIGH)
    GUN_MUX_S3_PATH_C0_IR_TX, // S3, path 0 (MCP2_PIN_CD4053B_S3 = LOW)
    GUN_MUX_S3_PATH_C1_ACCESSORY_S1  // S3, path 1 (MCP2_PIN_CD4053B_S3 = HIGH)
} GunMuxPath;

/**
 * @brief Selects a specific named path on the gun CD4053B multiplexer.
 * @param mcp Pointer to the initialized MCP23008 device structure (gun).
 * @param path The descriptive GunMuxPath to select.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t cd4053b_select_gun_path(mcp23008_t *mcp, GunMuxPath path);

#ifdef __cplusplus
}
#endif

#endif // CD4053B_WRAPPER_H
