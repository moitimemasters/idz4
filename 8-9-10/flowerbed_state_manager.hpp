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
          last_flowerbed_ping(std::chrono::system_clock::now()) {}

    bool setFlowerbedConnected(bool connected) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (flowerbed_connected) {
            last_flowerbed_ping = std::chrono::system_clock::now();
            return false;
        }
        last_flowerbed_ping = std::chrono::system_clock::now();
        flowerbed_connected = connected;
        return true;
    }

    bool addGardener(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gardener_ids.find(client_id) != gardener_ids.end()) {
            gardener_timestamps[client_id] = std::chrono::system_clock::now();
            return true;
        }
        if (gardener_ids.size() >= 2) {
            return false;
        }
        gardener_ids.insert(client_id);
        gardener_timestamps[client_id] = std::chrono::system_clock::now();
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
        gardener_timestamps[client_id] = std::chrono::system_clock::now();
    }

    void updateFlowerbedTimestamp() {
        std::lock_guard<std::mutex> lock(mutex_);
        last_flowerbed_ping = std::chrono::system_clock::now();
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
        auto now = std::chrono::system_clock::now();
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

    void updateNonitorConnection(std::string monitorClientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_monitors[monitorClientId] = std::chrono::system_clock::now();
        auto deadline =
            std::chrono::system_clock::now() - std::chrono::seconds(10);
        std::erase_if(connected_monitors,
                      [&](const auto& kv) { return kv.second < deadline; });
    }

    std::string getMonitorData() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string info = "";
        info += "Gardeners connected: " + std::to_string(gardener_count) + "\n";
        info += "Flowerbed connected: " + std::to_string(flowerbed_connected) +
                "\n";

        info += "Flowers to water: ";
        for (auto flower : flowersToWater) {
            info += std::to_string(flower) + " ";
        }
        info += "\n";

        info += "Updates from gardeners (watered flowers):";
        if (updates.empty()) {
            info += "None";
        } else {
            info += "\n\t";
            for (auto update : updates) {
                info += std::to_string(update.first) + " ";
            }
        }

        info += "\n";

        info += "Connected monitors: ";
        if (connected_monitors.empty()) {
            info += "None\n";
        } else {
            info += "\n\t";
            for (const auto& [clientId, timestamp] : connected_monitors) {
                auto timet = std::chrono::system_clock::to_time_t(timestamp);
                auto tm = std::localtime(&timet);
                char buffer[32] = {0};
                std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm);
                info += clientId + "\t last update: " + buffer + "\n\t";
            }
        }

        return info;
    }

   private:
    int gardener_count;
    bool flowerbed_connected;
    std::unordered_set<std::string> gardener_ids;
    std::unordered_map<std::string, std::chrono::system_clock::time_point>
        gardener_timestamps;
    std::chrono::system_clock::time_point last_flowerbed_ping;
    std::mutex mutex_;
    std::vector<std::pair<size_t, int>> updates;
    std::unordered_map<std::string, std::chrono::system_clock::time_point>
        connected_monitors;
    std::unordered_set<size_t> flowersToWater;
};

#endif  // FLOWERBEDSTATEMANAGER_HPP
