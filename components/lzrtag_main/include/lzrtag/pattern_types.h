#ifndef LZRTAG_PATTERN_TYPES_H
#define LZRTAG_PATTERN_TYPES_H

#include <cstdint>

namespace LZR {
    enum pattern_mode_t : uint16_t {
        OFF,
        BATTERY_LEVEL,
        CHARGE,
        CONNECTING,
        PLAYER_DECIDED,
        IDLE,
        TEAM_SELECT,
        DEAD,
        ACTIVE,
        OTA,
        PATTERN_MODE_MAX, // Placeholder
    };
}

#endif // LZRTAG_PATTERN_TYPES_H
