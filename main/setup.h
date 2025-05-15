#ifndef SETUP_H
#define SETUP_H

#include "mcp23008.h"
#include "lvgl.h"

#define LV_TICK_PERIOD_MS 1

#ifdef __cplusplus
extern "C" {
#endif

void display_touch_init(void);
void time_overlay_init(void);
void lvgl_full_init(void);
void lvgl_tick_timer_setup(void);

#ifdef __cplusplus
}
#endif

#endif // SETUP_H
