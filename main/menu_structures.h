#ifndef MENU_STRUCTURES_H
#define MENU_STRUCTURES_H

#include "lvgl.h"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <chrono>

// --- Forward Declarations ---
struct MenuScreenDefinition; 

// --- Menu Item Render Type ---
// Defines how a menu item should be visually represented on the screen.
typedef enum {
    RENDER_AS_BUTTON,
    RENDER_AS_STATIC_LABEL
} MenuItemRenderType;

// --- Menu Item Action Type ---
// Defines what action a button should perform when clicked.
// ACTION_NONE is used for items that are not interactive (like static labels).
typedef enum {
    ACTION_NONE, 
    ACTION_NAVIGATE_SUBMENU,
    ACTION_DISPLAY_TEXT_CONTENT_SCREEN, // A button that opens a dedicated text screen with inline content
    ACTION_DISPLAY_TEXT_FILE_SCREEN,    // A button that opens a dedicated text screen with content from a file
    ACTION_EXECUTE_PREDEFINED_FUNCTION,
    ACTION_GO_BACK
} MenuItemAction;

// --- Menu Item Visibility Condition Types ---
// Defines how a menu item's visibility should be determined.
typedef enum {
    VISIBILITY_ALWAYS,           // Always visible (default behavior)
    VISIBILITY_DATETIME_RANGE,   // Visible only within a specific datetime range
    VISIBILITY_MQTT_STATE,       // Visible based on MQTT subscription state (e.g., a live sensor value)
    VISIBILITY_CONTENT_AVAILABLE, // Visible if a specific content ID is marked as available in PersistentState
    VISIBILITY_TIME_RANGE,       // Visible only within specific time of day
    VISIBILITY_DATE_RANGE,       // Visible only within specific date range
    VISIBILITY_CONTENT_VIEWED,   // Visible if a specific content ID/page has been viewed (from PersistentState)
    VISIBILITY_CONTENT_NOT_VIEWED // Visible if a specific content ID/page has NOT been viewed (from PersistentState)
} MenuItemVisibilityType;

// --- Visibility Condition Structure ---
// Contains parameters for determining menu item visibility.
typedef struct {
    MenuItemVisibilityType type;
    
    // For datetime/time/date range conditions
    // These were previously std::string start_time, end_time.
    // Renaming for clarity based on their actual usage for different types.
    std::string start_datetime_str; // Format: \"YYYY-MM-DD HH:MM\" (for DATETIME_RANGE)
    std::string end_datetime_str;   // Format: \"YYYY-MM-DD HH:MM\" (for DATETIME_RANGE)
    std::string start_time_str;     // Format: \"HH:MM\" (for TIME_RANGE)
    std::string end_time_str;       // Format: \"HH:MM\" (for TIME_RANGE)
    std::string start_date_str;     // Format: \"YYYY-MM-DD\" (for DATE_RANGE)
    std::string end_date_str;       // Format: \"YYYY-MM-DD\" (for DATE_RANGE)
    
    // For MQTT_STATE, CONTENT_AVAILABLE, CONTENT_VIEWED, CONTENT_NOT_VIEWED conditions
    std::string state_variable;  // Name of the MQTT topic, content_id, or page_id
    std::string state_value;     // Required value for MQTT_STATE visibility (empty = any non-null value)
    
    // std::function<bool()> evaluate_func; // This was likely for an older design, can be removed if evaluation is centralized
} MenuItemVisibilityCondition;

// --- Menu Item Definition ---
// Describes a single item (button or label) within a menu screen.
typedef struct {
    std::string name; // Added name for easier identification, e.g. for logging or specific item manipulation
    std::string text_to_display;      // For a button's label, or the static text content for a label item.
    MenuItemRenderType render_type;   // How this item should be rendered (e.g., button or static label).
    MenuItemAction action;            // If it's a button, what action it performs. Ignored for static labels.
    std::string action_target;        // Target for the action (e.g., submenu name, file path, function key). Ignored for static labels if action is ACTION_NONE.
    MenuItemVisibilityCondition visibility; // Visibility condition for this item (optional)
} MenuItemDefinition;

// --- Menu Screen Definition ---
// Describes a complete menu screen.
struct MenuScreenDefinition {
    std::string name;                 // Unique name/ID for this screen.
    std::string title;                // Title to display on the screen.
    std::vector<MenuItemDefinition> items; // List of items (buttons or labels).
    std::string defined_parent_name;  // Optional: Statically defined parent in menu.txt. Used for "Back" navigation.
    lv_obj_t* (*custom_create_func)(void); // Optional: For screens needing custom C++ creation logic.
};

// --- Predefined C++ Functions ---
typedef void (*predefined_cpp_function_t)(void);
extern std::map<std::string, predefined_cpp_function_t> G_PredefinedFunctions;

// --- Global Menu Data Store ---
extern std::map<std::string, MenuScreenDefinition> G_MenuScreens;

// --- SD Manager Function (declaration) ---
// Parses the menu definition file and populates G_MenuScreens.
bool parse_menu_definition_file(const char* file_path_on_sd);

// --- Menu Visibility Function Declarations from menu_visibility.h ---
// (Ensure these match what's actually in menu_visibility.h)

// No longer needed here if menu_visibility.h is included by files needing these.
// bool evaluate_menu_item_visibility(const MenuItemDefinition& item_def);
// MenuItemVisibilityCondition parse_visibility_condition(const std::string& visibility_str);
// void setup_mqtt_visibility_handlers();
// void set_mqtt_state_variable(const std::string& name, const std::string& value);

// Removed outdated/unused declarations for get_mqtt_state_variable, init_mqtt_handler_for_visibility,
// setup_mesh_visibility_handlers, set_mesh_state_variable, get_mesh_state_variable.

#endif // MENU_STRUCTURES_H