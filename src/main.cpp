#include "Webserv.hpp"
#include "Signal.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    std::string configPath = "config/default.conf";

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [configuration_file]" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        configPath = argv[1];
    }

    Signal::setup();

    Logger::info("Starting webserv...");

    Webserv webserv;
    if (!webserv.init(configPath)) {
        Logger::error("Failed to initialize server");
        return EXIT_FAILURE;
    }

    webserv.run();

    Logger::info("webserv stopped gracefully");
    return EXIT_SUCCESS;
}
