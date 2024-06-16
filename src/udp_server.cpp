#include "udp_server.hpp"

#include <netinet/in.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "debug_logs.hpp"
#include "exceptions.hpp"
#include "message_parser.hpp"
#include "messages.hpp"

UDPServer::UDPServer(uint16_t port) : running(false) {
    socketManager.initSocket(port);
    socketManager.bindSocket();
    connectionManager.setMessageHandler(this);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Could not create epoll_fd");
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socketManager.getSocketFD();
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketManager.getSocketFD(), &ev) ==
        -1) {
        throw std::runtime_error("Could not add socket to epoll");
    }
}

UDPServer::~UDPServer() {
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
}

void UDPServer::start() {
    running = true;
    std::cout << "Server is running" << std::endl;

    while (running) {
        epoll_event events[1];
        int nfds = epoll_wait(epoll_fd, events, 1, -1);
        if (nfds == -1) {
            throw std::runtime_error("Could not wait for events");
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].events & EPOLLIN) {
                char buffer[1024];
                struct sockaddr_in client_addr;
                int len = socketManager.receiveMessage(buffer, sizeof(buffer),
                                                       client_addr);
                DEBUG_LOG_BLOCK({
                    std::cout << "Received " << len << " bytes" << std::endl;
                });

                if (len > 0) {
                    buffer[len] = '\0';
                    std::string message(buffer);
                    handleClientMessage(message, client_addr);
                }
            }
        }

        handshakeManager.removeInactiveClients();
        connectionManager.removeInactiveClients();
    }
}

void UDPServer::stop() { running = false; }

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void UDPServer::handleClientMessage(const std::string &message,
                                    struct sockaddr_in &client_addr) {
    auto parsed_message_opt = MessageParser::instance().parseMessage(message);

    if (!parsed_message_opt) {
        std::cerr << "[ERROR] Failed to parse message: " << message
                  << std::endl;
        return;
    }

    ParsedMessage parsed_message = *parsed_message_opt;
    DEBUG_LOG_BLOCK({ std::cout << "parsing message\n"; });
    std::visit(
        overloaded{
            [&](const AckMessage &ack) { handleAckMessage(ack, client_addr); },
            [&](const NackMessage &nack) {
                handleNackMessage(nack, client_addr);
            },
            [&](const DataMessage &data) {
                handleDataMessage(data, client_addr);
            },
            [&](const InitRequest &init_request) {
                DEBUG_LOG_BLOCK({ std::cout << "init request\n"; });
                handleInitRequest(init_request, client_addr);
            },
            [&](const InitResponse &init_response) {
                handleInitResponse(init_response, client_addr);
            },
            [&](const HandshakeMessage &handshake) {
                handleHandshakeMessage(handshake.client_id, client_addr);
            },
            [&](const HandshakeCompleteMessage) {

            },
        },
        parsed_message);
}

void UDPServer::handleAckMessage(const AckMessage &ack,
                                 struct sockaddr_in &client_addr) {
    DEBUG_LOG_BLOCK({
        std::cout << "Received ACK: client_id=" << ack.client_id
                  << " seq_num=" << ack.seq_num << std::endl;
    });
}

void UDPServer::handleNackMessage(const NackMessage &nack,
                                  struct sockaddr_in &client_addr) {
    DEBUG_LOG_BLOCK({
        std::cout << "Received NACK: client_id=" << nack.client_id
                  << " seq_num=" << nack.seq_num << std::endl;
    });
}

void UDPServer::handleDataMessage(const DataMessage &data,
                                  struct sockaddr_in &client_addr) {
    if (!handshakeManager.isClientKnown(data.client_id) ||
        !handshakeManager.isHandshakeComplete(data.client_id)) {
        sendHandshakeToClient(data.client_id, client_addr);
        return;
    }

    uint32_t computed_checksum = computeChecksum(data.payload);
    if (computed_checksum != data.checksum) {
        sendNackToClient(data.client_id, data.seq_num, client_addr);
        return;
    }

    DEBUG_LOG_BLOCK({
        std::cout << "Received DATA: client_id=" << data.client_id
                  << " seq_num=" << data.seq_num
                  << " total_segments=" << data.total_segments
                  << " checksum=" << data.checksum
                  << " payload=" << data.payload << std::endl;
    });

    sendAckToClient(data.client_id, data.seq_num, client_addr);
    connectionManager.trackSegment(data.client_id, data.seq_num,
                                   data.total_segments, data.payload);

    std::string complete_message;
    try {
        connectionManager.assembleMessage(data.client_id, complete_message);
    } catch (IncompleteMessageException) {
        return;
    }
    connectionManager.getMessageHandler()->handleMessage(
        data.client_id, client_addr, complete_message);
}

