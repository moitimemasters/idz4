#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

class ClientNotFoundException : public std::runtime_error {
   public:
    explicit ClientNotFoundException(const std::string& id)
        : std::runtime_error("Client not found: " + id) {}
};

class IncompleteMessageException : public std::runtime_error {
   public:
    explicit IncompleteMessageException()
        : std::runtime_error("Incomplete message received.") {}
};

#endif  // EXCEPTIONS_H
