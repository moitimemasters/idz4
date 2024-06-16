#include "message_parser.hpp"

#include <regex>

uint32_t computeChecksum(const std::string& data) {
    uint32_t checksum = 0;
    for (char c : data) {
        checksum += static_cast<uint8_t>(c);
    }
    return checksum;
}

std::optional<ParsedMessage> MessageParser::parseMessage(
    const std::string& message) {
    for (const auto& [type, parser] : parsers) {
        if (auto result = parser(message)) {
            return result;
        }
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseAckMessage(const std::string& message) {
    std::regex ack_regex(R"(ACK:\s+(\w+)\s+SEQ:\s+(\d+))");
    std::smatch match;
    if (std::regex_match(message, match, ack_regex) && match.size() == 3) {
        AckMessage ack;
        ack.client_id = match[1].str();
        ack.seq_num = std::stoul(match[2].str());
        return ParsedMessage{ack};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseNackMessage(const std::string& message) {
    std::regex nack_regex(R"(NACK:\s+(\w+)\s+SEQ:\s+(\d+))");
    std::smatch match;
    if (std::regex_match(message, match, nack_regex) && match.size() == 3) {
        NackMessage nack;
        nack.client_id = match[1].str();
        nack.seq_num = std::stoul(match[2].str());
        return ParsedMessage{nack};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseDataMessage(const std::string& message) {
    std::regex data_regex(
        R"(ID:(\w+);SEQ:(\d+);TOT:(\d+);CS:(\d+);DATA:([\S\s]*))");
    std::smatch match;
    if (std::regex_match(message, match, data_regex) && match.size() == 6) {
        DataMessage data;
        data.client_id = match[1].str();
        data.seq_num =

            std::stoul(match[2].str());
        data.total_segments = std::stoul(match[3].str());
        data.checksum = std::stoul(match[4].str());
        data.payload = match[5].str();
        return ParsedMessage{data};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseInitRequestMessage(
    const std::string& message) {
    if (message == "INIT_REQUEST") {
        return ParsedMessage{InitRequest{}};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseInitResponseMessage(
    const std::string& message) {
    std::regex init_response_regex(R"(INIT_RESPONSE:\s+(\w+))");
    std::smatch match;
    if (std::regex_match(message, match, init_response_regex) &&
        match.size() == 2) {
        InitResponse init_response;
        init_response.client_id = match[1].str();
        return ParsedMessage{init_response};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseHandshakeMessage(const std::string& message) {
    std::regex handshake_regex(R"(HS:\s+(\w+))");
    std::smatch match;
    if (std::regex_match(message, match, handshake_regex) &&
        match.size() == 2) {
        HandshakeMessage handshake;
        handshake.client_id = match[1].str();
        return ParsedMessage{handshake};
    }
    return std::nullopt;
}

std::optional<ParsedMessage> parseHandshakeCompleteMessage(
    const std::string& message) {
    if (message == "HS_COMPLETE") {
        return ParsedMessage{HandshakeCompleteMessage{}};
    }
    return std::nullopt;
}

struct ParserRegistrar {
    ParserRegistrar() {
        auto& parser = MessageParser::instance();
        parser.registerParser("ACK", parseAckMessage);
        parser.registerParser("NACK", parseNackMessage);
        parser.registerParser("DATA", parseDataMessage);
        parser.registerParser("INIT_REQUEST", parseInitRequestMessage);
        parser.registerParser("INIT_RESPONSE", parseInitResponseMessage);
        parser.registerParser("HANDSHAKE", parseHandshakeMessage);
        parser.registerParser("HANDSHAKE_COMPLETE",
                              parseHandshakeCompleteMessage);
    }
};

static ParserRegistrar registrar;
