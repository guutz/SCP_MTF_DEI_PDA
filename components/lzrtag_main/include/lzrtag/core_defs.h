#ifndef LZRTG_CORE_DEFS_H
#define LZRTG_CORE_DEFS_H

// Core definitions shared between main application and lzrtag components

enum LZRTag_CORE_WEAPON_STATUS {
    LZRTag_WPN_STAT_INITIALIZING,
    LZRTag_WPN_STAT_DISCHARGED,
    LZRTag_WPN_STAT_CHARGING,
    LZRTag_WPN_STAT_NOMINAL,
};

// Forward declarations for common LZRTag classes
namespace LZR {
    struct ColorSet;
    class Player;
}
namespace LZRTag {
    namespace Weapon {
        class BaseWeapon;
        class Handler;
    }
}
namespace Housekeeping {
    class BatteryManager;
}
namespace Xasin {
    namespace Communication {
        class CommHandler;
    }
}

#endif // LZRTG_CORE_DEFS_H