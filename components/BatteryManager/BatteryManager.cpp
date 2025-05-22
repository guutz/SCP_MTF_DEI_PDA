/*
 * BatteryManager.cpp
 *
 *  Created on: 9 Jan 2019
 *      Author: xasin
 */

#include "xasin/BatteryManager.h"

namespace Housekeeping {

BatteryManager::BatteryManager(uint8_t cellCount)
	: current_mv_var(9000), // Start at 9V
	  cell_count(1), // Always 1 for 9V
	  cutoff_voltage(6000) // 6V cutoff for 9V battery
{
}

uint8_t BatteryManager::raw_capacity_for_voltage(uint32_t millivolts) {
	// Linear mapping: 9V = 100%, 6V = 0%
	if (millivolts >= 9000) return 100;
	if (millivolts <= 6000) return 0;
	return (uint8_t)(((int32_t)millivolts - 6000) * 100 / 3000);
}

uint8_t BatteryManager::capacity_for_voltage(uint32_t millivolts) {
	return raw_capacity_for_voltage(millivolts);
}

uint32_t BatteryManager::voltage_for_raw_capacity(uint8_t percentage) {
	// Inverse of above: 0% = 6V, 100% = 9V
	if (percentage >= 100) return 9000;
	if (percentage == 0) return 6000;
	return 6000 + (percentage * 30);
}

uint32_t BatteryManager::voltage_for_capacity(uint8_t percentage) {
	return voltage_for_raw_capacity(percentage);
}

void BatteryManager::set_voltage(uint32_t millivolts) {
	current_mv_var = millivolts;
}

uint8_t BatteryManager::current_capacity() {
	return raw_capacity_for_voltage(current_mv_var);
}

uint32_t BatteryManager::current_mv() {
	return current_mv_var;
}

bool BatteryManager::battery_ok() {
	return current_mv_var >= 6000;
}

} /* namespace Housekeeping */
