\
#include "CommunicationManager.h"
#include "esp_log.h"

namespace lzrtag {

static const char *TAG = "CommunicationManager";

CommunicationManager::CommunicationManager(CommHandler* handler)
    : comm_handler_(handler) {
    if (!handler) {
        ESP_LOGE(TAG, "CommHandler cannot be null!");
        // Depending on error policy, might throw or set an internal error state
    }
}

CommunicationManager::~CommunicationManager() {
    if (comm_handler_) {
        comm_handler_->stop(); // Ensure handler is stopped before destruction
    }
}

bool CommunicationManager::start() {
    if (comm_handler_) {
        ESP_LOGI(TAG, "Starting communication handler...");
        return comm_handler_->start();
    }
    ESP_LOGE(TAG, "No communication handler set.");
    return false;
}

void CommunicationManager::stop() {
    if (comm_handler_) {
        ESP_LOGI(TAG, "Stopping communication handler...");
        comm_handler_->stop();
    }
}

bool CommunicationManager::isConnected() const {
    if (comm_handler_) {
        return comm_handler_->isConnected();
    }
    return false;
}

bool CommunicationManager::publish(const std::string& topic, const std::string& payload) {
    if (comm_handler_) {
        // ESP_LOGD(TAG, "Publishing to topic '%s'", topic.c_str());
        return comm_handler_->publish(topic, payload);
    }
    ESP_LOGE(TAG, "Cannot publish, no communication handler set.");
    return false;
}

bool CommunicationManager::subscribe(const std::string& topic, 
                                   std::function<void(const std::string&, const std::string&)> callback) {
    if (comm_handler_) {
        ESP_LOGI(TAG, "Subscribing to topic '%s'", topic.c_str());
        return comm_handler_->subscribe(topic, callback);
    }
    ESP_LOGE(TAG, "Cannot subscribe, no communication handler set.");
    return false;
}

bool CommunicationManager::unsubscribe(const std::string& topic) {
    if (comm_handler_) {
        ESP_LOGI(TAG, "Unsubscribing from topic '%s'", topic.c_str());
        return comm_handler_->unsubscribe(topic);
    }
    ESP_LOGE(TAG, "Cannot unsubscribe, no communication handler set.");
    return false;
}

std::string CommunicationManager::getDeviceId() const {
    if (comm_handler_) {
        return comm_handler_->getDeviceId();
    }
    return "";
}

bool CommunicationManager::isRootNode() const {
    if (comm_handler_) {
        return comm_handler_->isRootNode();
    }
    return false; // Default to false if no handler
}

} // namespace lzrtag
