#include "CgiProcess.hpp"
#include "CgiEnvironment.hpp"
#include "CgiResponseParser.hpp"
#include "ErrorResponse.hpp"
#include "Socket.hpp"
#include "FileSystem.hpp"
#include "Time.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>

// ==================== CgiPipeReadHandler ====================

CgiPipeReadHandler::CgiPipeReadHandler(CgiProcess* process, int fd)
    : _process(process), _fd(fd) {}

CgiPipeReadHandler::~CgiPipeReadHandler() {}

int CgiPipeReadHandler::getFd() const {
    return _fd;
}

void CgiPipeReadHandler::handleRead() {
    if (_process) {
        _process->handleStdoutRead();
    }
}

void CgiPipeReadHandler::handleWrite() {}

bool CgiPipeReadHandler::wantsRead() const {
    return _fd >= 0 && _process && !_process->isDone();
}

bool CgiPipeReadHandler::wantsWrite() const {
    return false;
}

bool CgiPipeReadHandler::isDead() const {
    return _fd < 0 || !_process || _process->isDone();
}

// ==================== CgiPipeWriteHandler ====================

CgiPipeWriteHandler::CgiPipeWriteHandler(CgiProcess* process, int fd)
    : _process(process), _fd(fd) {}

CgiPipeWriteHandler::~CgiPipeWriteHandler() {}

int CgiPipeWriteHandler::getFd() const {
    return _fd;
}

void CgiPipeWriteHandler::handleRead() {}

void CgiPipeWriteHandler::handleWrite() {
    if (_process) {
        _process->handleStdinWrite();
    }
}

bool CgiPipeWriteHandler::wantsRead() const {
    return false;
}

bool CgiPipeWriteHandler::wantsWrite() const {
    return _fd >= 0 && _process && !_process->isDone();
}

bool CgiPipeWriteHandler::isDead() const {
    return _fd < 0 || !_process || _process->isDone();
}

// ==================== CgiProcess ====================

CgiProcess::CgiProcess(const RequestContext& ctx, int timeoutSeconds)
    : _ctx(ctx),
      _pid(-1),
      _stdinFd(-1),
      _stdoutFd(-1),
      _inputBody(ctx.getRequest().getBody()),
      _inputWritten(0),
      _startTime(0),
      _timeoutSeconds(timeoutSeconds),
      _isDone(false),
      _isError(false),
      _errorCode(0),
      _readHandler(NULL),
      _writeHandler(NULL),
      _registry(NULL) {}

CgiProcess::~CgiProcess() {
    cleanup();
}

bool CgiProcess::launch(PollRegistry& registry) {
    _registry = &registry;

    std::string scriptPath = _ctx.getScriptPath().empty() ? _ctx.getResolvedFsPath() : _ctx.getScriptPath();
    if (!FileSystem::exists(scriptPath)) {
        _isDone = true;
        _isError = true;
        _errorCode = 404;
        _response = ErrorResponse::build(404, _ctx.getServer());
        return false;
    }

    int inPipe[2];
    int outPipe[2];

    if (pipe(inPipe) < 0) {
        _isDone = true;
        _isError = true;
        _errorCode = 500;
        _response = ErrorResponse::build(500, _ctx.getServer());
        return false;
    }

    if (pipe(outPipe) < 0) {
        close(inPipe[0]);
        close(inPipe[1]);
        _isDone = true;
        _isError = true;
        _errorCode = 500;
        _response = ErrorResponse::build(500, _ctx.getServer());
        return false;
    }

    CgiEnvironment cgiEnv;
    cgiEnv.build(_ctx);
    char** envp = cgiEnv.createEnvp();

    _pid = fork();
    if (_pid < 0) {
        CgiEnvironment::freeEnvp(envp);
        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);
        _isDone = true;
        _isError = true;
        _errorCode = 500;
        _response = ErrorResponse::build(500, _ctx.getServer());
        return false;
    }

    if (_pid == 0) {
        // Child Process
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);

        std::string scriptDir = FileSystem::getDirname(scriptPath);
        std::string scriptFile = FileSystem::getFilename(scriptPath);
        if (!scriptDir.empty()) {
            chdir(scriptDir.c_str());
        }

        std::string interpreter = _ctx.getCgiBin();
        char* argv[3];

        if (!interpreter.empty()) {
            argv[0] = const_cast<char*>(interpreter.c_str());
            argv[1] = const_cast<char*>(scriptFile.c_str());
            argv[2] = NULL;
            execve(interpreter.c_str(), argv, envp);
        } else {
            std::string execTarget = "./" + scriptFile;
            argv[0] = const_cast<char*>(execTarget.c_str());
            argv[1] = NULL;
            execve(execTarget.c_str(), argv, envp);
        }

        // If execve fails
        exit(127);
    }

    // Parent Process
    CgiEnvironment::freeEnvp(envp);

    close(inPipe[0]);
    close(outPipe[1]);

    _stdinFd = inPipe[1];
    _stdoutFd = outPipe[0];

    Socket::setNonBlocking(_stdinFd);
    Socket::setNonBlocking(_stdoutFd);

    // Parent-side pipe ends must not leak into later CGI children: a child
    // holding a copy of another request's stdin write end would keep that
    // script from ever seeing EOF on its body.
    Socket::setCloexec(_stdinFd);
    Socket::setCloexec(_stdoutFd);

    _startTime = Time::now();

    if (_inputBody.empty()) {
        close(_stdinFd);
        _stdinFd = -1;
    } else {
        _writeHandler = new CgiPipeWriteHandler(this, _stdinFd);
        _registry->registerHandler(_writeHandler);
    }

    _readHandler = new CgiPipeReadHandler(this, _stdoutFd);
    _registry->registerHandler(_readHandler);

    return true;
}

