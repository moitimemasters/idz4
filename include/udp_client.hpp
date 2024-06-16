#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP

#include <netinet/in.h>
#include <sys/epoll.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "messages.hpp"

class TimeOutException : public std::runtime_error {
   public:
    TimeOutException(const std::string &message)
        : std::runtime_error(message) {}
};

class UDPClient {
   public:
    UDPClient(const std::string &server_address, uint16_t server_port);
    ~UDPClient();

    std::optional<std::string> sendMessage(const std::string &message,
                                           int timeout = 5);

   private:
    int sockfd;
    int epoll_fd;
    struct sockaddr_in server_addr;
    std::string client_id;
    uint32_t seq_num;

    void initSocket();
    bool performHandshake();
    bool sendAndReceiveAck(const std::string &message, int timeout);
    std::optional<std::string> receiveMessage(int timeout);

    std::vector<std::string> segmentMessage(const std::string &message);
    uint32_t computeChecksum(const std::string &data);

    void sendSegment(const std::string &segment);
    bool awaitAck(const std::string &segment, int timeout);

    void handleAck(const AckMessage &ack);
    void handleNack(const NackMessage &nack);
    void handleHandshake(const HandshakeMessage &hs);
    void handleHandshakeComplete(const HandshakeCompleteMessage &hsc);
    std::optional<std::string> receiveResponse(int timeout);

    void setupEpoll();
    void handleEpollEvents();

    bool handleEpollEvents(const std::string &current_segment, int timeout);
};

#endif  // UDP_CLIENT_HPP
