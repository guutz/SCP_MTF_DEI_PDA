// main/menu_visibility.h
#ifndef MENU_VISIBILITY_H
#define MENU_VISIBILITY_H

#include "menu_structures.h" // For MenuItemDefinition, MenuItemVisibilityCondition, G_MenuScreens (if extern here)
#include "persistent_state.h" // For MenuPersistentState
#include <string>

// Function to set up MQTT subscriptions for visibility conditions based on G_MenuScreens
void setup_mqtt_visibility_handlers();

// Function to parse a visibility condition string into a MenuItemVisibilityCondition struct
MenuItemVisibilityCondition parse_visibility_condition(const std::string& condition_str);

// Function to evaluate all visibility conditions for a menu item
// Takes the item's definition. Internally fetches the current persistent state.
bool evaluate_menu_item_visibility(const MenuItemDefinition& item_def);

// Function to set an MQTT state variable (called by MQTT callbacks)
// This updates the internal map used by VISIBILITY_MQTT_STATE.
void set_mqtt_state_variable(const std::string& topic, const std::string& value);

// Helper function (if needed externally, otherwise keep static in .cpp)
// bool evaluate_visibility_condition(const MenuItemVisibilityCondition& condition);


#endif // MENU_VISIBILITY_H
