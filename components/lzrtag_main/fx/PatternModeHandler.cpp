// PatternModeHandler.cpp
#include "lzrtag/PatternModeHandler.h"
#include "xasin/BatteryManager.h"
#include "xasin/neocontroller/Color.h"

void PatternModeHandler::switch_to_mode(LZR::pattern_mode_t mode) {
    modePatterns_.clear();
    target_mode_ = mode;
    switch (mode) {
    case LZR::OFF:
    case LZR::PATTERN_MODE_MAX:
    case LZR::PLAYER_DECIDED:
    case LZR::BATTERY_LEVEL:
        break;
    case LZR::CONNECTING: {
        modePatterns_.emplace_back();
        auto &ip = modePatterns_[0];
        ip.pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
        ip.pattern_period = 255 * (VEST_LEDS);
        ip.pattern_p2_length = ip.pattern_period - 255;
        ip.pattern_p1_length = 2 * 255;
        ip.pattern_trap_percent = 0.5 * (1 << 16);
        ip.time_func = LZR::FX::time_func_t::TRAPEZ;
        ip.timefunc_p1_period = 600 * 1.6;
        ip.timefunc_period = ip.timefunc_p1_period;
        ip.timefunc_trap_percent = 0.9 * (1 << 16);
        break;
    }
    case LZR::OTA:
    case LZR::CHARGE: {
        switch_to_mode(LZR::IDLE);
        auto &ip = modePatterns_[0];
        ip.pattern_p1_length = 1.5 * 255;
        ip.timefunc_p1_period = 10 * 600;
        ip.timefunc_period = ip.timefunc_p1_period;
        break;
    }
    case LZR::IDLE: {
        modePatterns_.emplace_back();
        auto &ip = modePatterns_[0];
        ip.overlayColor = Xasin::NeoController::Color(0x333333);
        ip.pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
        ip.pattern_period = 255 * (VEST_LEDS);
        ip.pattern_p2_length = ip.pattern_period;
        ip.pattern_p1_length = 2 * 255;
        ip.pattern_trap_percent = 0.5 * (1 << 16);
        ip.sine_amplitude = 0.2 * (1 << 16);
        ip.sine_center = 0;
        ip.time_func = LZR::FX::time_func_t::LINEAR;
        ip.timefunc_p1_period = 600 * 8;
        ip.timefunc_period = ip.timefunc_p1_period;
        break;
    }
    case LZR::TEAM_SELECT:
        for (int i = 0; i < 3; i++) {
            modePatterns_.emplace_back();
            auto &iP = modePatterns_[i];
            iP.overlayColor = Xasin::NeoController::Color::HSV(120 * i, 255, 95);
            iP.overlayColor.alpha = 255;
            iP.pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
            iP.pattern_p1_length = 2.5 * 255;
            iP.pattern_period = 255 * (VEST_LEDS + 4);
            iP.pattern_trap_percent = (1 << 16) * 0.5;
            iP.pattern_shift = 2 * 255;
            iP.pattern_p2_length = 255 * (VEST_LEDS + 4);
            iP.pattern_trap_percent = 0.6 * (1 << 16);
            iP.time_func = LZR::FX::time_func_t::LINEAR;
            iP.timefunc_p1_period = 0.75 * 600;
            iP.timefunc_period = 5 * 600;
            iP.timefunc_shift = (2 - i) * 600 * 0.2;
        }
        break;
    case LZR::DEAD: {
        modePatterns_.emplace_back();
        auto &ip = modePatterns_[0];
        ip.overlayColor = Xasin::NeoController::Color(0x333333);
        ip.pattern_func = LZR::FX::pattern_func_t::TRAPEZ;
        ip.pattern_period = 255 * (VEST_LEDS);
        ip.pattern_p2_length = ip.pattern_period;
        ip.pattern_p1_length = 2 * 255;
        ip.pattern_trap_percent = 0.5 * (1 << 16);
        ip.time_func = LZR::FX::time_func_t::LINEAR;
        ip.timefunc_p1_period = 600 * 3;
        ip.timefunc_period = ip.timefunc_p1_period;
        break;
    }
    case LZR::ACTIVE: {
        modePatterns_.emplace_back();
        auto &ip = modePatterns_[0];
        ip.pattern_func = LZR::FX::pattern_func_t::SINE;
        ip.pattern_period = 255 * (VEST_LEDS) * 5;
        ip.pattern_p1_length = 5 * 255;
        ip.sine_amplitude = 0.1 * (1 << 16);
        ip.sine_center = 0.55 * (1 << 16);
        ip.time_func = LZR::FX::time_func_t::EQUAL_SINE;
        ip.timefunc_p1_period = 600 * 8;
        ip.timefunc_period = ip.timefunc_p1_period;
        modePatterns_.emplace_back();
        auto &ij = modePatterns_[1];
        ij.pattern_func = LZR::FX::pattern_func_t::SINE;
        ij.pattern_period = -255 * (VEST_LEDS);
        ij.pattern_p1_length = 3 * 255;
        ij.sine_amplitude = 0.05 * (1 << 16);
        ij.sine_center = 0;
        ij.time_func = LZR::FX::time_func_t::LINEAR;
        ij.timefunc_p1_period = 2 * 600;
        ij.timefunc_period = ij.timefunc_p1_period;
        ij.overlay = false;
        break;
    }
    }
    current_mode_ = mode;
}

