#include "lzrtag/vibrationHandler.h"
#include "mcp23008_wrapper.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lzrtag/mcp_access.h"
#include "lzrtag/player.h"
#include "lzrtag/weapon/handler.h"

namespace LZR {

VibrationHandler::VibrationHandler(Player* player, LZRTag::Weapon::Handler* gunHandler)
    : player_(player), gunHandler_(gunHandler), vibr_motor_count_(0), last_button_tick_(0), s_lzrtag_mcp_(nullptr) {}

void VibrationHandler::set_mcp23008_instance(mcp23008_t *mcp) { s_lzrtag_mcp_ = mcp; }

void VibrationHandler::button_bump() { last_button_tick_ = xTaskGetTickCount(); }

float VibrationHandler::heartbeat_pattern() {
    static float beat_amplitude = 0;
    beat_amplitude *= 0.93F;
    if (!player_ || !player_->get_heartbeat())
        return beat_amplitude;
    if ((0b101 & (xTaskGetTickCount() / 75)) == 0)
        beat_amplitude = 0.4F;
    return beat_amplitude;
}

void VibrationHandler::apply_hit_pattern(float &prev) {
    static float intensity = 0;
    static float current_hit_level = 0;
    intensity *= 0.9F;
    if (player_ && player_->is_hit())
        intensity = 1;
    if (intensity < 0.01F)
        return;
    if ((vibr_motor_count_ & 0b11) == 0)
        current_hit_level = 0.3F + 0.7F * (esp_random() >> 24) / 255.0F;
    prev = (current_hit_level * intensity) + (1 - intensity) * prev;
}

void VibrationHandler::apply_button_pattern(float &prev) {
    if ((xTaskGetTickCount() - last_button_tick_) < 20 / portTICK_PERIOD_MS)
        prev = 0.3F;
}

void VibrationHandler::apply_shot_pattern(float &prev) {
    if (gunHandler_)
        gunHandler_->apply_vibration(prev);
}

void VibrationHandler::vibrator_tick() {
    last_button_tick_++;
    float out_level = heartbeat_pattern();
    apply_button_pattern(out_level);
    apply_hit_pattern(out_level);
    apply_shot_pattern(out_level);
    mcp23008_t *mcp = s_lzrtag_mcp_;
    if (mcp)
        mcp23008_wrapper_write_pin(mcp, (MCP23008_NamedPin)MCP2_PIN_HAPTIC_MOTOR, out_level > 0.3F);
}

} // namespace LZR