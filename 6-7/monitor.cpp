#include <cstdlib>
#include <iostream>
#include <thread>

#include "udp_client.hpp"

class GardenerClient {
   public:
    GardenerClient(const std::string &server_ip, uint16_t server_port)
        : client(server_ip, server_port) {}

    void start() {
        while (true) {
            try {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                auto response = client.sendMessage("/monitor/");
                if (response.has_value()) {
                    std::cout << "----------------------------\n";
                    std::cout << response.value() << std::endl;
                } else {
                    std::cerr << "[ERROR] No response received to monitor."
                              << std::endl;
                }
            } catch (const std::exception &e) {
                std::cerr
                    << "[ERROR] Error occired while geting info from server: "
                    << e.what() << std::endl;
            }
        }
    }

   private:
    UDPClient client;
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
