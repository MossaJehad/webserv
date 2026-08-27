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

    // Descriptor numbers are recycled by the kernel, so a stale owner must
    // never remove the entry of whoever inherited its fd. Pass the handler that
    // is expected to be registered; the entry is dropped only if it matches.
    bool unregisterHandler(int fd, const IEventHandler* expected);
    bool unregisterHandler(int fd);
    bool unregisterHandler(IEventHandler* handler);
    IEventHandler* getHandler(int fd) const;

    void buildPollFds(std::vector<struct pollfd>& pollfds) const;
    const std::map<int, IEventHandler*>& getHandlers() const;
    size_t size() const;
    void clear();
};

#endif
