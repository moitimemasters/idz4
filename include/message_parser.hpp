#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "messages.hpp"

uint32_t computeChecksum(const std::string& data);

class ParseError : public std::runtime_error {
   public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

class MessageParser {
   public:
    using ParserFunction =
        std::function<std::optional<ParsedMessage>(const std::string&)>;

    static MessageParser& instance() {
        static MessageParser instance;
        return instance;
    }

    void registerParser(const std::string& type, ParserFunction parser) {
        parsers[type] = parser;
    }

    std::optional<ParsedMessage> parseMessage(const std::string& message);

   private:
    std::unordered_map<std::string, ParserFunction> parsers;
};

#endif  // MESSAGE_PARSER_H
