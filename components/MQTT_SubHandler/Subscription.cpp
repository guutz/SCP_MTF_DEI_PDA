/*
 * Subscription.cpp
 *
 *  Created on: 3 Feb 2019
 *      Author: xasin
 */

#include "xasin/mqtt/Subscription.h"

#include "esp_log.h"

namespace Xasin {
namespace MQTT {

Subscription::Subscription(Handler &handler, const std::string topic, int qos)
	:mqtt_handler(handler),
	 topic(topic), qos(qos),
	 on_received(nullptr) {

	xSemaphoreTake(mqtt_handler.config_lock, portMAX_DELAY);
	handler.subscriptions.push_back(this);
	xSemaphoreGive(mqtt_handler.config_lock);

	if(handler.mqtt_connected)
		raw_subscribe();
}

Subscription::~Subscription() {
	xSemaphoreTake(mqtt_handler.config_lock, portMAX_DELAY);

	bool conflictFound = false;

	for(auto s = mqtt_handler.subscriptions.begin(); s < mqtt_handler.subscriptions.end(); ) {
		if((*s) == this)
			mqtt_handler.subscriptions.erase(s);
		else {
			if((*s)->topic == this->topic)
				conflictFound = true;
			s++;
		}
	}

	xSemaphoreGive(mqtt_handler.config_lock);

	if(conflictFound)
		return;

	if(mqtt_handler.mqtt_connected)
		esp_mqtt_client_unsubscribe(mqtt_handler.mqtt_handle, topic.data());
}

void Subscription::raw_subscribe() {
	ESP_LOGI(mqtt_tag, "Subscribing to %s", topic.data());
	esp_mqtt_client_subscribe(mqtt_handler.mqtt_handle, topic.data(), qos);
}

void Subscription::feed_data(MQTT_Packet data) { // data.topic here is the full topic from MQTT event
	if(on_received == nullptr) return;
	
	// Store the original full topic before it's potentially modified or shadowed
	std::string original_full_topic = data.topic;

	if(data.topic.length() < topic.length()) return; // topic is the subscription filter

	std::string topicRest; // This will be the part of the topic AFTER the filter match

	int i=0;
	while(true) {
		if(topic.at(i) == '#') { // Wildcard #
			topicRest = std::string(original_full_topic.data() + i, original_full_topic.length() - i);
			break;
		}
		// Note: This logic doesn't handle '+' wildcard explicitly, assumes exact segment match or '#'
		if(topic.at(i) != original_full_topic.at(i)) return; // No match

		i++;

		if(i == topic.length()) { // Reached end of subscription filter
			if(i == original_full_topic.length()) { // Exact match
				topicRest = ""; // No "rest"
			} else if (original_full_topic.at(i) == '/') { // Matched a prefix, like "foo/bar" to "foo/bar/baz"
                 topicRest = std::string(original_full_topic.data() + i +1, original_full_topic.length() -i -1);
            }
            else { // Filter is "foo/bar" but message is "foo/barbaz" - not a valid match by segments
                return;
            }
			break;
		}
		if (i == original_full_topic.length()) { // Reached end of message topic but not filter (e.g. filter "a/b/c", message "a/b")
			return; // No match
		}
	}

	ESP_LOGV(mqtt_tag, "Topic %s matched! (Full: %s, Rest:%s)", topic.data(), original_full_topic.c_str(), topicRest.c_str());
	// Pass topicRest, original data, and the original_full_topic
	on_received({topicRest, data.data, original_full_topic});
}

} /* namespace MQTT */
} /* namespace Xasin */
