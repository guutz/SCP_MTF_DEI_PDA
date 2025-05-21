/*
 * player.cpp
 *
 *  Created on: 27 Feb 2019
 *      Author: xasin
 */

#include "lzrtag/player.h"
#include <stdlib.h>

#include "esp_log.h"

#include "lzrtag/animatorThread.h"
#include "lzrtag/colorSets.h"

#include <cJSON.h>
#include <cstring>

namespace LZR {

Player::Player(const std::string devID, Xasin::Communication::CommHandler &comm_handler) :
	ID(0),
	team(0), brightness(0),
	isMarked(false),
	markerColor(0),
	heartbeat(false),
	name(""),
	deadUntil(0), hitUntil(0), vibrateUntil(0),
	currentGun(0), shotLocked(0),
	comm_handler(comm_handler), should_reload(false) {

	comm_handler.subscribe("event/#",
		[this](const Xasin::Communication::CommReceivedData& message) {
			std::string topic_str(message.topic.begin(), message.topic.end());
			std::string payload_str(message.payload.begin(), message.payload.end());

			if(topic_str == "hit")
				hitUntil = xTaskGetTickCount() + atof(payload_str.c_str())*600;
			else if(topic_str == "vibrate")
				vibrateUntil = xTaskGetTickCount() + atof(payload_str.c_str())*600;
			else if(topic_str == "reload")
				should_reload = true;
		}
	);

	comm_handler.subscribe("get/#",
			[this](const Xasin::Communication::CommReceivedData& message) {
			std::string topic_str(message.topic.begin(), message.topic.end());
			std::string payload_str(message.payload.begin(), message.payload.end());

		cJSON * json;
			if(payload_str.length() > 0)
				json = cJSON_Parse(payload_str.c_str());
			else
				json = cJSON_CreateNull();

			if(json == nullptr)
				return;

			if(topic_str == "id" && (cJSON_IsNumber(json) || cJSON_IsNull(json)))
				ID = json->valueint;
			else if(topic_str == "team" && (cJSON_IsNumber(json) || cJSON_IsNull(json)))
				team = json->valueint;
			else if(topic_str == "brightness" && (cJSON_IsNumber(json) || cJSON_IsNull(json)))
				brightness = json->valueint;
			else if(topic_str == "gun_config") {
				if(cJSON_IsNull(json)) {
					currentGun = 0;
				}
				else if(cJSON_IsNumber(json))
					currentGun = json->valueint;
				
				shotLocked = currentGun <= 0;
			}
			else if(topic_str == "mark_config") {
				isMarked = !cJSON_IsFalse(json);
				uint32_t markerCode = json->valueint;

				if(markerCode <= 0)
					isMarked = false;
				else if(markerCode < 8)
					markerColor = LZR::teamColors[markerCode].vestShotEnergy;
				else
					markerColor = markerCode;
			}
			else if(topic_str == "heartbeat")
				heartbeat = cJSON_IsTrue(json);
			else if(topic_str == "name")
				name = json->valuestring;
			else if(topic_str == "dead") {
				if(cJSON_IsTrue(json)) {
					if(deadUntil == 0) {
						deadUntil = portMAX_DELAY;
					}
				}
				else {
					deadUntil = 0;
				}
			}

			cJSON_Delete(json);
		}
	);
}

void Player::init() {
}

void Player::tick() {
	if((deadUntil != 0) && (xTaskGetTickCount() > deadUntil)) {
		deadUntil = 0;
		comm_handler.publish("get/dead", "false", strlen("false"), 1, true);
	}
}

int Player::get_id() {
	return ID;
}

int Player::get_team() {
	if(team < 0)
		return 0;
	if(team > 7)
		return 0;
	return this->team;
}

pattern_mode_t Player::get_brightness() {
    if (!comm_handler.isConnected())
		return pattern_mode_t::CONNECTING;

	if(is_dead())
		return pattern_mode_t::DEAD;

	if(brightness < 0)
		return pattern_mode_t::IDLE;
	if(brightness > (pattern_mode_t::PATTERN_MODE_MAX - pattern_mode_t::IDLE))
		return pattern_mode_t::ACTIVE;

	return static_cast<pattern_mode_t>(brightness + pattern_mode_t::IDLE);

}

bool Player::is_marked() {
	return isMarked;
}
Xasin::NeoController::Color Player::get_marked_color() {
	return markerColor;
}
bool Player::get_heartbeat() {
	return heartbeat;
}

std::string Player::get_name() {
	return name;
}

bool Player::can_shoot() {
	if(ID == 0)
		return false;

	if(is_dead())
		return false;
	if(shotLocked)
		return false;

	return true;
}

int Player::get_gun_num() {
	return currentGun;
}
void Player::set_gun_ammo(int32_t current, int32_t clipsize, int32_t total) {
	if(gun_ammo_info == nullptr) {
		gun_ammo_info = cJSON_CreateObject();
		cJSON_AddNumberToObject(gun_ammo_info, "current", 0);
		cJSON_AddNumberToObject(gun_ammo_info, "clipsize", 0);
		cJSON_AddNumberToObject(gun_ammo_info, "total", 0);
	}

	bool changed = false;

	cJSON * json = cJSON_GetObjectItem(gun_ammo_info, "current");
	if(json->valueint != current) {
		changed = true;
		cJSON_SetNumberValue(json, current);
	}

	json = cJSON_GetObjectItem(gun_ammo_info, "clipsize");
	if(json->valueint != clipsize) {
		changed = true;
		cJSON_SetNumberValue(json, clipsize);
	}

	json = cJSON_GetObjectItem(gun_ammo_info, "total");
	if(json->valueint != total) {
		changed = true;
		cJSON_SetNumberValue(json, total);
	}

	if(changed) {
		char * printed = cJSON_PrintUnformatted(gun_ammo_info);
		comm_handler.publish("get/ammo", printed, strlen(printed), 0, true);
		cJSON_free(printed);
	}
}

bool Player::is_dead() {
	return xTaskGetTickCount() < deadUntil;
}
bool Player::is_hit() {
	return xTaskGetTickCount() < hitUntil;
}
bool Player::should_vibrate() {
	if(is_hit())
		return false;

	return xTaskGetTickCount() < vibrateUntil;
}

}
