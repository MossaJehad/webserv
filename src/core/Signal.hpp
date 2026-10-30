#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <csignal>

class Signal {
private:
    static volatile sig_atomic_t _stopRequested;

public:
    static void setup();
    static void handleSignal(int sig);
    static bool isStopping();
    static void requestStop();
};

#endif
