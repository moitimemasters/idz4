#ifndef UDP_SERVER_HPP
#define UDP_SERVER_HPP

#include <netinet/in.h>
#include <sys/epoll.h>

#include <string>

#include "connection_manager.hpp"
#include "handshake_manager.hpp"
#include "message_dispatcher.hpp"
#include "messages.hpp"
#include "socket_manager.hpp"

class UDPServer : public MessageDispatcher {
   public:
    UDPServer(uint16_t port);
    ~UDPServer();

    void start();
    void stop();

    void sendMessage(const std::string &client_id,
                     struct sockaddr_in &client_addr,
                     const std::string &message) override;

   private:
    bool running;
    SocketManager socketManager;
    ConnectionManager connectionManager;
    HandshakeManager handshakeManager;
    int epoll_fd;

    void handleClientMessage(const std::string &message,
                             struct sockaddr_in &client_addr);
    void handleAckMessage(const AckMessage &ack,
                          struct sockaddr_in &client_addr);
    void handleNackMessage(const NackMessage &nack,
                           struct sockaddr_in &client_addr);
    void handleDataMessage(const DataMessage &data,
                           struct sockaddr_in &client_addr);
    void handleInitRequest(const InitRequest &init_request,
                           struct sockaddr_in &client_addr);
    void handleInitResponse(const InitResponse &init_response,
                            struct sockaddr_in &client_addr);
    void sendInitResponseToClient(const std::string &client_id,
                                  struct sockaddr_in &client_addr);
    void handleHandshakeMessage(const std::string &client_id,
                                struct sockaddr_in &client_addr);
    void sendAckToClient(const std::string &client_id, uint32_t seq_num,
                         struct sockaddr_in &client_addr);
    void sendNackToClient(const std::string &client_id, uint32_t seq_num,
                          struct sockaddr_in &client_addr);
    void sendHandshakeToClient(const std::string &client_id,
                               struct sockaddr_in &client_addr);
    void sendHandshakeCompleteToClient(const std::string &client_id,
                                       struct sockaddr_in &client_addr);

    std::string generateClientId();
};

#endif  // UDP_SERVER_HPP
