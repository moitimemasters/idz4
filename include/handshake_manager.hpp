#ifndef HANDSHAKE_MANAGER_HPP
#define HANDSHAKE_MANAGER_HPP

#include <ctime>
#include <string>
#include <unordered_map>

class HandshakeManager {
   public:
    void startHandshake(const std::string &client_id);
    bool isHandshakeComplete(const std::string &client_id);
    void completeHandshake(const std::string &client_id);
    bool isClientKnown(const std::string &client_id);
    void removeInactiveClients();

   private:
    struct HandshakeState {
        bool handshake_complete;
        std::time_t last_active;
    };

    std::unordered_map<std::string, HandshakeState> handshakes;
    static constexpr int HANDSHAKE_TIMEOUT = 60;  // in seconds
};

#endif  // HANDSHAKE_MANAGER_HPP