void PatternModeHandler::tick() {
    if (!rgbController_ || !colorSet_) return;
    LZR::pattern_mode_t targetPattern = target_mode_;
    // Example: if PLAYER_DECIDED, you may want to get the mode from a player object
    // if (target_mode_ == LZR::PLAYER_DECIDED && player_) targetPattern = player_->get_brightness();
    // Add OTA state logic if needed
    if (targetPattern != current_mode_)
        switch_to_mode(targetPattern);
    switch (current_mode_) {
    case LZR::OFF:
    case LZR::PATTERN_MODE_MAX:
    case LZR::PLAYER_DECIDED:
        break;
    case LZR::CHARGE:
        modePatterns_[0].overlayColor = Xasin::NeoController::Color::HSV(120 * battery_->current_capacity() / 100.0, 200, 35);
        rgbController_->colors.fill(Xasin::NeoController::Color(Material::GREEN, 20), 1, -1);
        break;
    case LZR::IDLE:
        modePatterns_[0].overlayColor = Xasin::NeoController::Color(0x333333);
        modePatterns_[0].overlayColor.merge_overlay(colorSet_->vestBase, 45);
        break;
    case LZR::BATTERY_LEVEL:
        rgbController_->colors.fill(Xasin::NeoController::Color::HSV(120 * battery_->current_capacity() / 100.0, 200, 50), 1, -1);
        break;
    case LZR::CONNECTING:
        modePatterns_[0].overlayColor = (/*mqtt_->is_disconnected() == 1*/ false) ? Xasin::NeoController::Color(Material::AMBER, 70) : Xasin::NeoController::Color(Material::PINK, 70);
        break;
    case LZR::DEAD:
    case LZR::TEAM_SELECT: {
        Xasin::NeoController::Color tColor = colorSet_->vestBase;
        tColor.bMod(90);
        rgbController_->colors.fill(tColor, 1, -1);
        break;
    }
    case LZR::ACTIVE:
        modePatterns_[0].overlayColor = colorSet_->vestBase;
        modePatterns_[1].overlayColor = colorSet_->vestShotEnergy;
        break;
    case LZR::OTA:
        modePatterns_[0].overlayColor = Xasin::NeoController::Color(Material::BLUE, 200, 35);
        break;
    }
    for (auto &pattern : modePatterns_) {
        pattern.tick();
        for (int i = 0; i < rgbController_->length - 1; ++i)
            pattern.apply_color_at(rgbController_->colors[i + 1], i);
    }
}
