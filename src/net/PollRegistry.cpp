#include "PollRegistry.hpp"
#include <cstddef>

PollRegistry::PollRegistry() {}
PollRegistry::~PollRegistry() {}

bool PollRegistry::registerHandler(IEventHandler* handler) {
    if (!handler || handler->getFd() < 0) {
        return false;
    }
    _handlers[handler->getFd()] = handler;
    return true;
}

bool PollRegistry::unregisterHandler(int fd) {
    if (fd < 0) return false;
    return _handlers.erase(fd) > 0;
}

bool PollRegistry::unregisterHandler(IEventHandler* handler) {
    if (!handler) return false;
    return unregisterHandler(handler->getFd());
}

IEventHandler* PollRegistry::getHandler(int fd) const {
    std::map<int, IEventHandler*>::const_iterator it = _handlers.find(fd);
    if (it != _handlers.end()) {
        return it->second;
    }
    return NULL;
}

void PollRegistry::buildPollFds(std::vector<struct pollfd>& pollfds) const {
    pollfds.clear();
    pollfds.reserve(_handlers.size());

    for (std::map<int, IEventHandler*>::const_iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        int fd = it->first;
        IEventHandler* handler = it->second;

        if (!handler || handler->isDead()) {
            continue;
        }

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = 0;
        pfd.revents = 0;

        if (handler->wantsRead()) {
            pfd.events |= POLLIN;
        }
        if (handler->wantsWrite()) {
            pfd.events |= POLLOUT;
        }

        if (pfd.events != 0) {
            pollfds.push_back(pfd);
        }
    }
}

const std::map<int, IEventHandler*>& PollRegistry::getHandlers() const {
    return _handlers;
}

size_t PollRegistry::size() const {
    return _handlers.size();
}

void PollRegistry::clear() {
    _handlers.clear();
}
