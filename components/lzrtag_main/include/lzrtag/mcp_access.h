#pragma once
#include "mcp23008_wrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
void lzrtag_set_mcp23008_instance(mcp23008_t *mcp);
mcp23008_t *lzrtag_get_mcp23008_instance(void);
void lzrtag_set_vibrate_motor(bool on);
bool lzrtag_get_trigger(void);
#ifdef __cplusplus
}
#endif
