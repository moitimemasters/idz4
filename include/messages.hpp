#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <variant>

struct AckMessage {
    std::string client_id;
    uint32_t seq_num;
};
struct NackMessage {
    std::string client_id;
    uint32_t seq_num;
};
struct DataMessage {
    std::string client_id;
    uint32_t seq_num;
    uint32_t total_segments;
    uint32_t checksum;
    std::string payload;
};
struct InitRequest {};
struct InitResponse {
    std::string client_id;
};
struct HandshakeMessage {
    std::string client_id;
};
struct HandshakeCompleteMessage {};

using ParsedMessage =
    std::variant<AckMessage, NackMessage, DataMessage, InitRequest,
                 InitResponse, HandshakeMessage, HandshakeCompleteMessage>;

#endif  // MESSAGES_H
