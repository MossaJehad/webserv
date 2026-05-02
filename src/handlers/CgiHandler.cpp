#include "CgiHandler.hpp"
#include "../http/HttpResponse.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <signal.h>

CgiHandler::CgiHandler() {}
CgiHandler::~CgiHandler() {}

void CgiHandler::handle(const ServerConfig& srv,
                        const LocationConfig& loc,
                        const HttpRequest& req,
                        HttpResponse& resp,
                        const std::string& scriptPath)
{
    // Find extension
    std::string ext;
    size_t dot = scriptPath.rfind('.');
    if (dot != std::string::npos) ext = scriptPath.substr(dot + 1);

    std::string interpreter = findInterpreter(ext, loc);

    // Check script exists
    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0) {
        std::string ep = srv.getErrorPage(404);
        resp.setStatus(404);
        resp.setBody(HttpResponse::buildError(404, ep), "text/html");
        return;
    }

    // Pipes: parent reads from outPipe[0], child writes to outPipe[1]
    //        parent writes to inPipe[1], child reads from inPipe[0]
    int inPipe[2], outPipe[2];
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }

    if (pid == 0) {
        // Child
        dup2(inPipe[0],  STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        std::vector<std::string> envVec = buildEnv(srv, loc, req, scriptPath);
        char** envp = vecToCharPP(envVec);

        // Change to script directory first, then use basename as script arg
        std::string scriptDir  = scriptPath.substr(0, scriptPath.rfind('/'));
        std::string scriptBase = scriptPath.substr(scriptPath.rfind('/') + 1);
        if (!scriptDir.empty()) chdir(scriptDir.c_str());

        // Build argv: split interpreter string on spaces, then append script basename
        std::vector<std::string> argVec;
        if (!interpreter.empty()) {
            std::istringstream iss(interpreter);
            std::string token;
            while (iss >> token)
                argVec.push_back(token);
        }
        argVec.push_back("./" + scriptBase);
        char** argv = vecToCharPP(argVec);

        execve(argv[0], argv, envp);
        // If execve fails
        std::cerr << "execve failed for '" << argv[0] << "': " << strerror(errno) << std::endl;
        freeCharPP(envp);
        freeCharPP(argv);
        _exit(1);
    }

    // Parent
    close(inPipe[0]);
    close(outPipe[1]);

    // Set non-blocking
    fcntl(outPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(inPipe[1],  F_SETFL, O_NONBLOCK);

    // Write request body to CGI stdin
    const std::string& body = req.getBody();
    size_t written = 0;
    while (written < body.size()) {
        ssize_t n = write(inPipe[1], body.c_str() + written, body.size() - written);
        if (n < 0) break;
        written += static_cast<size_t>(n);
    }
    close(inPipe[1]);

    // Read CGI output using poll with timeout
    std::string output;
    char buf[4096];
    struct pollfd pfd;
    pfd.fd     = outPipe[0];
    pfd.events = POLLIN;

    int timeout = 10000; // 10 seconds
    while (true) {
        int ret = poll(&pfd, 1, timeout);
        if (ret <= 0) break; // timeout or error
        if (pfd.revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(outPipe[0], buf, sizeof(buf));
            if (n <= 0) break;
            output.append(buf, static_cast<size_t>(n));
        }
        if (pfd.revents & (POLLHUP | POLLERR)) break;
    }
    close(outPipe[0]);

    // Wait for child
    int status;
    waitpid(pid, &status, 0);

    if (output.empty()) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }

    parseCgiOutput(output, resp);
}

