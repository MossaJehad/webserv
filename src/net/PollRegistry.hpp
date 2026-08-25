#ifndef POLLREGISTRY_HPP
#define POLLREGISTRY_HPP

#include "IEventHandler.hpp"
#include <poll.h>
#include <map>
#include <vector>
#include <cstddef>

class PollRegistry {
private:
    std::map<int, IEventHandler*> _handlers;

public:
    PollRegistry();
    ~PollRegistry();

    bool registerHandler(IEventHandler* handler);
    bool unregisterHandler(int fd);
    bool unregisterHandler(IEventHandler* handler);
    IEventHandler* getHandler(int fd) const;

    void buildPollFds(std::vector<struct pollfd>& pollfds) const;
    const std::map<int, IEventHandler*>& getHandlers() const;
    size_t size() const;
    void clear();
};

#endif
