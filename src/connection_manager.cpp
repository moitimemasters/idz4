#include "connection_manager.hpp"

#include "exceptions.hpp"

ConnectionManager::ConnectionManager() : messageHandler(nullptr) {}

void ConnectionManager::registerClient(const std::string& client_id) {
    try {
        ClientState& client_state = getClientState(client_id);
        client_state.total_segments = 0;
        client_state.segments.clear();
    } catch (const ClientNotFoundException&) {
        ClientState new_client_state;
        new_client_state.total_segments = 0;
        new_client_state.segments.clear();
        clients[client_id] = new_client_state;
    }
    updateClientActivity(client_id);
}

void ConnectionManager::trackSegment(const std::string& client_id,
                                     uint32_t seq_num, uint32_t total_segments,
                                     const std::string& payload) {
    ClientState& client_state = getClientState(client_id);
    client_state.segments[seq_num] = payload;
    client_state.total_segments = total_segments;
    updateClientActivity(client_id);
}

void ConnectionManager::assembleMessage(const std::string& client_id,
                                        std::string& complete_message) {
    ClientState& client_state = getClientState(client_id);
    if (client_state.segments.size() != client_state.total_segments) {
        throw IncompleteMessageException();
    }

    complete_message.clear();
    for (uint32_t i = 0; i < client_state.total_segments; ++i) {
        if (client_state.segments.find(i) == client_state.segments.end()) {
            throw IncompleteMessageException();
        }
        complete_message += client_state.segments[i];
    }

    client_state.segments.clear();
}

void ConnectionManager::removeInactiveClients() {
    auto now = std::time(nullptr);
    while (!inactiveClients.empty() &&
           now - inactiveClients.top().first > INACTIVITY_TIMEOUT) {
        const auto& client_id = inactiveClients.top().second;
        auto it = clients.find(client_id);
        if (it != clients.end() &&
            now - it->second.last_active > INACTIVITY_TIMEOUT) {
            clients.erase(it);
        }
        inactiveClients.pop();
    }
}

void ConnectionManager::setMessageHandler(MessageDispatcher* handler) {
    this->messageHandler = handler;
}

MessageDispatcher* ConnectionManager::getMessageHandler() const {
    return messageHandler;
}

void ConnectionManager::updateClientActivity(const std::string& client_id) {
    auto& client_state = clients[client_id];
    client_state.last_active = std::time(nullptr);

    inactiveClients.push(std::make_pair(client_state.last_active, client_id));
}

ConnectionManager::ClientState& ConnectionManager::getClientState(
    const std::string& client_id) {
    auto it = clients.find(client_id);
    if (it == clients.end()) {
        throw ClientNotFoundException(client_id);
    }
    return it->second;
}
