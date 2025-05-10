#ifndef MENU_STRUCTURES_H
#define MENU_STRUCTURES_H

#include "lvgl.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

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

// --- Menu Item Definition ---
// Describes a single item (button or label) within a menu screen.
typedef struct {
    std::string text_to_display;      // For a button's label, or the static text content for a label item.
    MenuItemRenderType render_type;   // How this item should be rendered (e.g., button or static label).
    MenuItemAction action;            // If it's a button, what action it performs. Ignored for static labels.
    std::string action_target;        // Target for the action (e.g., submenu name, file path, function key). Ignored for static labels if action is ACTION_NONE.
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

#endif // MENU_STRUCTURES_H