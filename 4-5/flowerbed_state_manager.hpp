#ifndef FLOWERBEDSTATEMANAGER_HPP
#define FLOWERBEDSTATEMANAGER_HPP

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "debug_logs.hpp"

class FlowerBedStateManager {
   public:
    FlowerBedStateManager()
        : gardener_count(0),
          flowerbed_connected(false),
          last_flowerbed_ping(std::chrono::steady_clock::now()) {}

    bool setFlowerbedConnected(bool connected) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (flowerbed_connected) {
            last_flowerbed_ping = std::chrono::steady_clock::now();
            return false;
        }
        last_flowerbed_ping = std::chrono::steady_clock::now();
        flowerbed_connected = connected;
        return true;
    }

    bool addGardener(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gardener_ids.find(client_id) != gardener_ids.end()) {
            gardener_timestamps[client_id] = std::chrono::steady_clock::now();
            return true;
        }
        if (gardener_ids.size() >= 2) {
            return false;
        }
        gardener_ids.insert(client_id);
        gardener_timestamps[client_id] = std::chrono::steady_clock::now();
        gardener_count = gardener_ids.size();
        return true;
    }

    void removeGardener(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gardener_ids.find(client_id) == gardener_ids.end()) {
            return;
        }
        gardener_ids.erase(client_id);
        gardener_timestamps.erase(client_id);
        gardener_count = gardener_ids.size();
    }

    void updateGardenerTimestamp(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        gardener_timestamps[client_id] = std::chrono::steady_clock::now();
    }

    void updateFlowerbedTimestamp() {
        std::lock_guard<std::mutex> lock(mutex_);
        last_flowerbed_ping = std::chrono::steady_clock::now();
    }

    bool isReady() {
        std::lock_guard<std::mutex> lock(mutex_);
        DEBUG_LOG_BLOCK({
            std::cout << "flowerbed_connected: " << flowerbed_connected
                      << std::endl;
            std::cout << "gardener_count: " << gardener_count << std::endl;
        });
        return flowerbed_connected && gardener_count >= 2;
    }

    void addUpdate(size_t flowerIndex, int flowerState) {
        std::lock_guard<std::mutex> lock(mutex_);
        updates.emplace_back(flowerIndex, flowerState);
    }

    std::vector<std::pair<size_t, int>> getUpdates() {
        std::lock_guard<std::mutex> lock(mutex_);
        return updates;
    }

    void clearUpdates() {
        std::lock_guard<std::mutex> lock(mutex_);
        updates.clear();
    }

    void checkConnections() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(10);

        std::erase_if(gardener_ids, [&](const std::string& id) {
            return now - gardener_timestamps[id] > timeout;
        });

        std::erase_if(gardener_timestamps, [&](const auto& kv) {
            return now - kv.second > timeout;
        });

        gardener_count = gardener_ids.size();

        if (now - last_flowerbed_ping > timeout) {
            flowerbed_connected = false;
        }
    }

    int getFlowerToWater() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (flowersToWater.empty()) {
            return -1;
        }
        size_t flowerIndex = *flowersToWater.begin();
        flowersToWater.erase(flowerIndex);
        return flowerIndex;
    }

    void addFlowersToWater(std::unordered_set<size_t> newFlowersToWater) {
        std::lock_guard<std::mutex> lock(mutex_);
        flowersToWater.insert(newFlowersToWater.begin(),
                              newFlowersToWater.end());
    }

   private:
    int gardener_count;
    bool flowerbed_connected;
    std::unordered_set<std::string> gardener_ids;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        gardener_timestamps;
    std::chrono::steady_clock::time_point last_flowerbed_ping;
    std::mutex mutex_;
    std::vector<std::pair<size_t, int>> updates;
    std::unordered_set<size_t> flowersToWater;
};

#endif  // FLOWERBEDSTATEMANAGER_HPP
