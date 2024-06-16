#include "handshake_manager.hpp"

void HandshakeManager::startHandshake(const std::string &client_id) {
    handshakes[client_id] = {false, std::time(nullptr)};
}

bool HandshakeManager::isHandshakeComplete(const std::string &client_id) {
    return handshakes[client_id].handshake_complete;
}

void HandshakeManager::completeHandshake(const std::string &client_id) {
    handshakes[client_id].handshake_complete = true;
    handshakes[client_id].last_active = std::time(nullptr);
}

bool HandshakeManager::isClientKnown(const std::string &client_id) {
    return handshakes.find(client_id) != handshakes.end();
}

void HandshakeManager::removeInactiveClients() {
    std::time_t now = std::time(nullptr);
    for (auto it = handshakes.begin(); it != handshakes.end();) {
        if (it->second.last_active + HANDSHAKE_TIMEOUT < now) {
            it = handshakes.erase(it);
        } else {
            ++it;
        }
    }
}
