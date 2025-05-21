#pragma once
#include <string>
#include <vector>
#include <cstdarg>

// Add a log message to the menu log (thread-safe)
void menu_log_add(const char* tag, const char* fmt, ...);
// Show the log modal dialog (call from menu functions)
void show_menu_log_modal(const char* title = "System Log");
// Get a copy of the current log buffer (for display/testing)
std::vector<std::string> menu_log_get_buffer();
