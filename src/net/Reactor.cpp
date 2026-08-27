#include "Reactor.hpp"
#include "Signal.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <exception>

namespace {
const int POLL_TIMEOUT_MS = 100; // keeps timeout sweeps and signals responsive
}

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
            // No descriptor is currently interested in events: idle via poll()
            // itself rather than a sleep call, keeping every wait inside poll().
            poll(NULL, 0, POLL_TIMEOUT_MS);
            _connManager.sweepDeadAndTimedOut(_registry);
            continue;
        }

        int ready = poll(&pollfds[0], pollfds.size(), POLL_TIMEOUT_MS);

        if (ready > 0) {
            dispatch(pollfds);
        }

        // Periodic timeout & cleanup sweep
        _connManager.sweepDeadAndTimedOut(_registry);
    }

    Logger::info("Reactor event loop stopped");
}

void Reactor::dispatch(const std::vector<struct pollfd>& pollfds) {
    for (size_t i = 0; i < pollfds.size(); ++i) {
        if (pollfds[i].revents == 0) {
            continue;
        }

        int fd = pollfds[i].fd;
        short revents = pollfds[i].revents;

        // Re-resolve the handler on every step: a previous iteration may have
        // closed this descriptor and unregistered its handler.
        IEventHandler* handler = _registry.getHandler(fd);
        if (!handler || handler->isDead()) {
            continue;
        }

        // A failure while servicing one descriptor (std::bad_alloc on a huge
        // request, for instance) must cost only that connection. The reactor
        // itself has to keep running, so nothing escapes this scope.
        try {
            // Readability, hang-up and error all funnel into the read path so
            // the handler can observe the peer's EOF exactly once.
            if (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                handler->handleRead();
            }

            // If still alive and registered, check writability in same cycle
            handler = _registry.getHandler(fd);
            if (handler && !handler->isDead() && (revents & POLLOUT)) {
                handler->handleWrite();
            }
        } catch (const std::exception& e) {
            Logger::error("Dropping fd " + StringUtils::toString(fd) + ": " + e.what());
            _connManager.closeConnection(fd, _registry);
        } catch (...) {
            Logger::error("Dropping fd " + StringUtils::toString(fd) + ": unknown error");
            _connManager.closeConnection(fd, _registry);
        }
    }
}