std::vector<std::string> CgiHandler::buildEnv(const ServerConfig& srv,
                                               const LocationConfig& loc,
                                               const HttpRequest& req,
                                               const std::string& scriptPath) const
{
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=" + req.getVersion());
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("REQUEST_URI=" + req.getUri());
    env.push_back("PATH_INFO=" + req.getPath());
    env.push_back("SCRIPT_NAME=" + req.getPath());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("QUERY_STRING=" + req.getQuery());
    env.push_back("REDIRECT_STATUS=200");

    std::ostringstream port;
    port << srv.port;
    env.push_back("SERVER_PORT=" + port.str());

    std::string host = req.getHeader("host");
    if (host.empty()) host = srv.host;
    env.push_back("SERVER_NAME=" + host);
    env.push_back("HTTP_HOST=" + host);

    std::string ct = req.getHeader("content-type");
    if (!ct.empty()) env.push_back("CONTENT_TYPE=" + ct);

    std::ostringstream cl;
    cl << req.getBody().size();
    env.push_back("CONTENT_LENGTH=" + cl.str());

    // Forward important headers
    const std::map<std::string, std::string>& headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string name = "HTTP_";
        for (size_t i = 0; i < it->first.size(); ++i) {
            char c = it->first[i];
            if (c == '-') name += '_';
            else name += static_cast<char>(toupper(static_cast<unsigned char>(c)));
        }
        env.push_back(name + "=" + it->second);
    }

    // Add document root
    if (!loc.root.empty()) env.push_back("DOCUMENT_ROOT=" + loc.root);

    // Preserve PATH for interpreter lookup
    const char* pathEnv = getenv("PATH");
    if (pathEnv) env.push_back(std::string("PATH=") + pathEnv);

    return env;
}

std::string CgiHandler::findInterpreter(const std::string& ext,
                                         const LocationConfig& loc) const
{
    if (!loc.cgiPass.empty()) return loc.cgiPass;
    // Use /usr/bin/env as a portable PATH-based lookup
    if (ext == "py" || ext == "python") return "/usr/bin/env python3";
    if (ext == "php") return "/usr/bin/env php";
    if (ext == "pl" || ext == "perl")  return "/usr/bin/env perl";
    if (ext == "rb" || ext == "ruby")  return "/usr/bin/env ruby";
    if (ext == "sh") return "/usr/bin/env sh";
    return "";
}

void CgiHandler::parseCgiOutput(const std::string& raw, HttpResponse& resp) const
{
    // Split headers from body
    size_t sep = raw.find("\r\n\r\n");
    bool hasCRLF = true;
    if (sep == std::string::npos) {
        sep = raw.find("\n\n");
        hasCRLF = false;
    }

    if (sep == std::string::npos) {
        // No headers, treat all as body
        resp.setStatus(200);
        resp.setBody(raw, "text/html");
        return;
    }

    std::string headerSection = raw.substr(0, sep);
    std::string body = raw.substr(sep + (hasCRLF ? 4 : 2));

    // Parse CGI headers
    int statusCode = 200;
    std::string contentType = "text/html";
    std::istringstream hs(headerSection);
    std::string line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line[line.size()-1] == '\r')
            line = line.substr(0, line.size()-1);
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value = value.substr(1);

        // Case-insensitive compare
        std::string nameLow = name;
        for (size_t i = 0; i < nameLow.size(); ++i)
            nameLow[i] = static_cast<char>(tolower(static_cast<unsigned char>(nameLow[i])));

        if (nameLow == "status") {
            statusCode = std::atoi(value.c_str());
        } else if (nameLow == "content-type") {
            contentType = value;
        } else if (nameLow == "location") {
            resp.setStatus(302);
            resp.setHeader("Location", value);
            resp.setBody("", "text/html");
            return;
        } else {
            resp.setHeader(name, value);
        }
    }

    resp.setStatus(statusCode);
    resp.setBody(body, contentType);
}

char** CgiHandler::vecToCharPP(const std::vector<std::string>& v) const
{
    char** arr = new char*[v.size() + 1];
    for (size_t i = 0; i < v.size(); ++i) {
        arr[i] = new char[v[i].size() + 1];
        std::strcpy(arr[i], v[i].c_str());
    }
    arr[v.size()] = NULL;
    return arr;
}

void CgiHandler::freeCharPP(char** pp) const
{
    if (!pp) return;
    for (size_t i = 0; pp[i]; ++i)
        delete[] pp[i];
    delete[] pp;
}
