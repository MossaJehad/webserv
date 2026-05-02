#include <iostream>
#include <csignal>
#include <cstdlib>
#include "config/ConfigParser.hpp"
#include "core/ServerManager.hpp"

static bool g_running = true;

static void signalHandler(int sig)
{
    (void)sig;
    std::cout << "\nShutting down..." << std::endl;
    g_running = false;
    // Exit cleanly — destructors will run
    exit(0);
}

int main(int argc, char* argv[])
{
    std::string configFile = "webserv.conf";
    if (argc == 2)
        configFile = argv[1];
    else if (argc > 2) {
        std::cerr << "Usage: ./webserv [config_file]" << std::endl;
        return 1;
    }

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe

    ConfigParser parser;
    if (!parser.parse(configFile)) {
        std::cerr << "Config error: " << parser.getError() << std::endl;
        return 1;
    }

    const std::vector<ServerConfig>& servers = parser.getServers();
    std::cout << "Loaded " << servers.size() << " server(s) from " << configFile << std::endl;

    ServerManager manager;
    if (!manager.init(servers)) {
        std::cerr << "Failed to initialize server." << std::endl;
        return 1;
    }

    manager.run();
    return 0;
}
