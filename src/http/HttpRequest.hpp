#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "ConfigTypes.hpp"
#include "HttpHeaders.hpp"
#include <string>

class HttpRequest {
private:
    HttpMethod _method;
    std::string _rawMethod;
    std::string _uri;
    std::string _path;
    std::string _query;
    std::string _fragment;
    std::string _version;
    HttpHeaders _headers;
    std::string _body;
    std::string _host;
    int _port;
    size_t _contentLength;
    bool _isChunked;
    bool _keepAlive;
    std::string _clientIp;
    int _clientPort;

public:
    HttpRequest();
    ~HttpRequest();

    HttpMethod getMethod() const;
    void setMethod(HttpMethod method);

    const std::string& getRawMethod() const;
    void setRawMethod(const std::string& rawMethod);

    const std::string& getUri() const;
    void setUri(const std::string& uri);

    const std::string& getPath() const;
    void setPath(const std::string& path);

    const std::string& getQuery() const;
    void setQuery(const std::string& query);

    const std::string& getFragment() const;
    void setFragment(const std::string& fragment);

    const std::string& getVersion() const;
    void setVersion(const std::string& version);

    const HttpHeaders& getHeaders() const;
    HttpHeaders& getHeaders();

    const std::string& getBody() const;
    void setBody(const std::string& body);
    void appendBody(const std::string& chunk);

    const std::string& getHost() const;
    void setHost(const std::string& host);

    int getPort() const;
    void setPort(int port);

    size_t getContentLength() const;
    void setContentLength(size_t length);

    bool isChunked() const;
    void setIsChunked(bool chunked);

    bool isKeepAlive() const;
    void setKeepAlive(bool keepAlive);

    const std::string& getClientIp() const;
    void setClientIp(const std::string& ip);

    int getClientPort() const;
    void setClientPort(int port);

    void clear();
};

#endif
