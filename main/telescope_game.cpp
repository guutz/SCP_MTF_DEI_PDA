#include "telescope_game.h"
#include "menu_log.h"
#include <vector>
#include <utility>
#include <string>
#include <cmath>

// Define secret Alt/Az combinations
static const std::vector<std::pair<float, float>> secret_combinations = {
    {45.0f, 90.0f}, // Example: Alt 45, Az 90
    {30.0f, 180.0f},
    {60.0f, 270.0f}
};

// Check if the commanded Alt/Az matches any secret combination
bool check_secret_combination(float alt, float az, std::string &message) {
    for (const auto &combo : secret_combinations) {
        if (fabs(combo.first - alt) < 0.1f && fabs(combo.second - az) < 0.1f) {
            message = "Congratulations! You've unlocked a secret message!";
            return true;
        }
    }
    return false;
}
