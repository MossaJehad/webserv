#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "HttpRequest.hpp"
#include "ChunkedDecoder.hpp"
#include <string>

enum ParserState {
    PARSER_STATE_REQUEST_LINE,
    PARSER_STATE_HEADERS,
    PARSER_STATE_BODY_CONTENT_LENGTH,
    PARSER_STATE_BODY_CHUNKED,
    PARSER_STATE_COMPLETE,
    PARSER_STATE_ERROR
};

class RequestParser {
private:
    ParserState _state;
    int _errorCode;
    std::string _buffer;
    HttpRequest _request;
    ChunkedDecoder _chunkedDecoder;
    size_t _maxBodySize;
    size_t _bodyBytesRead;

    void parseRequestLine(const std::string& line);
    void parseHeaderLine(const std::string& line);
    void parseUri(const std::string& uri);
    void finalizeHeaders();

public:
    explicit RequestParser(size_t maxBodySize = DEFAULT_CLIENT_MAX_BODY_SIZE);
    ~RequestParser();

    // Feeds raw bytes from client. Returns true if need more or complete, false on fatal error.
    bool feed(const char* data, size_t len);
    bool isComplete() const;
    bool isError() const;
    int getErrorCode() const;

    const HttpRequest& getRequest() const;
    HttpRequest& getRequest();

    void setMaxBodySize(size_t size);
    void reset();
};

#endif
