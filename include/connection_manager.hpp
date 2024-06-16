#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <ctime>
#include <queue>
#include <string>
#include <unordered_map>

#include "message_dispatcher.hpp"

class ConnectionManager {
   public:
    ConnectionManager();
    void registerClient(const std::string& client_id);
    void trackSegment(const std::string& client_id, uint32_t seq_num,
                      uint32_t total_segments, const std::string& payload);
    void assembleMessage(const std::string& client_id,
                         std::string& complete_message);
    void removeInactiveClients();
    void setMessageHandler(MessageDispatcher* handler);
    MessageDispatcher* getMessageHandler() const;

   private:
    struct ClientState {
        std::unordered_map<uint32_t, std::string> segments;
        uint32_t total_segments;
        std::time_t last_active;
    };

    std::unordered_map<std::string, ClientState> clients;
    std::priority_queue<std::pair<std::time_t, std::string>,
                        std::vector<std::pair<std::time_t, std::string>>,
                        std::greater<>>
        inactiveClients;
    MessageDispatcher* messageHandler;

    static constexpr int INACTIVITY_TIMEOUT = 300;

    void updateClientActivity(const std::string& client_id);
    ClientState& getClientState(const std::string& client_id);
};

#endif  // CONNECTION_MANAGER_H
