/*
 * animatorThread.cpp
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#include "lzrtag/animatorThread.h"
#include "lzrtag/mcp_access.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lzrtag/setup.h"

#include "lzrtag/ManeAnimator.h"

#include "driver/ledc.h"
#include <cmath>

#include "lzrtag/colorSets.h"

#include "lzrtag/sounds.h"

#include "lzrtag/patterns/ShotFlicker.h"
#include "lzrtag/patterns/VestPattern.h"

#include "lzrtag/vibrationHandler.h"

#include "esp_log.h"

namespace LZR {

using namespace Xasin::NeoController;

ColorSet currentColors = teamColors[0];
ColorSet bufferedColors = currentColors;

FXSet 	currentFX = brightnessLevels[0];
FXSet 	bufferedFX = currentFX;

auto vestShotPattern = FX::ShotFlicker(VEST_LEDS, VEST_LEDS);
auto vestHitMarker 	 = FX::VestPattern();
auto vestDeathMarker = FX::VestPattern();

auto vestMarkedMarker = FX::VestPattern();

std::vector<FX::BasePattern*> vestPatterns = {
		&vestShotPattern,
		&vestHitMarker,
		&vestDeathMarker,
		&vestMarkedMarker,
};

void set_bat_pwr(uint8_t level, uint8_t brightness = 255) {
	uint8_t gLevel = 255 - pow(255 - 2.55*level, 2)/255;

	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255 - (255-gLevel)*pow(brightness, 2)/65025);
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 255 - gLevel*pow(brightness, 2)/65025);

	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

}

void setup_vest_patterns() {
	vestHitMarker.pattern_func = FX::pattern_func_t::TRAPEZ;
	vestHitMarker.pattern_p1_length = 1.3*255;
	vestHitMarker.pattern_period = 255*VEST_LEDS;
	vestHitMarker.pattern_p2_length = 255*VEST_LEDS;
	vestHitMarker.pattern_trap_percent = (1<<16) * 0.3;

	vestHitMarker.time_func = FX::time_func_t::LINEAR;
	vestHitMarker.timefunc_p1_period = 0.5*600;
	vestHitMarker.timefunc_period = vestHitMarker.timefunc_p1_period;

	vestHitMarker.overlayColor = Color(0xFFFFFF);
	vestHitMarker.overlayColor.alpha = 50;

	vestDeathMarker = vestHitMarker;
	vestDeathMarker.pattern_period  = 2*255;
	vestDeathMarker.pattern_p2_length = 1*255;
	vestDeathMarker.overlayColor.alpha = 80;

	vestDeathMarker.timefunc_period = 0.2*600;
	vestDeathMarker.timefunc_p1_period = 0.2*600;

	vestMarkedMarker.pattern_func = FX::pattern_func_t::TRAPEZ;
	vestMarkedMarker.pattern_period = 255 * (VEST_LEDS);
	vestMarkedMarker.pattern_p2_length = vestMarkedMarker.pattern_period - 255;

	vestMarkedMarker.pattern_p1_length = 2 * 255;
	vestMarkedMarker.pattern_trap_percent = 0.5 * (1 << 16);

	vestMarkedMarker.time_func = FX::time_func_t::TRAPEZ;
	vestMarkedMarker.timefunc_p1_period = 600 * 1.6;
	vestMarkedMarker.timefunc_period = vestMarkedMarker.timefunc_p1_period;
	vestMarkedMarker.timefunc_trap_percent = 0.9 * (1 << 16);

}

void status_led_tick() {
	float conIndB = 0;
	float batBrightness = 1;

	switch(LZR::main_weapon_status) {
	case CHARGING: {
		int cycleTime = xTaskGetTickCount() % 1200;

		if(battery.is_charging)
			batBrightness = 0.4 + 0.6*cycleTime/1200;
		else if(cycleTime%600 < 300)
			batBrightness = 0.6;
	break;
	}
	case DISCHARGED:
		if(xTaskGetTickCount()%300 < 150) {
			batBrightness = 0.1;
			conIndB = 0.5;
		}
	break;
	default:
		if(mqtt.is_disconnected() == 0)
			conIndB = (0.3 + 0.3*sin(xTaskGetTickCount()/2300.0 * M_PI));
		else if(mqtt.is_disconnected() == 1)
			conIndB = xTaskGetTickCount()%800 < 600 ? 1 : 0.5;
		else
			conIndB = xTaskGetTickCount()%800 < 400 ? 1 : 0;
	}
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 255 - pow(conIndB ,2)*255);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);


	set_bat_pwr(battery.current_capacity(), batBrightness*255);
}

TickType_t vibr_motor_count_old = 0;
void vibr_motor_tick() {
	vibrator_tick();

	return;

	vibr_motor_count_old++;

	bool vibrOn = false;

	if((xTaskGetTickCount() - gunHandler.get_last_shot_tick()) < 60)
		vibrOn = true;
	else if(player.is_hit() && player.is_dead())
		vibrOn = (vibr_motor_count_old & 0b1);
	else if(player.is_hit())
		vibrOn = (vibr_motor_count_old & 0b1010) == 0;
	else if(player.get_heartbeat())
		vibrOn = ((0b101 & (xTaskGetTickCount()/75)) == 0);
	else if(player.should_vibrate())
		vibrOn = true;

	lzrtag_set_vibrate_motor(vibrOn);

}

#define COLOR_FADE(cName, alpha) bufferedColors.cName.merge_transition(currentColors.cName, alpha)
#define FX_FADE(fxName, alpha)	 bufferedFX.fxName = (bufferedFX.fxName * (1-alpha) + currentFX.fxName * alpha)

void vest_tick() {

	////////////////////
	// Vest color fading
	////////////////////
	COLOR_FADE(muzzleFlash, 5000);
	COLOR_FADE(muzzleHeat,  5000);
	COLOR_FADE(vestBase, 	2000);
	COLOR_FADE(vestShotEnergy, 5000);

	FX_FADE(minBaseGlow, 0.02);
	FX_FADE(maxBaseGlow, 0.02);
	FX_FADE(waverAmplitude, 0.02);
	FX_FADE(waverPeriod, 0.03);
	FX_FADE(waverPositionShift, 0.01);

	////////////////////
	// Muzzle heatup
	////////////////////
	Color newMuzzleColor = 	bufferedColors.muzzleHeat;
	newMuzzleColor.bMod(gunHandler.get_gun_heat() * 0.6F);

	////////////////////
	// Muzzle flash for shots
	////////////////////
	if(gunHandler.was_shot_tick())
	 	newMuzzleColor.merge_overlay(bufferedColors.muzzleFlash);
	RGBController.colors[0] = newMuzzleColor;

	////////////////////
	// Generation of vest base color + heatup
	////////////////////
	Color currentVestColor = bufferedColors.vestBase;
	 currentVestColor.bMod(bufferedFX.minBaseGlow +
	 		(bufferedFX.maxBaseGlow - bufferedFX.minBaseGlow)*gunHandler.get_gun_heat()/255.0);

	// Reset of all colors
	RGBController.colors.fill(0, 1, -1);

	////////////////////
	// Basic vest wavering
	///////////////////
	FX::mode_tick();

	// Determine whether hit flicker shall run
	vestHitMarker.enabled = player.is_hit() && !player.is_dead();
	vestDeathMarker.enabled = player.is_hit() && player.is_dead();

	vestMarkedMarker.enabled = player.is_marked();
	vestMarkedMarker.overlayColor = player.get_marked_color();
	vestMarkedMarker.overlayColor.alpha = 100;

	/////////////////////////////////////
	// Vest shot flaring & wave animation
	/////////////////////////////////////
	for(auto p : vestPatterns) {
		p->tick();
		for(int i=1; i<RGBController.length; i++) {
			p->apply_color_at(RGBController.colors[i], i-1);
		}
	}
}

void animation_thread(void *args) {
	while(true) {
		int g_num = player.get_gun_num();
		if(g_num > 0 && g_num <= weapons.size())
			gunHandler.set_weapon(weapons[g_num]);

		gunHandler.update_btn(!lzrtag_get_trigger());
		gunHandler.fx_tick();

		if(player.should_reload)
			gunHandler.tempt_reload();
		player.should_reload = false;

		auto ammo_info = gunHandler.get_ammo();
		player.set_gun_ammo(ammo_info.current_ammo, ammo_info.clipsize, ammo_info.total_ammo);

		status_led_tick();

		if(main_weapon_status == NOMINAL) {
			player.tick();

			currentColors = teamColors[player.get_team()];
			currentFX = brightnessLevels[player.get_brightness()];

			vibr_motor_tick();
		}
		else // Disable vibration motor
			lzrtag_set_vibrate_motor(false);

		vest_tick();

		Color newMuzzleColor = RGBController.colors[0];
		Color actualMuzzle = Color();
		actualMuzzle.r = newMuzzleColor.g;
		actualMuzzle.g = newMuzzleColor.r;
		actualMuzzle.b = newMuzzleColor.b;
		RGBController.colors[0] = actualMuzzle;

		RGBController.update();

		vTaskDelay(10);
	}
}

void start_animation_thread() {
	currentColors = teamColors[0];
	bufferedColors = currentColors;

	currentFX = brightnessLevels[0];

	setup_vest_patterns();

	FX::init();

	Sounds::init();

    TaskHandle_t animatorTaskHandle;
    xTaskCreatePinnedToCore(animation_thread, "Animator", 4*1024, nullptr, 10, &animatorTaskHandle, 1);
}
}
