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

// Upper bounds on what a single request message may spend before it is even
// routed, so a hostile peer cannot grow server memory without limit.
const size_t MAX_REQUEST_LINE_BYTES   = 8192;
const size_t MAX_HEADER_SECTION_BYTES = 16384;
const size_t MAX_HEADER_COUNT         = 128;

class RequestParser {
private:
    ParserState _state;
    int _errorCode;
    std::string _buffer;
    HttpRequest _request;
    ChunkedDecoder _chunkedDecoder;
    size_t _maxBodySize;
    size_t _bodyBytesRead;
    size_t _headerBytes;
    size_t _headerCount;
    bool _sawHost;

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

    // True when some bytes of a request have been consumed but the message is
    // not finished yet, i.e. the peer owes us data (used to answer 408).
    bool hasPartialRequest() const;

    // True when a body was announced but not fully read, meaning the peer is
    // probably still sending.
    bool wasBodyTruncated() const;

    const HttpRequest& getRequest() const;
    HttpRequest& getRequest();

    void setMaxBodySize(size_t size);
    void reset();

    // Hands over bytes received past the end of the current message so the
    // caller can replay them as the next (pipelined) request.
    std::string takeLeftover();
};

#endif
