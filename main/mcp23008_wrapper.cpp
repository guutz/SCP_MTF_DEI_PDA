#include "mcp23008_wrapper.h"
#include "mcp23008.h" // For underlying driver functions
#include "esp_log.h"   // For ESP_LOGx macros
#include "esp_err.h"

static const char *TAG_WRAPPER = "mcp23008_wrapper";

esp_err_t mcp23008_wrapper_init(mcp23008_t *mcp) {
    if (!mcp) {
        ESP_LOGE(TAG_WRAPPER, "MCP23008 device pointer is NULL in init");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = mcp23008_init(mcp); // Initialize the chip
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to initialize MCP23008 (0x%X)", ret);
        return ret;
    }

    // Set initial pin directions using the defaults from the header
    ret = mcp23008_set_port_direction(mcp, MCP23008_DEFAULT_IODIR);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to set default IODIR (0x%X)", ret);
        return ret;
    }

    // Set initial pull-ups using the defaults from the header
    ret = mcp23008_set_pullups(mcp, MCP23008_DEFAULT_GPPU);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to set default GPPU (0x%X)", ret);
        return ret;
    }

    ESP_LOGI(TAG_WRAPPER, "MCP23008 wrapper initialized with default settings. IODIR: 0x%02X, GPPU: 0x%02X", MCP23008_DEFAULT_IODIR, MCP23008_DEFAULT_GPPU);
    return ESP_OK;
}

esp_err_t mcp23008_wrapper_set_pin_direction(mcp23008_t *mcp, MCP23008_NamedPin pin, MCP23008_PinDirection direction) {
    if (!mcp) return ESP_ERR_INVALID_ARG;
    if (pin > 7) return ESP_ERR_INVALID_ARG; // MCP23008 has 8 pins (0-7)

    uint8_t current_iodir;
    esp_err_t ret = mcp23008_get_port_direction(mcp, &current_iodir);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to read IODIR (0x%X)", ret);
        return ret;
    }

    if (direction == MCP_PIN_DIR_INPUT) {
        current_iodir |= (1 << pin);
    } else {
        current_iodir &= ~(1 << pin);
    }

    ret = mcp23008_set_port_direction(mcp, current_iodir);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to write IODIR (0x%X)", ret);
    }
    return ret;
}

esp_err_t mcp23008_wrapper_set_pin_pullup(mcp23008_t *mcp, MCP23008_NamedPin pin, bool enabled) {
    if (!mcp) return ESP_ERR_INVALID_ARG;
    if (pin > 7) return ESP_ERR_INVALID_ARG;

    uint8_t current_gppu;
    esp_err_t ret = mcp23008_read_pullups(mcp, &current_gppu);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to read GPPU (0x%X)", ret);
        return ret;
    }

    if (enabled) {
        current_gppu |= (1 << pin);
    } else {
        current_gppu &= ~(1 << pin);
    }

    ret = mcp23008_set_pullups(mcp, current_gppu);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to write GPPU (0x%X)", ret);
    }
    return ret;
}

esp_err_t mcp23008_wrapper_write_pin(mcp23008_t *mcp, MCP23008_NamedPin pin, bool value) {
    if (!mcp) return ESP_ERR_INVALID_ARG;
    if (pin > 7) return ESP_ERR_INVALID_ARG;

    // The mcp23008_port_set_bit and mcp23008_port_clear_bit functions in your
    // driver already read the current port state, modify it, and write it back.
    // They also update mcp->current.
    esp_err_t ret;
    if (value) {
        ret = mcp23008_port_set_bit(mcp, (uint8_t)pin);
    } else {
        ret = mcp23008_port_clear_bit(mcp, (uint8_t)pin);
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to write to pin %d (0x%X)", pin, ret);
    }
    return ret;
}

esp_err_t mcp23008_wrapper_read_pin(mcp23008_t *mcp, MCP23008_NamedPin pin, bool *value) {
    if (!mcp || !value) return ESP_ERR_INVALID_ARG;
    if (pin > 7) return ESP_ERR_INVALID_ARG;

    uint8_t port_values;
    esp_err_t ret = mcp23008_read_port(mcp, &port_values);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to read GPIO port (0x%X)", ret);
        *value = false; // Default to false on error
        return ret;
    }

    *value = (port_values & (1 << pin)) ? true : false;
    return ESP_OK;
}

esp_err_t mcp23008_wrapper_toggle_pin(mcp23008_t *mcp, MCP23008_NamedPin pin) {
    if (!mcp) return ESP_ERR_INVALID_ARG;
    if (pin > 7) return ESP_ERR_INVALID_ARG;

    // The mcp23008_toggle_bit function in your driver already reads the current
    // port state, modifies it, and writes it back. It also updates mcp->current.
    esp_err_t ret = mcp23008_toggle_bit(mcp, (uint8_t)pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WRAPPER, "Failed to toggle pin %d (0x%X)", pin, ret);
    }
    return ret;
}
