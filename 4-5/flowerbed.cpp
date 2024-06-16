#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include "debug_logs.hpp"
#include "message_parser.hpp"
#include "udp_client.hpp"

class FlowerBedClient {
   public:
    FlowerBedClient(const std::string& server_ip, uint16_t server_port)
        : client(server_ip, server_port), stop_flag(false) {
        std::srand(std::time(nullptr));
        flower_states.resize(10, 0);
        flower_watering_counts.resize(10, 0);
    }

    static constexpr size_t retryAttempts = 3;
    static constexpr uint32_t getUpdatesTimeout = 10;
    static constexpr uint32_t pingInterval = 2;
    static constexpr uint32_t newFlowersInterval = 30;

    void start() {
        if (!firstPing()) {
            return;
        }

        jthreads.emplace_back(&FlowerBedClient::pingServer, this,
                              std::stop_token());
        jthreads.emplace_back(&FlowerBedClient::updateFlowerStates, this,
                              std::stop_token());

        size_t attempts = 0;

        while (!stop_flag.load() && attempts < retryAttempts) {
            try {
                std::optional<std::string> response;
                {
                    std::lock_guard<std::mutex> lock(socket_mtx);
                    std::cout << "[INFO] Requesting updates on gardeners "
                                 "actions on the flowerbed..."
                              << std::endl;
                    response =
                        client.sendMessage("/getUpdates/", getUpdatesTimeout);
                }

                if (response.has_value()) {
                    if (response.value() == "NOT_READY") {
                        std::cerr << "[WARNING] Server not ready, retrying..."
                                  << std::endl;
                        attempts++;
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    } else {
                        handleServerResponse(response.value());
                        attempts = 0;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            } catch (const TimeOutException& e) {
                std::cerr << "[ERROR] Server is probably down: " << e.what()
                          << std::endl;
            } catch (const ParseError& e) {
                std::cerr << "[ERROR] Server sent invalid message: " << e.what()
                          << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Error occured while fetching updates: "
                          << e.what() << std::endl;
            }
        }
        if (attempts >= retryAttempts) {
            std::cerr << "[ERROR] Exceeded maximum retry attempts. Exiting..."
                      << std::endl;
            stop_flag.store(true);
        }

        stop();
    }

   private:
    UDPClient client;
    std::atomic<bool> stop_flag;
    std::vector<int> flower_states;
    std::vector<int> flower_watering_counts;
    std::mutex mtx;
    std::mutex socket_mtx;
    std::condition_variable cv;
    std::vector<std::jthread> jthreads;

    bool firstPing() {
        try {
            std::optional<std::string> response;
            {
                std::lock_guard<std::mutex> lock(socket_mtx);
                std::cout << "[INFO] Sending first ping..." << std::endl;
                response = client.sendMessage("/ping/flowerbed/",
                                              5);  // Timeout in seconds
            }

            if (response.has_value()) {
                if (response.value() == "OK") {
                    std::cout
                        << "[INFO] First ping successful: " << response.value()
                        << std::endl;
                    return true;
                } else if (response.value() == "HC") {
                    std::cerr << "[WARNING] Another flowerBed client is "
                                 "already connected."
                              << std::endl;
                } else {
                    std::cerr << "[ERROR] Unexpected response to first ping: "
                              << response.value() << std::endl;
                }
            } else {
                std::cerr << "[ERROR] No response received to first ping."
                          << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr
                << "[ERROR] Error occured while doing first ping to server: "
                << e.what() << std::endl;
        }

        stop_flag.store(true);
        return false;
    }

    void pingServer(std::stop_token stop_token) {
        try {
            while (!stop_token.stop_requested()) {
                std::optional<std::string> response;
                {
                    std::lock_guard<std::mutex> lock(socket_mtx);
                    std::cout << "[INFO] Pinging server..." << std::endl;
                    response = client.sendMessage("/ping/flowerbed/", 10);
                }

                if (response.has_value()) {
                    if (response.value() == "OK" || response.value() == "HC") {
                        std::cout
                            << "[INFO] Ping successful: " << response.value()
                            << std::endl;
                    } else {
                        std::cerr << "[ERROR] Unexpected ping response: "
                                  << response.value() << std::endl;
                        stop_flag.store(true);
                        cv.notify_all();
                        break;
                    }
                } else {
                    std::cerr << "[ERROR] No response received to ping."
                              << std::endl;
                    stop_flag.store(true);
                    cv.notify_all();
                    break;
                }

                std::this_thread::sleep_for(std::chrono::seconds(pingInterval));
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERRPR] Error occured while pinging server: "
                      << e.what() << std::endl;
            stop_flag.store(true);
            cv.notify_all();
        }
    }

    void updateFlowerStates(std::stop_token stop_token) {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dis(4, 10);

        try {
            while (!stop_token.stop_requested()) {
                std::vector<int> new_flowers;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    int num_new_flowers = dis(gen);
                    for (int i = 0; i < num_new_flowers; ++i) {
                        int flower_index = std::rand() % 10;
                        new_flowers.push_back(flower_index);
                    }
                }

                std::stringstream ss;
                for (const auto& flower : new_flowers) {
                    ss << flower << ";";
                }

                std::optional<std::string> response;
                {
                    std::lock_guard<std::mutex> lock(socket_mtx);
                    std::cout << "[INFO] Telling server what flowers need to "
                                 "be watered..."
                              << std::endl;
                    DEBUG_LOG_BLOCK(
                        { std::cout << "[DEBUG] " << ss.str() << std::endl; });
                    response = client.sendMessage("/toWater/" + ss.str(), 10);
                }

                if (response.has_value()) {
                    if (response.value() != "OK") {
                        std::cerr << "[ERROR] Unexpected response to pushing "
                                     "unwatered flowers: "
                                  << response.value() << std::endl;
                        stop_flag.store(true);
                        cv.notify_all();
                        break;
                    }
                    std::cout << "[INFO] Server acknowledged new flowers."
                              << std::endl;
                } else {
                    std::cerr << "[ERROR] No response received when sending "
                                 "new flowers."
                              << std::endl;
                    stop_flag.store(true);
                    cv.notify_all();
                    break;
                }

                std::unique_lock<std::mutex> lock(mtx);
                std::this_thread::sleep_for(
                    std::chrono::seconds(newFlowersInterval));
                cv.wait_for(lock, std::chrono::seconds(newFlowersInterval),
                            [this] { return stop_flag.load(); });
            }
        } catch (const std::exception& e) {
            std::cerr
                << "[ERROR] Error occured while pushing flowers to server: "
                << e.what() << std::endl;
            stop_flag.store(true);
            cv.notify_all();
        }
    }

    void handleServerResponse(const std::string& response) {
        DEBUG_LOG_BLOCK({
            std::cout << "[DEBUG] Received response from server: " << response
                      << std::endl;
        });

        std::stringstream ss(response);
        std::string item;
        flower_watering_counts.resize(10, 0);

        while (std::getline(ss, item, ';')) {
            if (!item.empty()) {
                auto pos = item.find(':');
                if (pos != std::string::npos) {
                    int flowerIndex = std::stoi(item.substr(0, pos));
                    int flowerState = std::stoi(item.substr(pos + 1));

                    std::lock_guard<std::mutex> lock(mtx);
                    if (flowerIndex >= 0 && flowerIndex < 10) {
                        flower_states[flowerIndex] = flowerState;
                        flower_watering_counts[flowerIndex]++;

                        if (flower_watering_counts[flowerIndex] > 1) {
                            std::cerr << "[WARNING] Flower " << flowerIndex
                                      << " was watered twice and dies."
                                      << std::endl;
                            std::cerr << "[WARNING] Flowerbed will stop its "
                                         "process..."
                                      << std::endl;
                            stop_flag.store(true);
                            cv.notify_all();
                            break;
                        }

                        if (flowerState == 0) {
                            std::cerr << "[WARNING] Flower " << flowerIndex
                                      << " was not watered in time."
                                      << std::endl;
                            std::cerr << "[WARNING] Flowerbed will stop its "
                                         "process..."
                                      << std::endl;

                            stop_flag.store(true);
                            cv.notify_all();
                            break;
                        }
                    }
                }
            }
        }
    }

    void stop() {
        stop_flag.store(true);
        cv.notify_all();

        for (auto& jth : jthreads) {
            if (jth.joinable()) {
                jth.request_stop();
            }
        }
    }
};

void clientTask(const std::string& server_ip, uint16_t server_port) {
    FlowerBedClient client(server_ip, server_port);
    client.start();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::string server_ip = argv[1];
    uint16_t server_port = std::stoi(argv[2]);

    std::thread client_thread(clientTask, server_ip, server_port);
    client_thread.join();

    return 0;
}
