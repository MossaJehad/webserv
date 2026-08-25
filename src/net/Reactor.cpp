#include "Reactor.hpp"
#include "Signal.hpp"
#include "Logger.hpp"
#include <unistd.h>

Reactor::Reactor(PollRegistry& registry, ConnectionManager& connManager)
    : _registry(registry),
      _connManager(connManager),
      _running(false) {}

Reactor::~Reactor() {}

void Reactor::stop() {
    _running = false;
}

bool Reactor::isRunning() const {
    return _running;
}

void Reactor::run() {
    _running = true;
    std::vector<struct pollfd> pollfds;

    Logger::info("Reactor event loop started");

    while (_running && !Signal::isStopping()) {
        _registry.buildPollFds(pollfds);

        if (pollfds.empty()) {
            usleep(10000); // 10ms sleep if no fds
            continue;
        }

        int ready = poll(&pollfds[0], pollfds.size(), 100); // 100ms timeout for responsiveness

        if (ready > 0) {
            for (size_t i = 0; i < pollfds.size(); ++i) {
                if (pollfds[i].revents == 0) {
                    continue;
                }

                int fd = pollfds[i].fd;
                short revents = pollfds[i].revents;

                IEventHandler* handler = _registry.getHandler(fd);
                if (!handler || handler->isDead()) {
                    continue;
                }

                // Check readability first (or error/hangup)
                if (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                    handler->handleRead();
                }

                // If still alive and registered, check writability in same cycle
                handler = _registry.getHandler(fd);
                if (handler && !handler->isDead() && (revents & POLLOUT)) {
                    handler->handleWrite();
                }
            }
        }

        // Periodic timeout & cleanup sweep
        _connManager.sweepDeadAndTimedOut(_registry);
    }

    Logger::info("Reactor event loop stopped");
}
