#include "Signal.hpp"
#include <csignal>

volatile sig_atomic_t Signal::_stopRequested = 0;

void Signal::handleSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        _stopRequested = 1;
    }
}

void Signal::setup() {
    _stopRequested = 0;
    std::signal(SIGINT, Signal::handleSignal);
    std::signal(SIGTERM, Signal::handleSignal);
    std::signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE so writing to closed socket does not abort
}

bool Signal::isStopping() {
    return _stopRequested != 0;
}

void Signal::requestStop() {
    _stopRequested = 1;
}
