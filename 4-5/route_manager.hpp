#ifndef ROUTEMANAGER_HPP
#define ROUTEMANAGER_HPP

#include <netinet/in.h>

#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

class RouteManager {
   public:
    using RouteHandler = std::function<void(
        const std::string &client_id, struct sockaddr_in &addr,
        const std::string &prefix, const std::string &suffix)>;

    void registerRoute(const std::string &prefix, RouteHandler handler) {
        routes_[prefix] = std::move(handler);
    }

    void handleRoute(const std::string &client_id, struct sockaddr_in &addr,
                     const std::string &message) {
        for (const auto &route : routes_) {
            if (message.find(route.first) == 0) {
                std::string prefix = route.first;
                std::string suffix = message.substr(prefix.length());
                route.second(client_id, addr, prefix, suffix);
                return;
            }
        }
        std::cerr << "No route found for message: " << message << std::endl;
    }

   private:
    std::unordered_map<std::string, RouteHandler> routes_;
};

#endif  // ROUTEMANAGER_HPP
