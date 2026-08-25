#ifndef REACTOR_HPP
#define REACTOR_HPP

#include "PollRegistry.hpp"
#include "ConnectionManager.hpp"
#include <vector>
#include <poll.h>

class Reactor {
private:
    PollRegistry& _registry;
    ConnectionManager& _connManager;
    bool _running;

public:
    Reactor(PollRegistry& registry, ConnectionManager& connManager);
    ~Reactor();

    void run();
    void stop();
    bool isRunning() const;
};

#endif