void CgiProcess::closeStdin() {
    if (_stdinFd < 0) {
        return;
    }
    if (_registry) {
        _registry->unregisterHandler(_stdinFd, _writeHandler);
    }
    close(_stdinFd);
    _stdinFd = -1;
}

void CgiProcess::handleStdinWrite() {
    if (_stdinFd < 0 || _inputWritten >= _inputBody.size()) {
        closeStdin(); // body fully delivered: the child must now see EOF
        return;
    }

    size_t remaining = _inputBody.size() - _inputWritten;
    ssize_t bytes = write(_stdinFd, _inputBody.data() + _inputWritten, remaining);

    if (bytes > 0) {
        _inputWritten += bytes;
    }

    if (_inputWritten >= _inputBody.size() || bytes <= 0) {
        closeStdin();
    }
}

void CgiProcess::handleStdoutRead() {
    if (_stdoutFd < 0) return;

    // Exactly one read per readiness notification: poll() is level-triggered,
    // so any remaining output is reported again on the next cycle.
    char buffer[65536];
    ssize_t bytes = read(_stdoutFd, buffer, sizeof(buffer));

    if (bytes > 0) {
        // Buffering the script's output is the one allocation that scales with
        // untrusted data here. Failing it must fail this request only, and it
        // has to be handled where the process is reachable so the pipe is torn
        // down instead of being polled again next cycle.
        try {
            _outputBuffer.append(buffer, bytes);
        } catch (...) {
            Logger::error("Cannot buffer CGI output; failing request");
            cleanup();
            _isDone = true;
            _isError = true;
            _errorCode = 502;
            _response = ErrorResponse::build(502, _ctx.getServer());
        }
        return;
    }

    if (bytes < 0) {
        return; // Not ready yet; wait for the next poll() event
    }

    // bytes == 0: the child closed its stdout, which marks end of output.
    if (_registry) {
        _registry->unregisterHandler(_stdoutFd, _readHandler);
    }
    close(_stdoutFd);
    _stdoutFd = -1;

    reapChild();

    _response = CgiResponseParser::parse(_outputBuffer);
    _isDone = true;
}

// Collect the child unconditionally so no zombie can survive the request.
// If it is still running after we have its full output, it is killed first;
// SIGKILL cannot be blocked, so the blocking wait returns immediately.
void CgiProcess::reapChild() {
    if (_pid <= 0) {
        return;
    }

    int status;
    pid_t reaped = waitpid(_pid, &status, WNOHANG);
    if (reaped == 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, &status, 0);
    }
    _pid = -1;
}

void CgiProcess::checkTimeout() {
    if (_isDone) return;

    if (_startTime > 0 && (Time::now() - _startTime) >= _timeoutSeconds) {
        Logger::warn("CGI process timed out, killing pid " + StringUtils::toString(_pid));
        cleanup();
        _isDone = true;
        _isError = true;
        _errorCode = 504;
        _response = ErrorResponse::build(504, _ctx.getServer());
    }
}

void CgiProcess::cleanup() {
    if (_registry) {
        if (_stdinFd >= 0) {
            _registry->unregisterHandler(_stdinFd, _writeHandler);
        }
        if (_stdoutFd >= 0) {
            _registry->unregisterHandler(_stdoutFd, _readHandler);
        }
    }

    if (_stdinFd >= 0) {
        close(_stdinFd);
        _stdinFd = -1;
    }

    if (_stdoutFd >= 0) {
        close(_stdoutFd);
        _stdoutFd = -1;
    }

    delete _writeHandler;
    _writeHandler = NULL;

    delete _readHandler;
    _readHandler = NULL;

    reapChild();
}

bool CgiProcess::isDone() const {
    return _isDone;
}

bool CgiProcess::isError() const {
    return _isError;
}

int CgiProcess::getErrorCode() const {
    return _errorCode;
}

const HttpResponse& CgiProcess::getResponse() const {
    return _response;
}

int CgiProcess::getStdinFd() const {
    return _stdinFd;
}

int CgiProcess::getStdoutFd() const {
    return _stdoutFd;
}
