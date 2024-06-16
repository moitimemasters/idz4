#include "udp_client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "debug_logs.hpp"
#include "message_parser.hpp"
#include "messages.hpp"

UDPClient::UDPClient(const std::string &server_address, uint16_t server_port)
    : seq_num(0) {
    initSocket();
    setupEpoll();

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address.c_str(), &server_addr.sin_addr);

    DEBUG_LOG_BLOCK(
        { std::cout << "Server address: " << server_address << std::endl; });

    if (!performHandshake()) {
        throw std::runtime_error("Failed to perform handshake with server");
    }
}

UDPClient::~UDPClient() {
    close(sockfd);
    close(epoll_fd);
}

void UDPClient::initSocket() {
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
}
void UDPClient::setupEpoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl: sockfd");
        exit(EXIT_FAILURE);
    }
}

bool UDPClient::performHandshake() {
    const char *init_request = "INIT_REQUEST";
    if (sendto(sockfd, init_request, strlen(init_request), 0,
               (const struct sockaddr *)&server_addr,
               sizeof(server_addr)) < 0) {
        DEBUG_LOG_BLOCK({ std::cout << "Errono: " << errno << std::endl; });
        throw std::runtime_error("Failed to send INIT_REQUEST");
    }
    std::cout << "Sent INIT_REQUEST" << std::endl;
    auto response = receiveMessage(5);
    if (response.has_value()) {
        auto msg = *response;
        auto parsed_message_opt = MessageParser::instance().parseMessage(msg);
        if (!parsed_message_opt) {
            throw ParseError("Failed to parse message, received frin server: " +
                             msg);
        }
        if (parsed_message_opt &&
            std::holds_alternative<HandshakeMessage>(*parsed_message_opt)) {
            client_id =
                std::get<HandshakeMessage>(*parsed_message_opt).client_id;
            return true;
        }
    }
    return false;
}

std::optional<std::string> UDPClient::sendMessage(const std::string &message,
                                                  int timeout) {
    if (client_id.empty()) {
        if (!performHandshake()) {
            throw std::runtime_error("Failed to perform handshake with server");
        }
    }

    auto segments = segmentMessage(message);

    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (const auto &segment : segments) {
        while (!sendAndReceiveAck(segment, timeout)) {
            DEBUG_LOG_BLOCK(
                { std::cout << "still waiting for ack" << std::endl; });

            if (std::chrono::steady_clock::now() - start >
                std::chrono::seconds(10)) {
                throw TimeOutException("Waiting for ack timed out");
            }
        }
    }

    auto response = receiveResponse(timeout);
    return response;
}

