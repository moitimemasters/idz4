#ifndef MESSAGE_DISPATCHER_HPP
#define MESSAGE_DISPATCHER_HPP

#include <netinet/in.h>

#include <string>

class MessageDispatcher {
   public:
    virtual ~MessageDispatcher() = default;

    virtual void handleMessage(const std::string& client_id,
                               struct sockaddr_in& client_addr,
                               const std::string& message) = 0;
    virtual void sendMessage(const std::string& client_id,
                             struct sockaddr_in& client_addr,
                             const std::string& message) = 0;
};

#endif  // MESSAGE_DISPATCHER_HPP
