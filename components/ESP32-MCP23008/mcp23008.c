/**
 *
 * ESP32 driver for the MCP23008 I2C GPIO expander
 * 
 * For use with the ESP-IDF build framework; the I2C communication
 * is handled by the i2c_manager component.
 *
 * @author Alan K. Duncan
 * 
 * @author Jakub Prikner <jakub.prikner@gmail.com>
 * https://www.prikner.net
 *
 */

// ====================================================================

#include "mcp23008.h"

// registers
#define MCP23008_IODIR      0x00
#define MCP23008_IPOL       0x01
#define MCP23008_GPINTEN    0x02
#define MCP23008_DEFVAL     0x03
#define MCP23008_INTCON     0x04
#define MCP23008_IOCON      0x05
#define MCP23008_GPPU       0x06
#define MCP23008_INTF       0x07
#define MCP23008_INTCAP     0x08
#define MCP23008_GPIO       0x09
#define MCP23008_OLAT       0x0A

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define ACK_CHECK_EN  0x1      /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0      /*!< I2C master will not check ack from slave */
#define ACK_VAL       0x0      /*!< I2C ack value */
#define NACK_VAL      0x1      /*!< I2C nack value */

static const char *TAG = "MCP23008" ;

// ====================================================================

static esp_err_t mcp23008_read_reg ( mcp23008_t *mcp, uint8_t reg, uint8_t *d )
{
    CHECK_ARG (mcp) ;
    esp_err_t ret = i2c_manager_read ( mcp -> port, mcp -> address, reg, &d, 1 ) ;
    
	if ( ret != ESP_OK ) 
	{
        ESP_LOGE ( TAG, "Error reading register %d", reg ) ;
        return ESP_FAIL ;
    }
    vTaskDelay ( 30 / portTICK_RATE_MS ) ;

    return ESP_OK ;
}

// --------------------------------------------------------------------

static esp_err_t mcp23008_write_reg ( mcp23008_t *mcp, uint8_t reg, uint8_t d )
{
    CHECK_ARG (mcp) ;
	esp_err_t ret = i2c_manager_write ( mcp -> port, mcp -> address, reg, &d, 1 ) ;

	if ( ret != ESP_OK ) 
	{
        ESP_LOGE ( TAG, "Error writing register %d", reg ) ;
        return ESP_FAIL ;
    }
    return ESP_OK ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_init ( mcp23008_t *mcp )
{
    ESP_LOGI ( TAG, "Init --- I2C port: %d, address: 0x%x", mcp -> port, mcp -> address ) ;
    return mcp23008_write_reg ( mcp, MCP23008_IODIR, 0x00 ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_port ( mcp23008_t *mcp, uint8_t *d )
{
    esp_err_t ret = mcp23008_read_reg ( mcp, MCP23008_GPIO, d ) ;
    if ( ret == ESP_OK ) 
    {
        mcp -> current = *d ;
        return ESP_OK ;
    }
    return ret ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_write_port ( mcp23008_t *mcp, uint8_t d )
{
    esp_err_t ret = mcp23008_write_reg ( mcp, MCP23008_GPIO, d ) ;
    if ( ret == ESP_OK ) 
    {
        mcp -> current = d ;
        return ESP_OK ;
    }
    return ret ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_port_direction ( mcp23008_t *mcp, uint8_t dirs )
{
    return mcp23008_write_reg ( mcp, MCP23008_IODIR, dirs ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_get_port_direction ( mcp23008_t *mcp, uint8_t *dirs )
{
    return mcp23008_read_reg ( mcp, MCP23008_IODIR, dirs ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_interrupt_enable ( mcp23008_t *mcp, uint8_t intr )
{
    return mcp23008_write_reg ( mcp, MCP23008_GPINTEN, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_interrupt_enable ( mcp23008_t *mcp, uint8_t *intr )
{
    return mcp23008_read_reg ( mcp, MCP23008_GPINTEN, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_default_value ( mcp23008_t *mcp, uint8_t defval )
{
    return mcp23008_write_reg ( mcp, MCP23008_DEFVAL, defval ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_default_value ( mcp23008_t *mcp, uint8_t *defval )
{
    return mcp23008_read_reg ( mcp, MCP23008_DEFVAL, defval ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_interrupt_control ( mcp23008_t *mcp, uint8_t intr )
{
    return mcp23008_write_reg ( mcp, MCP23008_INTCON, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_interrupt_control ( mcp23008_t *mcp, uint8_t *intr )
{
    return mcp23008_read_reg ( mcp, MCP23008_INTCON, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_interrupt_reg ( mcp23008_t *mcp, uint8_t *intr )
{
    return mcp23008_read_reg ( mcp, MCP23008_INTF, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_interrupt_capture ( mcp23008_t *mcp, uint8_t *intr )
{
    return mcp23008_read_reg ( mcp, MCP23008_INTCAP, intr ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_output_latch ( mcp23008_t *mcp, uint8_t olat )
{
    return mcp23008_write_reg ( mcp, MCP23008_OLAT, olat ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_output_latch ( mcp23008_t *mcp, uint8_t *olat )
{
    return mcp23008_read_reg ( mcp, MCP23008_OLAT, olat ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_set_pullups ( mcp23008_t *mcp, uint8_t pu )
{
    return mcp23008_write_reg ( mcp, MCP23008_GPPU, pu ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_read_pullups ( mcp23008_t *mcp, uint8_t *pu )
{
    return mcp23008_read_reg ( mcp, MCP23008_GPPU, pu ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_port_set_bit ( mcp23008_t *mcp, uint8_t b )
{
    mcp -> current |= ( 1 << b ) ;
    return mcp23008_write_port ( mcp, mcp -> current ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_port_clear_bit ( mcp23008_t *mcp, uint8_t b )
{
    mcp -> current &= ~( 1 << b ) ;
    return mcp23008_write_port ( mcp, mcp -> current ) ;
}

// --------------------------------------------------------------------

esp_err_t mcp23008_toggle_bit ( mcp23008_t *mcp, uint8_t b )
{
    mcp -> current ^= ( 1 << b ) ;
    return mcp23008_write_port ( mcp, mcp -> current ) ; 
}


