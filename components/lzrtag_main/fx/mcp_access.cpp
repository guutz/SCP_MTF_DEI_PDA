#include "mcp_access.h"
#include "mcp23008_wrapper.h"

static mcp23008_t *s_lzrtag_mcp = nullptr;

void lzrtag_set_mcp23008_instance(mcp23008_t *mcp) { s_lzrtag_mcp = mcp; }
mcp23008_t *lzrtag_get_mcp23008_instance(void) { return s_lzrtag_mcp; }

void lzrtag_set_vibrate_motor(bool on) {
    mcp23008_t *mcp = lzrtag_get_mcp23008_instance();
    if (mcp)
        mcp23008_wrapper_write_pin(mcp, (MCP23008_NamedPin)MCP2_PIN_HAPTIC_MOTOR, on);
}

bool lzrtag_get_trigger(void) {
    mcp23008_t *mcp = lzrtag_get_mcp23008_instance();
    if (!mcp) return false;
    bool value = false;
    mcp23008_wrapper_read_pin(mcp, (MCP23008_NamedPin)MCP2_PIN_GUN_TRIGGER, &value);
    return value;
}
