/*
 * setup.h
 *
 *  Created on: 7 Jan 2019
 *      Author: xasin
 */

#ifndef MAIN_SETUP_H_
#define MAIN_SETUP_H_

#include "xasin/BatteryManager.h"

#include "xasin/audio.h"
#include "xasin/neocontroller.h"

#include "xasin/mqtt/Handler.h"

#include "player.h"
#include "weapon.h"


namespace LZR {

enum CORE_WEAPON_STATUS {
	INITIALIZING,
	DISCHARGED,
	CHARGING,
	NOMINAL,
};

extern CORE_WEAPON_STATUS main_weapon_status;

extern Housekeeping::BatteryManager battery;

extern Xasin::Audio::TX	audioManager;
extern Xasin::NeoController::NeoController RGBController;


extern Xasin::MQTT::Handler mqtt;

extern LZR::Player	player;

extern std::vector<LZRTag::Weapon::BaseWeapon *> weapons;
extern LZRTag::Weapon::Handler gunHandler;

void setup();

}

#endif /* MAIN_SETUP_H_ */
