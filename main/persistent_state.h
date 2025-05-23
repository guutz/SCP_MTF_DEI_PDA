#pragma once
#include <string>
#include <vector>
#include <set> // Added for unique collections

struct PlayerInfo {
    std::string device_id;
    std::vector<std::string> read_pages; // Kept as vector if order matters or duplicates allowed
                                         // Consider std::set<std::string> if unique and order doesn't matter
};

// Player info and persistent state
namespace PersistentState {
    // --- Structure to hold all persistent menu-related states ---
    struct MenuPersistentState {
        PlayerInfo playerInfo; // Contains device_id and read_pages
        std::set<std::string> available_content_ids; // IDs of content/files that are "unlocked"
    };

    // Global instance of the current menu state, loaded from SD card.
    // This is accessed by menu_visibility.cpp for evaluations.
    extern const MenuPersistentState& get_current_menu_state();

    // --- Functions for managing the consolidated persistent state ---
    bool load_menu_persistent_state(MenuPersistentState& state);
    bool save_menu_persistent_state(const MenuPersistentState& state);
    bool initialize_default_persistent_state_if_needed();


    // --- Convenience functions operating on the loaded state (or loading/saving internally) ---
    // These will now typically be called by the MQTT message handler or other game logic.
    // They will update a global instance of MenuPersistentState and then save it.

    void mark_content_as_available(const std::string& content_id);
    void mark_content_as_unavailable(const std::string& content_id); // If content can be re-locked
    bool is_content_available(const std::string& content_id); // Checks against the loaded state

    void mark_page_as_viewed(const std::string& page_id); // page_id is the identifier for the content/page
    bool has_page_been_viewed(const std::string& page_id); // Checks against the loaded state

    // Old functions - decide if they are still needed or if MenuPersistentState supersedes them
    // bool player_info_exists_on_sd(); 
    // bool read_player_info_from_sd(PlayerInfo &info); // Superseded by load_menu_persistent_state
    // bool write_default_player_info_to_sd(); // Superseded by initialize_default_persistent_state_if_needed
}
