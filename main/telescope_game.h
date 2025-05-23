#ifndef TELESCOPE_GAME_H
#define TELESCOPE_GAME_H

#include <string>

// Function to check if the commanded Alt/Az matches a secret combination
bool check_secret_combination(float alt, float az, std::string &message);

#endif // TELESCOPE_GAME_H
