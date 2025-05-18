/*
 * setup.cpp
 *
 *  Created on: 25 Jan 2019
 *      Author: xasin
 */

#include "lzrtag/setup.h"

#include "lzrtag/IR.h"
#include "lzrtag/animatorThread.h"

#include "driver/ledc.h"

#include <array>

#include "esp_log.h"

#include "lzrtag/weapon_defs.h"

#include <xnm/net_helpers.h>

#include "lzrtag/vibrationHandler.h"

namespace LZR {

CORE_WEAPON_STATUS main_weapon_status = INITIALIZING;

Housekeeping::BatteryManager battery = Housekeeping::BatteryManager();

Xasin::Audio::TX  audioManager;
Xasin::NeoController::NeoController 	RGBController = Xasin::NeoController::NeoController(PIN_WS2812_OUT, RMT_CHANNEL_0, WS2812_NUMBER);


Xasin::MQTT::Handler mqtt = Xasin::MQTT::Handler();

LZR::Player player = LZR::Player("", mqtt);

std::vector<LZRTag::Weapon::BaseWeapon *> weapons;
LZRTag::Weapon::Handler gunHandler(audioManager);

void core_processing_task(void *args) {
	while(true) {
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		audioManager.largestack_process();
	}
}


void setup_audio() {
	TaskHandle_t processing_task;
	xTaskCreate(core_processing_task, "LARGE", 32768, nullptr, 5, &processing_task);

	audioManager.init(processing_task);

	audioManager.volume_mod = 160;
}

std::array<uint16_t, 20> battery_samples;
int8_t battery_sample_pos = -1;
TickType_t lastBatteryUpdate = 0;

void setup_ping_req() {
	mqtt.subscribe_to("ping_signal",
			[](Xasin::MQTT::MQTT_Packet data) {

		auto system_info = cJSON_CreateObject();
		auto battery_json = cJSON_AddObjectToObject(system_info, "battery");

		cJSON_AddNumberToObject(battery_json, "mv", battery.current_mv());
		cJSON_AddNumberToObject(battery_json, "percentage", battery.current_capacity());

		cJSON_AddNumberToObject(system_info, "ping", uint32_t((xTaskGetTickCount() - *reinterpret_cast<const uint32_t*>(data.data.data()))/0.6));
		cJSON_AddNumberToObject(system_info, "heap", esp_get_free_heap_size());

		char * json_print = cJSON_Print(system_info);
		cJSON_Delete(system_info);

		mqtt.publish_to("get/_ping", json_print, strlen(json_print), 1);

		cJSON_free(json_print);
	});
}

void send_ping_req() {
	uint32_t outData = xTaskGetTickCount();
	mqtt.publish_to("ping_signal", &outData, 4, 0);
}


void housekeeping_thread(void *args) {
	TickType_t nextHWTick = xTaskGetTickCount();

	while(true) {
		if(xTaskGetTickCount() > nextHWTick) {

			if(!mqtt.is_disconnected())
				send_ping_req();

			nextHWTick += 1800;
		}


		vTaskDelay(30);

		if(XNM::NetHelpers::OTA::get_state() == XNM::NetHelpers::OTA::REBOOT_NEEDED)
			esp_restart();
	}
}

void shutdown_system() {
	LZR::FX::target_mode = LZR::OFF;
	vTaskDelay(100);

	ESP_LOGE("CORE", "Proper deep-sleep was not configured yet!!");

	// TODO Add proper power-down, the ESP-IDF's power
	// management changed!

	//esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
	//esp_deep_sleep_start();
}

void setup_weapons() {
	weapons.push_back(new LZRTag::Weapon::ShotWeapon(gunHandler, colibri_config));
	weapons.push_back(new LZRTag::Weapon::ShotWeapon(gunHandler, whip_config));
	weapons.push_back(new LZRTag::Weapon::ShotWeapon(gunHandler, steelfinger_config));
	weapons.push_back(new LZRTag::Weapon::ShotWeapon(gunHandler, sw_554_config));

	weapons.push_back(new LZRTag::Weapon::HeavyWeapon(gunHandler, nico_6_config));

	weapons.push_back(new LZRTag::Weapon::BeamWeapon(gunHandler, scalpel_cfg));
}

void setup() {
	vTaskDelay(10);


	IR::init();

	xTaskCreate(housekeeping_thread, "Housekeeping", 3*1024, nullptr, 10, nullptr);

	setup_audio();
	start_animation_thread();

	gunHandler.start_thread();
	gunHandler.can_shoot_func = []() {
		return player.can_shoot();
	};
	gunHandler.on_shot_func = []() { IR::send_signal(); };
	
	setup_weapons();

    LZR::FX::target_mode = LZR::BATTERY_LEVEL;
    vTaskDelay(200);


	player.init();

	vTaskDelay(3*600);
	LZR::FX::target_mode = LZR::PLAYER_DECIDED;

	setup_ping_req();

	main_weapon_status = NOMINAL;



	ESP_LOGI("LZR::Core", "Init finished");
}

}
