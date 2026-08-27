#include "Webserv.hpp"
#include "Signal.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <exception>
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

    // Last line of defence: the server must never terminate unexpectedly, not
    // even on std::bad_alloc, so nothing is allowed to escape main().
    try {
        Webserv webserv;
        if (!webserv.init(configPath)) {
            Logger::error("Failed to initialize server");
            return EXIT_FAILURE;
        }
        webserv.run();
    } catch (const std::exception& e) {
        Logger::error("Fatal: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Logger::error("Fatal: unknown error");
        return EXIT_FAILURE;
    }

    Logger::info("webserv stopped gracefully");
    return EXIT_SUCCESS;
}
