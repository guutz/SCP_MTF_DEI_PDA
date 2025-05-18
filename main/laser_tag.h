#ifndef LASER_TAG_H
#define LASER_TAG_H

#include "mcp23008_wrapper.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LZRTag mode entry/exit
bool laser_tag_mode_enter(void);
void laser_tag_mode_exit(void);

// Extern for the gun MCP23008 device
extern mcp23008_t gun_gpio_extender;

// Call this to check/init the gun MCP23008 and mux switch
void gun_mux_switch_init(void);

#ifdef __cplusplus
}
#endif

#endif // LASER_TAG_H
