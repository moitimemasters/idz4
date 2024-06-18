#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "udp_client.hpp"

class GardenerClient {
   public:
    GardenerClient(const std::string &server_ip, uint16_t server_port)
        : client(server_ip, server_port), stop_flag(false) {
        std::srand(std::time(nullptr));
    }

    static constexpr uint32_t ping_interval = 5;
    static constexpr uint32_t min_watering_interval = 2;
    static constexpr uint32_t max_watering_interval = 5;
    static constexpr size_t retry_attempts = 3;

    void start() {
        jthreads.emplace_back(&GardenerClient::pingServer, this,
                              std::stop_token());
        jthreads.emplace_back(&GardenerClient::waterFlowers, this,
                              std::stop_token());

    }

   private:
    UDPClient client;
    std::atomic<bool> stop_flag;
    std::mutex mtx;
    std::mutex socket_mtx;
    std::condition_variable cv;
    std::vector<std::jthread> jthreads;

    void pingServer(std::stop_token stop_token) {
        try {
            while (!stop_token.stop_requested() && !stop_flag.load()) {
                std::optional<std::string> response;
                {
                    std::lock_guard<std::mutex> lock(socket_mtx);
                    std::cout << "[INFO] Pinging server..." << std::endl;
                    response = client.sendMessage("/ping/gardener/",
                                                  5);  // Timeout in seconds
                }

                if (response.has_value()) {
                    if (response.value() == "OK") {
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

                std::this_thread::sleep_for(
                    std::chrono::seconds(ping_interval));
            }
        } catch (const std::exception &e) {
            std::cerr << "Ping thread exception: " << e.what() << std::endl;
            stop_flag.store(true);
            cv.notify_all();
        }
    }

    void waterFlowers(std::stop_token stop_token) {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> interval_dis(min_watering_interval,
                                                        max_watering_interval);

        try {
            while (!stop_token.stop_requested() && !stop_flag.load()) {
                int sleep_duration = interval_dis(gen);
                std::this_thread::sleep_for(
                    std::chrono::seconds(sleep_duration));

                int flower_index = -1;
                bool should_retry = true;
                size_t attempts = 0;

                while (should_retry && attempts < retry_attempts &&
                       !stop_token.stop_requested()) {
                    std::optional<std::string> response;
                    {
                        std::lock_guard<std::mutex> lock(socket_mtx);
                        std::cout << "[INFO] Requesting flower from server..."
                                  << std::endl;
                        response = client.sendMessage("/getFlower/", 10);
                    }

                    if (response.has_value()) {
                        const auto &val = response.value();
                        if (val == "NOT_READY") {
                            std::cerr
                                << "[WARNING] Server not ready, retrying..."
                                << std::endl;
                            attempts++;
                            std::this_thread::sleep_for(
                                std::chrono::seconds(10));
                        } else {
                            flower_index = std::stoi(val);
                            should_retry = false;
                        }
                    } else {
                        std::cerr << "[ERROR] No response received when "
                                     "getting flower."
                                  << std::endl;
                        attempts++;
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                }

                if (attempts >= retry_attempts) {
                    std::cerr
                        << "[ERROR] Exceeded maximum retry attempts. Exiting..."
                        << std::endl;

                    stop_flag.store(true);
                    break;
                }

                if (flower_index < 0) {
                    std::cout << "[INFO] No flowers to water." << std::endl;
                    continue;
                } else {
                    std::cout << "[INFO] Got flower number from server: "
                              << flower_index << std::endl;
                }

                {
                    std::optional<std::string> response;
                    {
                        std::lock_guard<std::mutex> lock(socket_mtx);
                        std::cout
                            << "[INFO] Notifying server to water flower number "
                            << flower_index << std::endl;
                        response = client.sendMessage(
                            "/water/" + std::to_string(flower_index), 10);
                    }

                    if (response.has_value()) {
                        if (response.value() != "OK") {
                            std::cerr << "[ERROR] Unexpected response to "
                                         "watering flower: "
                                      << response.value() << std::endl;
                            stop_flag.store(true);
                            cv.notify_all();
                            break;
                        } else {
                            std::cout << "[INFO] Flower " << flower_index
                                      << " was watered successfully."
                                      << std::endl;
                        }
                    } else {
                        std::cerr << "[ERROR] No response received when "
                                     "watering flower."
                                  << std::endl;
                        stop_flag.store(true);
                        cv.notify_all();
                        break;
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr
                << "[ERROR] Error happend while in flower watering process: "
                << e.what() << std::endl;
            stop_flag.store(true);
            cv.notify_all();
        }
    }

    void stop() {
        stop_flag.store(true);
        cv.notify_all();

        for (auto &jth : jthreads) {
            if (jth.joinable()) {
                jth.request_stop();
            }
        }
    }
};

void clientTask(const std::string &server_ip, uint16_t server_port) {
    GardenerClient client(server_ip, server_port);
    client.start();
}

int main(int argc, char *argv[]) {
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
