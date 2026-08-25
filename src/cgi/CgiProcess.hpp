#ifndef CGIPROCESS_HPP
#define CGIPROCESS_HPP

#include "IEventHandler.hpp"
#include "RequestContext.hpp"
#include "HttpResponse.hpp"
#include "PollRegistry.hpp"
#include <string>
#include <ctime>
#include <sys/types.h>

class CgiProcess;

class CgiPipeReadHandler : public IEventHandler {
private:
    CgiProcess* _process;
    int _fd;

public:
    CgiPipeReadHandler(CgiProcess* process, int fd);
    virtual ~CgiPipeReadHandler();

    virtual int getFd() const;
    virtual void handleRead();
    virtual void handleWrite();
    virtual bool wantsRead() const;
    virtual bool wantsWrite() const;
    virtual bool isDead() const;
};

class CgiPipeWriteHandler : public IEventHandler {
private:
    CgiProcess* _process;
    int _fd;

public:
    CgiPipeWriteHandler(CgiProcess* process, int fd);
    virtual ~CgiPipeWriteHandler();

    virtual int getFd() const;
    virtual void handleRead();
    virtual void handleWrite();
    virtual bool wantsRead() const;
    virtual bool wantsWrite() const;
    virtual bool isDead() const;
};

class CgiProcess {
private:
    RequestContext _ctx;
    pid_t _pid;
    int _stdinFd;   // Parent write end to child stdin
    int _stdoutFd;  // Parent read end from child stdout
    std::string _inputBody;
    size_t _inputWritten;
    std::string _outputBuffer;
    std::time_t _startTime;
    int _timeoutSeconds;
    bool _isDone;
    bool _isError;
    int _errorCode;
    HttpResponse _response;

    CgiPipeReadHandler* _readHandler;
    CgiPipeWriteHandler* _writeHandler;
    PollRegistry* _registry;

public:
    explicit CgiProcess(const RequestContext& ctx, int timeoutSeconds = 10);
    ~CgiProcess();

    bool launch(PollRegistry& registry);
    void handleStdinWrite();
    void handleStdoutRead();
    void checkTimeout();
    void cleanup();

    bool isDone() const;
    bool isError() const;
    int getErrorCode() const;
    const HttpResponse& getResponse() const;

    int getStdinFd() const;
    int getStdoutFd() const;
};

#endif
