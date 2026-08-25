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

void CgiProcess::handleStdinWrite() {
    if (_stdinFd < 0 || _inputWritten >= _inputBody.size()) {
        if (_stdinFd >= 0) {
            if (_registry && _writeHandler) {
                _registry->unregisterHandler(_stdinFd);
            }
            close(_stdinFd);
            _stdinFd = -1;
        }
        return;
    }

    size_t remaining = _inputBody.size() - _inputWritten;
    ssize_t bytes = write(_stdinFd, _inputBody.data() + _inputWritten, remaining);

    if (bytes > 0) {
        _inputWritten += bytes;
    }

    if (_inputWritten >= _inputBody.size() || bytes <= 0) {
        if (_registry && _writeHandler) {
            _registry->unregisterHandler(_stdinFd);
        }
        close(_stdinFd);
        _stdinFd = -1;
    }
}

void CgiProcess::handleStdoutRead() {
    if (_stdoutFd < 0) return;

    char buffer[4096];
    ssize_t bytes;
    bool gotEof = false;

    while ((bytes = read(_stdoutFd, buffer, sizeof(buffer))) > 0) {
        _outputBuffer.append(buffer, bytes);
    }

    if (bytes == 0) {
        gotEof = true;
    } else if (bytes < 0) {
        int status;
        pid_t res = waitpid(_pid, &status, WNOHANG);
        if (res > 0) {
            gotEof = true;
        }
    }

    if (gotEof) {
        if (_registry && _readHandler) {
            _registry->unregisterHandler(_stdoutFd);
        }
        close(_stdoutFd);
        _stdoutFd = -1;

        int status;
        waitpid(_pid, &status, WNOHANG);

        _response = CgiResponseParser::parse(_outputBuffer);
        _isDone = true;
    }
}

void CgiProcess::checkTimeout() {
    if (_isDone) return;

    if (_startTime > 0 && (Time::now() - _startTime) >= _timeoutSeconds) {
        Logger::warn("CGI process timed out, killing pid " + StringUtils::toString(_pid));
        if (_pid > 0) {
            kill(_pid, SIGKILL);
            int status;
            waitpid(_pid, &status, 0);
        }

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
            _registry->unregisterHandler(_stdinFd);
        }
        if (_stdoutFd >= 0) {
            _registry->unregisterHandler(_stdoutFd);
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

    if (_pid > 0) {
        int status;
        waitpid(_pid, &status, WNOHANG);
        _pid = -1;
    }
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
