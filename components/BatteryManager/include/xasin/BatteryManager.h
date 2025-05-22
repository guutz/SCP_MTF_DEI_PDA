/*
 * BatteryManager.h
 *
 *  Created on: 9 Jan 2019
 *      Author: xasin
 */

#ifndef XASLIBS_HOUSEKEEPING_BATTERYMANAGER_BATTERYMANAGER_H_
#define XASLIBS_HOUSEKEEPING_BATTERYMANAGER_BATTERYMANAGER_H_

#include "stdint.h"

namespace Housekeeping {

class BatteryManager {
private:
	uint16_t current_mv_var;

public:
	const uint8_t cell_count = 1; // Always 1 for 9V
	const uint32_t cutoff_voltage = 6000; // 6V cutoff for 9V battery

	BatteryManager(uint8_t cellCount = 1);

	// Returns 0-100% for 9V battery (linear mapping)
	uint8_t		raw_capacity_for_voltage(uint32_t millivolts);
	uint8_t 	capacity_for_voltage(uint32_t millivolts);
	uint32_t	voltage_for_raw_capacity(uint8_t percentage);
	uint32_t 	voltage_for_capacity(uint8_t percentage);

	void set_voltage(uint32_t millivolts);

	uint32_t current_mv();
	uint8_t  current_capacity();

	bool battery_ok();
};

} /* namespace Housekeeping */

#endif /* XASLIBS_HOUSEKEEPING_BATTERYMANAGER_BATTERYMANAGER_H_ */
