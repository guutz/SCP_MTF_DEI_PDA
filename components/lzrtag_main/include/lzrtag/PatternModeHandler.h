// PatternModeHandler.h
#pragma once
#include "lzrtag/patterns/VestPattern.h"
#include "lzrtag/animatorThread.h"
#include <vector>

class PatternModeHandler {
public:
    PatternModeHandler(LZR::ColorSet* colorSet, Housekeeping::BatteryManager* battery, Xasin::NeoController::NeoController* rgbController)
        : colorSet_(colorSet), battery_(battery), rgbController_(rgbController) {}
    ~PatternModeHandler() = default;

    void switch_to_mode(LZR::pattern_mode_t mode);
    void tick();

private:
    std::vector<LZR::FX::VestPattern> modePatterns_;
    LZR::pattern_mode_t target_mode_ = LZR::OFF;
    LZR::pattern_mode_t current_mode_ = LZR::OFF;
    LZR::ColorSet* colorSet_;
    Housekeeping::BatteryManager* battery_;
    Xasin::NeoController::NeoController* rgbController_;
};
