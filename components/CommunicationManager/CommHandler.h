// CommHandler.h
#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory> // For std::unique_ptr or std::shared_ptr

namespace Xasin {
namespace Communication {

// Communication mode
enum class CommMode {
    WIFI_MQTT_DIRECT,
    MESH_NETWORK
};

// Structure for received messages
struct CommReceivedData {
    std::string topic; // For MQTT, or a logical topic/group in mesh
    std::vector<uint8_t> payload;
    std::string source_id; // MAC address or mesh node ID of the sender
};

// Callback for received messages
using comm_message_callback_t = std::function<void(const CommReceivedData& message)>;

class CommHandler {
public:
    virtual ~CommHandler() = default;

    // Initializes and starts the communication handler
    // Config might be a struct or a more specific type depending on implementation
    virtual bool start(void* config = nullptr) = 0;

    // Stops the communication handler
    virtual void stop() = 0;

    // Checks if the handler is connected (e.g., to WiFi/MQTT broker or mesh network)
    virtual bool isConnected() const = 0;

    // Publishes a message
    // For MQTT: topic, data, length, retain, qos
    // For Mesh: could be broadcast or targeted. Topic might map to mesh group or message type.
    virtual bool publish(const std::string& topic, const void* data, size_t length, bool retain = false, int qos = 0) = 0;

    // Subscribes to a topic or message type
    // The handler will invoke the callback when a matching message is received.
    virtual bool subscribe(const std::string& topic, comm_message_callback_t callback, int qos = 0) = 0;

    // Unsubscribes from a topic
    virtual bool unsubscribe(const std::string& topic) = 0;

    // Allows the handler to perform periodic tasks (e.g., polling, event processing)
    virtual void update() = 0;

    // Returns the MAC address of the device, useful for mesh node identification
    virtual std::string getDeviceId() = 0;
};

} // namespace Communication
} // namespace Xasin
