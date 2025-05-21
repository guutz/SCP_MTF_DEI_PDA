/*
 * colorSets.cpp
 *
 *  Created on: 15 Mar 2019
 *      Author: xasin
 */


#include "lzrtag/colorSets.h"

namespace LZR {

const ColorSet teamColors[] = {
		{ // Team 0 - Inactive/Grey
			.muzzleFlash = 0x555555,
			.muzzleHeat  = 0,
			.vestBase    = 0,
			.vestShotEnergy = 0,
		},
		{ // Team 1 - Red
			.muzzleFlash = Material::CYAN,
			.muzzleHeat  = 0x44FFFF,
			.vestBase = Material::RED,
			.vestShotEnergy = Color(Material::RED).merge_overlay(Material::DEEP_ORANGE, 50)
		},
		{ // Team 2 - Green
			.muzzleFlash = Material::PINK,
			.muzzleHeat  = 0xFF44FF,
			.vestBase	 = Material::GREEN,
			.vestShotEnergy = Color(Material::GREEN).merge_overlay(Material::LIME, 70)
		},
		{ // Team 3 - Orange
			.muzzleFlash = Material::BLUE,
			.muzzleHeat  = 0x5555FF,
			.vestBase 	 = Material::ORANGE,
			.vestShotEnergy = Material::YELLOW,
		},
		{ // Team 4 - Blue
			.muzzleFlash = Material::YELLOW,
			.muzzleHeat  = 0xFFFF44,
			.vestBase 	 = Material::BLUE,
			.vestShotEnergy = Material::INDIGO,
		},
		{ // Team 5 - Purple
			.muzzleFlash = Material::GREEN,
			.muzzleHeat  = 0x55FF55,
			.vestBase	 = Material::PURPLE,
			.vestShotEnergy = Material::PINK,
		},
		{ // Team 6 - Cyan
			.muzzleFlash = Material::RED,
			.muzzleHeat  = 0xFF5555,
			.vestBase 	 = Material::CYAN,
			.vestShotEnergy = Color(Material::CYAN).merge_overlay(Material::DEEP_ORANGE, 20)
		},
		{ // Team 7 - White
			.muzzleFlash = 0xFFFFFF,
			.muzzleHeat  = 0x999999,
			.vestBase 	 = 0xEEEEFF,
			.vestShotEnergy = 0xAAAAFF
		}
};
const size_t NUM_TEAM_COLORS = sizeof(teamColors) / sizeof(teamColors[0]);

const FXSet brightnessLevels[] = {
		{	// Idle
			.minBaseGlow = 130,
			.maxBaseGlow = 130,

			.waverAmplitude = 0.8,
			.waverPeriod 	= 2000,
			.waverPositionShift = 0.5,
		},
		{	// Disabled glow
			.minBaseGlow = 140, .maxBaseGlow = 140,
			.waverAmplitude = 1,
			.waverPeriod 	= 1300,
			.waverPositionShift = 0.5,
		},
		{
			.minBaseGlow = 180, .maxBaseGlow = 200,
			.waverAmplitude = 0.3,
			.waverPeriod = 500,
			.waverPositionShift = 0.2,
		},
		{
			.minBaseGlow = 230, .maxBaseGlow = 255,
			.waverAmplitude = 0.3,
			.waverPeriod = 600,
			.waverPositionShift = 0.2,
		}
};
const size_t NUM_BRIGHTNESS_LEVELS = sizeof(brightnessLevels) / sizeof(brightnessLevels[0]);

}
