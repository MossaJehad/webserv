#ifndef REQUESTCONTEXT_HPP
#define REQUESTCONTEXT_HPP

#include "HttpRequest.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>

class RequestContext {
private:
    HttpRequest _request;
    const ServerConfig* _server;
    LocationConfig _location;
    std::string _resolvedFsPath;
    std::string _scriptPath;
    std::string _pathInfo;
    bool _isCgi;
    std::string _cgiBin;

public:
    RequestContext();
    ~RequestContext();

    const HttpRequest& getRequest() const;
    HttpRequest& getRequest();
    void setRequest(const HttpRequest& req);

    const ServerConfig* getServer() const;
    void setServer(const ServerConfig* server);

    const LocationConfig& getLocation() const;
    LocationConfig& getLocation();
    void setLocation(const LocationConfig& loc);

    const std::string& getResolvedFsPath() const;
    void setResolvedFsPath(const std::string& path);

    const std::string& getScriptPath() const;
    void setScriptPath(const std::string& scriptPath);

    const std::string& getPathInfo() const;
    void setPathInfo(const std::string& pathInfo);

    bool isCgi() const;
    void setIsCgi(bool isCgi);

    const std::string& getCgiBin() const;
    void setCgiBin(const std::string& cgiBin);
};

#endif
