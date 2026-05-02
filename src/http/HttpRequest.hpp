#pragma once
#include <string>
#include <map>
#include <vector>

enum RequestParseState {
    PARSE_REQUEST_LINE,
    PARSE_HEADERS,
    PARSE_BODY,
    PARSE_CHUNKED,
    PARSE_COMPLETE,
    PARSE_ERROR
};

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    void reset();
    // Feed raw bytes; returns true when request is complete
    bool feed(const char* data, size_t len);

    bool isComplete() const;
    bool hasError() const;
    int  getErrorCode() const;

    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getPath() const;
    const std::string& getQuery() const;
    const std::string& getVersion() const;
    const std::string& getHeader(const std::string& name) const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    bool isKeepAlive() const;

private:
    RequestParseState _state;
    std::string _raw;
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _query;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    int _errorCode;
    long _contentLength;
    bool _chunked;
    size_t _chunkSize;
    bool _chunkSizeParsed;

    bool parseRequestLine(const std::string& line);
    bool parseHeaderLine(const std::string& line);
    void parseUri(const std::string& raw);
    bool processBody();
    bool processChunked();
    std::string toLower(const std::string& s) const;
};
