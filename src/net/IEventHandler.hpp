#ifndef IEVENTHANDLER_HPP
#define IEVENTHANDLER_HPP

class IEventHandler {
public:
    virtual ~IEventHandler() {}
    virtual int getFd() const = 0;
    virtual void handleRead() = 0;
    virtual void handleWrite() = 0;
    virtual bool wantsRead() const = 0;
    virtual bool wantsWrite() const = 0;
    virtual bool isDead() const = 0;
    virtual void handleTimeout() {}
};

#endif
