\
#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include <string>
#include <functional>
#include <memory> // For std::unique_ptr

namespace lzrtag {

// Interface for communication handlers (e.g., ESP-MESH, MQTT direct)
class CommHandler {
public:
    virtual ~CommHandler() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const = 0;
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    virtual bool subscribe(const std::string& topic, 
                           std::function<void(const std::string& topic, const std::string& payload)> callback) = 0;
    virtual bool unsubscribe(const std::string& topic) = 0;
    virtual std::string getDeviceId() const = 0;
    virtual bool isRootNode() const = 0; // Indicates if the handler is operating as a root/master node
};

class CommunicationManager {
public:
    // Takes ownership of the handler
    explicit CommunicationManager(CommHandler* handler);
    ~CommunicationManager();

    // Prevent copy and assign
    CommunicationManager(const CommunicationManager&) = delete;
    CommunicationManager& operator=(const CommunicationManager&) = delete;
    // Allow move
    CommunicationManager(CommunicationManager&&) = default;
    CommunicationManager& operator=(CommunicationManager&&) = default;

    bool start();
    void stop();
    bool isConnected() const;
    
    bool publish(const std::string& topic, const std::string& payload);
    bool subscribe(const std::string& topic, 
                   std::function<void(const std::string& topic, const std::string& payload)> callback);
    bool unsubscribe(const std::string& topic);

    std::string getDeviceId() const;
    bool isRootNode() const;

private:
    std::unique_ptr<CommHandler> comm_handler_;
};

} // namespace lzrtag

#endif // COMMUNICATION_MANAGER_H
