/*
 * Handler.cpp
 *
 *  Created on: 3 Feb 2019
 *      Author: xasin
 */

#include "xasin/mqtt/Handler.h"
#include "xasin/mqtt/Subscription.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#include <xnm/net_helpers.h>

namespace Xasin {
namespace MQTT {

const char *mqtt_tag = "XNMQTT";

esp_err_t mqtt_handle_caller(esp_mqtt_event_t *event) {
	reinterpret_cast<Handler*>(event->user_context)->mqtt_handler(event);

	return ESP_OK;
}

Handler::Handler()
	: subscriptions(),
	  mqtt_handle(nullptr),
	  mqtt_started(false), mqtt_connected(false),
	  status_topic("status"), status_msg("OK") {

	config_lock = xSemaphoreCreateMutex();

	base_topic.reserve(64);

	base_topic 	= "/esp32/";
	base_topic += CONFIG_PROJECT_NAME;
	base_topic += "/";
	base_topic += XNM::NetHelpers::get_device_id();
	base_topic += "/";
}
Handler::Handler(const std::string &base_t) : Handler() {
	base_topic = base_t;
}

void Handler::topicsize_string(std::string &topic) {
	if(topic[0] == '/')
		return;

	topic = base_topic + topic;
}

void Handler::start(const mqtt_cfg &config) {
	assert(!mqtt_started);

	mqtt_started = true;
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_NONE);

	mqtt_cfg modConfig = config;

	modConfig.event_handle = mqtt_handle_caller;
	modConfig.user_context = this;

	mqtt_handle = esp_mqtt_client_init(&modConfig);
	esp_mqtt_client_start(mqtt_handle);
}
void Handler::start(const std::string uri) {
	mqtt_cfg config = {};
	config.uri = uri.data();

	config.disable_clean_session = false;

	config.use_global_ca_store = true;

	topicsize_string(this->status_topic);

	config.lwt_topic = this->status_topic.data();
	config.lwt_retain = true;
	config.lwt_qos = 1;
	config.lwt_msg_len = 0;

	config.keepalive = 5;

	start(config);
}


void Handler::mqtt_handler(esp_mqtt_event_t *event) {
	xSemaphoreTake(config_lock, portMAX_DELAY);

	switch(event->event_id) {
	case MQTT_EVENT_CONNECTED:
		mqtt_connected = true;

		if(!event->session_present) {
			for(auto s : subscriptions)
				s->raw_subscribe();
		}
		if(status_topic != "")
			this->publish_to(status_topic, status_msg.data(), status_msg.length(), true);
		ESP_LOGI(mqtt_tag, "Reconnected and subscribed");
	break;

	case MQTT_EVENT_DISCONNECTED:
		mqtt_connected = false;
		ESP_LOGW(mqtt_tag, "Disconnected from broker!");
	break;

	case MQTT_EVENT_DATA: {
		const std::string dataString = std::string(event->data, event->data_len);
		const std::string topicString = std::string(event->topic, event->topic_len);

		ESP_LOGD(mqtt_tag, "Data for topic %s:", topicString.data());
		ESP_LOG_BUFFER_HEXDUMP(mqtt_tag, dataString.data(), dataString.length(), ESP_LOG_VERBOSE);

		for(auto s : subscriptions)
			s->feed_data({topicString, dataString});
	}
	break;

	default: break;
	}

	xSemaphoreGive(config_lock);
}

void Handler::set_status(const std::string &newStatus) {
	if(status_topic == "")
		return;

	status_msg = newStatus;
	if(mqtt_connected)
		this->publish_to(status_topic, status_msg.data(), status_msg.length(), true);
}

void Handler::publish_to(std::string topic, const void *data, size_t length, bool retain, int qos) {
	if(!mqtt_connected) {
		ESP_LOGD(mqtt_tag, "Packet to %s dropped (disconnected)", topic.data());
		return;
	}

	topicsize_string(topic);

	ESP_LOGD(mqtt_tag, "Publishing to %s", topic.data());
	ESP_LOG_BUFFER_HEXDUMP(mqtt_tag, data, length, ESP_LOG_VERBOSE);
	
	esp_mqtt_client_publish(mqtt_handle, topic.data(), reinterpret_cast<const char*>(data)
				, length, qos, retain);
}

void Handler::publish_int(const std::string &topic, int32_t data, bool retain, int qos) {
	char buffer[10] = {};
	sprintf(buffer, "%d", data);

	publish_to(topic, buffer, strlen(buffer), retain, qos);
}

Subscription * Handler::subscribe_to(std::string topic, mqtt_callback cb, int qos) {
	topicsize_string(topic);	

	// NO subscribing necessary here, the subscription class already handles this
	auto nSub = new Subscription(*this, topic, qos);
	nSub->on_received = cb;

	return nSub;
}

uint8_t Handler::is_disconnected() const {
	if(!mqtt_started)
		return 255; // Not started

	if(mqtt_connected)
		return 0;   // Connected
	else
		return 1;   // Started but not connected to MQTT broker
}


} /* namespace MQTT */
} /* namespace Xasin */
