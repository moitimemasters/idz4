#include "socket_manager.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

SocketManager::SocketManager() : socket_fd(-1), port(0) {
    std::memset(&server_addr, 0, sizeof(server_addr));
}

SocketManager::~SocketManager() {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

void SocketManager::initSocket(uint16_t port) {
    this->port = port;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        throw std::runtime_error("Error creating socket.");
    }

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
}

void SocketManager::bindSocket() {
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        close(socket_fd);
        throw std::runtime_error("Error binding socket.");
    }
}

void SocketManager::sendMessage(const std::string& message,
                                const sockaddr_in& client_addr) {
    sendto(socket_fd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&client_addr, sizeof(client_addr));
}

int SocketManager::receiveMessage(char* buffer, size_t buffer_size,
                                  sockaddr_in& client_addr) const {
    socklen_t len = sizeof(client_addr);
    return recvfrom(socket_fd, buffer, buffer_size, 0,
                    (struct sockaddr*)&client_addr, &len);
}

int SocketManager::getSocketFD() const { return socket_fd; }