void UDPServer::sendInitResponseToClient(const std::string &client_id,
                                         struct sockaddr_in &client_addr) {
    std::string message = "HS: " + client_id;
    socketManager.sendMessage(message, client_addr);
    DEBUG_LOG_BLOCK({ std::cout << "sent message: " << message << std::endl; });
}

void UDPServer::handleInitRequest(const InitRequest &,
                                  struct sockaddr_in &client_addr) {
    std::string client_id = generateClientId();
    handshakeManager.startHandshake(client_id);
    sendInitResponseToClient(client_id, client_addr);
}

void UDPServer::handleInitResponse(const InitResponse &init_response,
                                   struct sockaddr_in &client_addr) {
    DEBUG_LOG_BLOCK({
        std::cout << "Received INIT_RESPONSE: client_id="
                  << init_response.client_id << std::endl;
    });
}

void UDPServer::handleHandshakeMessage(const std::string &client_id,
                                       struct sockaddr_in &client_addr) {
    if (handshakeManager.isClientKnown(client_id)) {
        handshakeManager.completeHandshake(client_id);
        connectionManager.registerClient(client_id);
        sendHandshakeCompleteToClient(client_id, client_addr);
    } else {
        handshakeManager.startHandshake(client_id);
        sendHandshakeToClient(client_id, client_addr);
    }
}

void UDPServer::sendAckToClient(const std::string &client_id, uint32_t seq_num,
                                struct sockaddr_in &client_addr) {
    std::string response =
        "ACK: " + client_id + " SEQ: " + std::to_string(seq_num);
    socketManager.sendMessage(response, client_addr);
}

void UDPServer::sendNackToClient(const std::string &client_id, uint32_t seq_num,
                                 struct sockaddr_in &client_addr) {
    std::string response =
        "NACK: " + client_id + " SEQ: " + std::to_string(seq_num);
    socketManager.sendMessage(response, client_addr);
}

void UDPServer::sendHandshakeToClient(const std::string &client_id,
                                      struct sockaddr_in &client_addr) {
    std::string response = "HS: " + client_id;
    socketManager.sendMessage(response, client_addr);
}

void UDPServer::sendHandshakeCompleteToClient(const std::string &client_id,
                                              struct sockaddr_in &client_addr) {
    std::string response = "HS_COMPLETE: " + client_id;
    socketManager.sendMessage(response, client_addr);
}

std::string UDPServer::generateClientId() {
    static std::size_t id = 0;
    return "client_" + std::to_string(id++);
}

inline std::vector<std::string> segmentMessage(const std::string &message,
                                               const std::string &client_id) {
    if (message.empty()) {
        return std::vector<std::string>{"ID:" + client_id +
                                        ";SEQ:0;TOT:1;CS:0;DATA:"};
    }
    uint32_t seq_num = 0;
    const size_t max_segment_size = 512;
    const size_t total_segments =
        (message.size() + max_segment_size - 1) / max_segment_size;
    std::vector<std::string> segments;

    for (size_t i = 0; i < total_segments; ++i) {
        const size_t start = i * max_segment_size;
        const size_t end = std::min(start + max_segment_size, message.size());
        const std::string segment_data = message.substr(start, end - start);
        const std::string segment =
            "ID:" + client_id + ";SEQ:" + std::to_string(seq_num++) +
            ";TOT:" + std::to_string(total_segments) +
            ";CS:" + std::to_string(computeChecksum(segment_data)) +
            ";DATA:" + segment_data;
        segments.push_back(segment);
    }

    return segments;
}

void UDPServer::sendMessage(const std::string &client_id,
                            struct sockaddr_in &addr,
                            const std::string &message) {
    DEBUG_LOG_BLOCK({
        if (message == "ERR") {
            std::cerr << "sending ERR message to " << client_id << std::endl;
        }
    });
    std::cout << "[INFO] sending {" << message << "} to " << client_id
              << std::endl;
    auto segments = segmentMessage(message, client_id);

    for (const auto &segment : segments) {
        socketManager.sendMessage(segment, addr);
    }
}
