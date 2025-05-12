# MCP23008

This is a slightly modified version of NSBum's MCP23008 library. Now it utilizes a thread-safe I2C Manager by ropg (https://github.com/ropg/i2c_manager).

To be able to use it, you first need to have the i2c_manager repository in the components directory. I2C ports then have to be enabled and setup individually via menuconfig. You can find more information here: https://github.com/ropg/i2c_manager#simplest-usage

---

ESP32 component for the MCP23008 I2C GPIO expander, for use with the [ESP-IDF](https://github.com/espressif/esp-idf) build framework. 

The ESP32 is a versatile little IoT SoC device that can handle a number of microcontroller tasks for automation and other projects. One of the problems with the ESP32 is the shortage of GPIO pins. Many of the pins on the device had dedicated functions. Some projects require a larger number of open pins. Therefore, this device and similar GPIO expanders can be used to fill the void.

The use of this component is straightfoward. If the I2C bus in use has not already been initialized, you'll need to take care of its initialization in advance of calling `mcp23008_init`.
