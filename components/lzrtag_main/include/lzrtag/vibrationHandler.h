#pragma once

#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"
#include "mcp23008_wrapper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace LZR {

class VibrationHandler {
public:
    VibrationHandler(Player* player, LZRTag::Weapon::Handler* gunHandler);
    void set_mcp23008_instance(mcp23008_t *mcp);
    void button_bump();
    float heartbeat_pattern();
    void apply_hit_pattern(float &prev);
    void apply_button_pattern(float &prev);
    void apply_shot_pattern(float &prev);
    void vibrator_tick();
private:
    Player* player_;
    LZRTag::Weapon::Handler* gunHandler_;
    uint32_t vibr_motor_count_;
    TickType_t last_button_tick_;
    mcp23008_t *s_lzrtag_mcp_;
};

} // namespace LZR