#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <netinet/in.h>

#include <string>

class SocketManager {
   public:
    SocketManager();
    ~SocketManager();

    void initSocket(uint16_t port);
    void bindSocket();
    void sendMessage(const std::string &message,
                     const sockaddr_in &client_addr);
    int receiveMessage(char *buffer, size_t buffer_size,
                       sockaddr_in &client_addr) const;
    int getSocketFD() const;

   private:
    int socket_fd;
    uint16_t port;
    struct sockaddr_in server_addr;
};

#endif  // SOCKET_MANAGER_H
