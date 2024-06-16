#include <netinet/in.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>

#include "debug_logs.hpp"
#include "flowerbed_state_manager.hpp"
#include "route_manager.hpp"
#include "udp_server.hpp"

class Server : public UDPServer {
   public:
    Server(uint16_t port) : UDPServer(port) {
        routeManager.registerRoute(
            "/ping/gardener/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &, const std::string &) {
                handleGardenerPing(client_id, addr);
            });
        routeManager.registerRoute(
            "/ping/flowerbed/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &, const std::string &) {
                handleFlowerbedPing(client_id, addr);
            });
        routeManager.registerRoute(
            "/getUpdates/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &,
                   const std::string &) { handleGetUpdates(client_id, addr); });
        routeManager.registerRoute(
            "/getFlower/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &, const std::string &) {
                handleGardenerRequest(client_id, addr);
            });
        routeManager.registerRoute(
            "/water/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &, const std::string &payload) {
                handleGardenerWatered(client_id, addr, payload);
            });
        routeManager.registerRoute(
            "/toWater/",
            [this](const std::string &client_id, struct sockaddr_in &addr,
                   const std::string &, const std::string &payload) {
                handleFlowerbedNewFlowers(client_id, addr, payload);
            });

        worker_thread = std::jthread([this](std::stop_token stop_token) {
            while (!stop_token.stop_requested()) {
                stateManager.checkConnections();

                if (!stateManager.isReady()) {
                    std::cout << "Waiting for all clients to be connected..."
                              << std::endl;
                } else {
                    std::cout << "All clients are connected. Ready to operate."
                              << std::endl;
                }

                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        });
    }
    void handleMessage(const std::string &client_id,
                       struct sockaddr_in &client_addr,
                       const std::string &message) override {
        std::cout << "[INFO] Recieved { " << message << " } from " << client_id
                  << std::endl;
        routeManager.handleRoute(client_id, client_addr, message);
    }

   private:
    RouteManager routeManager;
    FlowerBedStateManager stateManager;
    std::jthread worker_thread;

    void handleGardenerPing(const std::string &client_id,
                            struct sockaddr_in &addr) {
        if (stateManager.addGardener(client_id)) {
            sendMessage(client_id, addr, "OK");
            return;
        }
        std::cerr << "Gardener already connected: " << client_id << std::endl;
        sendMessage(client_id, addr, "ERR");
    }

    void handleFlowerbedPing(const std::string &client_id,
                             struct sockaddr_in &addr) {
        if (stateManager.setFlowerbedConnected(true)) {
            sendMessage(client_id, addr, "OK");
            return;
        }
        sendMessage(
            client_id, addr,
            "HC");  // has connection already, handle ownership of flowerbed on
                    // client side -- client must receive OK once
    }

    void handleGetUpdates(const std::string &client_id,
                          struct sockaddr_in &addr) {
        if (!stateManager.isReady()) {
            sendMessage(client_id, addr, "NOT_READY");
            return;
        }
        auto updates = stateManager.getUpdates();
        // join updates like flowerIndex:flowerState;...;
        std::string answer = "";
        for (const auto &[flowerIndex, flowerState] : updates) {
            answer += std::to_string(flowerIndex) + ":" +
                      std::to_string(flowerState) + ";";
        }
        stateManager.clearUpdates();
        sendMessage(client_id, addr, answer);
    }

    void handleGardenerRequest(const std::string &client_id,
                               struct sockaddr_in &addr) {
        if (!stateManager.isReady()) {
            sendMessage(client_id, addr, "NOT_READY");
            return;
        }

        int flowerIndex = stateManager.getFlowerToWater();
        sendMessage(client_id, addr, std::to_string(flowerIndex));
    }

    void handleGardenerWatered(const std::string &client_id,
                               struct sockaddr_in &addr,
                               const std::string &payload) {
        if (!stateManager.isReady()) {
            sendMessage(client_id, addr, "NOT_READY");
            return;
        }

        int flowerIndex = std::stoi(payload);
        if (flowerIndex < 0) {
            sendMessage(client_id, addr, "ERR");
            return;
        }
        stateManager.addUpdate(flowerIndex, 1);
        sendMessage(client_id, addr, "OK");
    }

    void handleFlowerbedNewFlowers(const std::string &client_id,
                                   struct sockaddr_in &addr,
                                   const std::string &payload) {
        if (!stateManager.isReady()) {
            sendMessage(client_id, addr, "NOT_READY");
            return;
        }
        // payload is flowerIndex;...;
        std::unordered_set<size_t> newFlowers;
        std::stringstream ss{payload};
        std::string item;
        while (std::getline(ss, item, ';')) {
            newFlowers.insert(std::stoi(item));
        }
        stateManager.addFlowersToWater(newFlowers);
        sendMessage(client_id, addr, "OK");
    }
};

int main() {
    Server server(12345);
    server.start();
}
