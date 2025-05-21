#include "menu_log.h"
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include "lvgl.h"
#include "menu_functions.h" // For modal dialog helpers

#define MENU_LOG_MAX_LINES 64
#define MENU_LOG_LINE_MAXLEN 128

static std::vector<std::string> log_buffer;
static std::mutex log_mutex;

void menu_log_add(const char* tag, const char* fmt, ...) {
    char line[MENU_LOG_LINE_MAXLEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    // Timestamp (optional, can use time() or tick count)
    char ts[16];
    snprintf(ts, sizeof(ts), "%lu", (unsigned long)time(nullptr));
    std::string msg = std::string("[") + ts + "] [" + tag + "] " + line;
    std::lock_guard<std::mutex> lock(log_mutex);
    log_buffer.push_back(msg);
    if (log_buffer.size() > MENU_LOG_MAX_LINES) {
        log_buffer.erase(log_buffer.begin(), log_buffer.begin() + (log_buffer.size() - MENU_LOG_MAX_LINES));
    }
}

std::vector<std::string> menu_log_get_buffer() {
    std::lock_guard<std::mutex> lock(log_mutex);
    return log_buffer;
}

// LVGL log modal dialog
void show_menu_log_modal(const char* title) {
    // Compose log text
    std::vector<std::string> lines = menu_log_get_buffer();
    std::string all;
    for (const auto& l : lines) {
        all += l + "\n";
    }
    if (all.empty()) all = "(No log messages)";
    // Show in modal dialog (reuse create_modal_dialog from menu_functions.cpp)
    extern ModalDialogParts create_modal_dialog(const char*, const char*, const char*, lv_event_cb_t, const char*, lv_event_cb_t, void*, void*, int);
    ModalDialogParts dialog = create_modal_dialog(
        title ? title : "System Log",
        all.c_str(),
        "CLOSE", nullptr,
        nullptr, nullptr, nullptr, nullptr, 2
    );
    // Add joystick group support for close button
    lv_group_t* joy_group = lvgl_joystick_get_group();
    lv_group_add_obj(joy_group, dialog.btn1);
}