std::vector<std::string> UDPClient::segmentMessage(const std::string &message) {
    seq_num = 0;
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

uint32_t UDPClient::computeChecksum(const std::string &data) {
    uint32_t checksum = 0;
    for (char c : data) {
        checksum += static_cast<uint32_t>(c);
    }
    return checksum;
}

void UDPClient::sendSegment(const std::string &segment) {
    sendto(sockfd, segment.c_str(), segment.size(), 0,
           (const struct sockaddr *)&server_addr, sizeof(server_addr));
}

bool UDPClient::sendAndReceiveAck(const std::string &message, int timeout) {
    sendSegment(message);
    return awaitAck(message, timeout);
}

bool UDPClient::awaitAck(const std::string &segment, int timeout) {
    while (true) {
        epoll_event events[1];
        int nfds = epoll_wait(epoll_fd, events, 1, 1000);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (nfds > 0) {
            if (handleEpollEvents(segment, timeout)) {
                return true;
            }
        } else {
            return false;
        }
    }
}

bool UDPClient::handleEpollEvents(const std::string &current_segment,
                                  int timeout) {
    auto response = receiveMessage(timeout);
    if (response.has_value()) {
        auto parsed_message_opt =
            MessageParser::instance().parseMessage(*response);
        if (parsed_message_opt) {
            if (std::holds_alternative<AckMessage>(*parsed_message_opt)) {
                handleAck(std::get<AckMessage>(*parsed_message_opt));
                return true;
            } else if (std::holds_alternative<NackMessage>(
                           *parsed_message_opt)) {
                handleNack(std::get<NackMessage>(*parsed_message_opt));
                return false;
            } else if (std::holds_alternative<HandshakeMessage>(
                           *parsed_message_opt)) {
                handleHandshake(
                    std::get<HandshakeMessage>(*parsed_message_opt));
                sendSegment(current_segment);
                return false;
            } else if (std::holds_alternative<HandshakeCompleteMessage>(
                           *parsed_message_opt)) {
                handleHandshakeComplete(
                    std::get<HandshakeCompleteMessage>(*parsed_message_opt));
                sendSegment(current_segment);
                return false;
            }
        }
    }
    return false;
}

void UDPClient::handleAck(const AckMessage &ack) {
    DEBUG_LOG_BLOCK({
        std::cout << "Received ACK: client_id=" << ack.client_id
                  << " seq_num=" << ack.seq_num << std::endl;
    });
}

void UDPClient::handleNack(const NackMessage &nack) {
    DEBUG_LOG_BLOCK({
        std::cerr << "Received NACK: client_id=" << nack.client_id
                  << " seq_num=" << nack.seq_num << std::endl;
    });
    throw std::runtime_error("Received NACK from server, segment not accepted");
}

void UDPClient::handleHandshake(const HandshakeMessage &hs) {
    DEBUG_LOG_BLOCK({
        std::cout << "Received Handshake: client_id=" << hs.client_id
                  << std::endl;
    });
    std::string handshake_response = "HS: " + client_id;

    client_id = hs.client_id;
    if (sendto(sockfd, handshake_response.c_str(), handshake_response.length(),
               0, (const struct sockaddr *)&server_addr,
               sizeof(server_addr)) < 0) {
        throw std::runtime_error("Error sending handshake response");
    }
}

void UDPClient::handleHandshakeComplete(const HandshakeCompleteMessage &hsc) {
    std::cout << "Received Handshake Complete" << std::endl;
}

std::optional<std::string> UDPClient::receiveResponse(int timeout) {
    auto message = receiveMessage(timeout);

    DEBUG_LOG_BLOCK({
        std::cout << "[DEBUG] recieved message: " << *message << std::endl;
    });

    if (!message) {
        return std::nullopt;
    }
    std::string response = "";
    std::unordered_map<uint32_t, std::string> segments;
    uint32_t total;
    while (auto parsed_message_opt =
               MessageParser::instance().parseMessage(*message)) {
        if (std::holds_alternative<DataMessage>(*parsed_message_opt)) {
            auto data = std::get<DataMessage>(*parsed_message_opt);
            segments[data.seq_num] = data.payload;
            total = data.total_segments;
        } else {
            break;
        }
        if (segments.size() == total) {
            for (uint32_t i = 0; i < total; i++) {
                response += segments[i];
            }
            break;
        }
        message = receiveMessage(timeout);
    }
    return (response == "") ? std::nullopt : std::make_optional(response);
}

std::optional<std::string> UDPClient::receiveMessage(int timeout) {
    if (timeout > 0) {
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                       sizeof(struct timeval)) < 0) {
            throw std::runtime_error("Error setting timeout");
        }
    }

    char buffer[1024];
    socklen_t len = sizeof(server_addr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                     (struct sockaddr *)&server_addr, &len);
    if (n < 0) {
        if (errno == EAGAIN) {
            throw TimeOutException("Server response timed out");
        }
        throw std::runtime_error(std::string{"Error receiving message: "} +
                                 std::strerror(errno));
    }
    DEBUG_LOG_BLOCK(
        { std::cout << "recieved " << n << " bytes" << std::endl; });
    if (n > 0) {
        buffer[n] = '\0';
        DEBUG_LOG_BLOCK({ std::cout << std::string(buffer) << std::endl; });
        return std::string(buffer);
    }
    return std::nullopt;
}
