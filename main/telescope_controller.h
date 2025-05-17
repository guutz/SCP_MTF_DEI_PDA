#ifndef TELESCOPE_CONTROLLER_H
#define TELESCOPE_CONTROLLER_H

#include "driver/uart.h"
#include "joystick.h" // For JoystickState_t

#define TELESCOPE_UART_NUM UART_NUM_1
#define TELESCOPE_TXD_PIN GPIO_NUM_17
#define TELESCOPE_RXD_PIN GPIO_NUM_16
#define TELESCOPE_UART_BAUD_RATE 115200 // Match baud rate from Python script

// Based on your Python script's speed values
#define MAX_AZ_SPEED 500
#define MAX_EL_SPEED 1100

#define JOYSTICK_MAX_VALUE 4095 // Assuming 12-bit ADC resolution
#define JOYSTICK_DEADZONE 1000 // Adjust based on your joystick's neutral position

class TelescopeController {
public:
    TelescopeController();
    virtual ~TelescopeController();

    esp_err_t init();
    virtual void process_joystick_input(const JoystickState_t* joystick_state);
    virtual esp_err_t send_command(const char* cmd, bool add_cr = true, int send_delay_ms = 8, int recv_delay_ms = 8);
    virtual esp_err_t read_response(char* buffer, size_t buffer_len, int timeout_ms = 500);

private:
    void startup_sequence();
    void brakes_off();
    // Add other necessary private methods based on python script, e.g., for specific commands
    // esp_err_t send_raw_command(const char* cmd_str);
    // int map_joystick_to_speed(int joystick_val, int max_speed);


    bool initialized;
    bool brakes_are_off;
    // Add any other state variables needed, e.g., current_az_speed, current_el_speed
};

#endif // TELESCOPE_CONTROLLER_H
