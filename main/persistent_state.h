#pragma once
#include <string>
#include <vector>

struct PlayerInfo {
    std::string device_id;
    std::vector<std::string> read_pages;
};

// Player info and persistent state
namespace PersistentState {
    // Player info
    bool player_info_exists_on_sd();
    bool read_player_info_from_sd(PlayerInfo &info);
    bool write_default_player_info_to_sd();
    // Future: add functions for menu read-tracking, etc.
}
