#include "RequestParser.hpp"
#include "StringUtils.hpp"
#include "HttpStatus.hpp"

RequestParser::RequestParser(size_t maxBodySize)
    : _state(PARSER_STATE_REQUEST_LINE),
      _errorCode(0),
      _chunkedDecoder(maxBodySize),
      _maxBodySize(maxBodySize),
      _bodyBytesRead(0) {}

RequestParser::~RequestParser() {}

void RequestParser::reset() {
    _state = PARSER_STATE_REQUEST_LINE;
    _errorCode = 0;
    _buffer.clear();
    _request.clear();
    _chunkedDecoder.reset();
    _bodyBytesRead = 0;
}

bool RequestParser::isComplete() const {
    return _state == PARSER_STATE_COMPLETE;
}

bool RequestParser::isError() const {
    return _state == PARSER_STATE_ERROR;
}

int RequestParser::getErrorCode() const {
    return _errorCode;
}

const HttpRequest& RequestParser::getRequest() const {
    return _request;
}

HttpRequest& RequestParser::getRequest() {
    return _request;
}

void RequestParser::setMaxBodySize(size_t size) {
    _maxBodySize = size;
}

void RequestParser::parseUri(const std::string& uri) {
    _request.setUri(uri);

    std::string path = uri;
    std::string query;
    std::string fragment;

    size_t hashPos = path.find('#');
    if (hashPos != std::string::npos) {
        fragment = path.substr(hashPos + 1);
        path = path.substr(0, hashPos);
    }

    size_t qPos = path.find('?');
    if (qPos != std::string::npos) {
        query = path.substr(qPos + 1);
        path = path.substr(0, qPos);
    }

    _request.setPath(path);
    _request.setQuery(query);
    _request.setFragment(fragment);
}

void RequestParser::parseRequestLine(const std::string& line) {
    std::vector<std::string> parts = StringUtils::splitWhitespace(line);
    if (parts.size() < 3) {
        _state = PARSER_STATE_ERROR;
        _errorCode = STATUS_BAD_REQUEST;
        return;
    }

    std::string methodStr = parts[0];
    std::string uriStr = parts[1];
    std::string versionStr = parts[2];

    HttpMethod method = stringToMethod(methodStr);
    _request.setRawMethod(methodStr);
    _request.setMethod(method);

    if (uriStr.empty() || uriStr[0] != '/') {
        // May be full URI like http://localhost:8080/index.html
        if (StringUtils::startsWith(uriStr, "http://") || StringUtils::startsWith(uriStr, "https://")) {
            size_t slashPos = uriStr.find('/', 8);
            if (slashPos != std::string::npos) {
                uriStr = uriStr.substr(slashPos);
            } else {
                uriStr = "/";
            }
        } else {
            _state = PARSER_STATE_ERROR;
            _errorCode = STATUS_BAD_REQUEST;
            return;
        }
    }

    if (uriStr.size() > 4096) {
        _state = PARSER_STATE_ERROR;
        _errorCode = STATUS_URI_TOO_LONG;
        return;
    }

    parseUri(uriStr);

    if (versionStr != "HTTP/1.0" && versionStr != "HTTP/1.1") {
        if (StringUtils::startsWith(versionStr, "HTTP/")) {
            _state = PARSER_STATE_ERROR;
            _errorCode = STATUS_HTTP_VERSION_NOT_SUPPORTED;
            return;
        }
        _state = PARSER_STATE_ERROR;
        _errorCode = STATUS_BAD_REQUEST;
        return;
    }

    _request.setVersion(versionStr);
    _request.setKeepAlive(versionStr == "HTTP/1.1");
}

void RequestParser::parseHeaderLine(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos || colon == 0) {
        _state = PARSER_STATE_ERROR;
        _errorCode = STATUS_BAD_REQUEST;
        return;
    }

    std::string name = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    _request.getHeaders().add(name, value);
}

void RequestParser::finalizeHeaders() {
    // Extract Host header
    if (_request.getHeaders().has("Host")) {
        std::string hostVal = _request.getHeaders().get("Host");
        size_t colon = hostVal.find(':');
        if (colon != std::string::npos) {
            _request.setHost(hostVal.substr(0, colon));
            _request.setPort(StringUtils::toInt(hostVal.substr(colon + 1), 80));
        } else {
            _request.setHost(hostVal);
            _request.setPort(80);
        }
    } else if (_request.getVersion() == "HTTP/1.1") {
        // HTTP/1.1 requires Host header
        _state = PARSER_STATE_ERROR;
        _errorCode = STATUS_BAD_REQUEST;
        return;
    }

    // Check Connection header
    if (_request.getHeaders().has("Connection")) {
        std::string conn = StringUtils::toLower(_request.getHeaders().get("Connection"));
        if (conn == "close") {
            _request.setKeepAlive(false);
        } else if (conn == "keep-alive") {
            _request.setKeepAlive(true);
        }
    }

    // Check Transfer-Encoding
    if (_request.getHeaders().has("Transfer-Encoding")) {
        std::string te = StringUtils::toLower(_request.getHeaders().get("Transfer-Encoding"));
        if (te.find("chunked") != std::string::npos) {
            _request.setIsChunked(true);
            _chunkedDecoder.reset();
            _state = PARSER_STATE_BODY_CHUNKED;
            return;
        }
    }

    // Check Content-Length
    if (_request.getHeaders().has("Content-Length")) {
        std::string clStr = _request.getHeaders().get("Content-Length");
        size_t len = StringUtils::toSizeT(clStr);
        _request.setContentLength(len);

        if (_maxBodySize > 0 && len > _maxBodySize) {
            _state = PARSER_STATE_ERROR;
            _errorCode = STATUS_PAYLOAD_TOO_LARGE;
            return;
        }

        if (len > 0) {
            _bodyBytesRead = 0;
            _state = PARSER_STATE_BODY_CONTENT_LENGTH;
            return;
        }
    }

    _state = PARSER_STATE_COMPLETE;
}

bool RequestParser::feed(const char* data, size_t len) {
    if (_state == PARSER_STATE_COMPLETE || _state == PARSER_STATE_ERROR) {
        return _state == PARSER_STATE_COMPLETE;
    }

    _buffer.append(data, len);

    // 1. Request Line
    if (_state == PARSER_STATE_REQUEST_LINE) {
        size_t crlf = _buffer.find("\r\n");
        size_t lf = _buffer.find('\n');
        if (crlf == std::string::npos && lf == std::string::npos) {
            if (_buffer.size() > 8192) {
                _state = PARSER_STATE_ERROR;
                _errorCode = STATUS_URI_TOO_LONG;
                return false;
            }
            return true; // Need more data
        }

        std::string line;
        if (crlf != std::string::npos && (lf == std::string::npos || crlf <= lf)) {
            line = _buffer.substr(0, crlf);
            _buffer.erase(0, crlf + 2);
        } else {
            line = _buffer.substr(0, lf);
            _buffer.erase(0, lf + 1);
        }

        if (line.empty()) {
            // Ignore leading empty lines before request line
            return true;
        }

        parseRequestLine(line);
        if (_state == PARSER_STATE_ERROR) {
            return false;
        }
        _state = PARSER_STATE_HEADERS;
    }

    // 2. Headers
    if (_state == PARSER_STATE_HEADERS) {
        while (true) {
            size_t crlf = _buffer.find("\r\n");
            size_t lf = _buffer.find('\n');
            if (crlf == std::string::npos && lf == std::string::npos) {
                if (_buffer.size() > 16384) {
                    _state = PARSER_STATE_ERROR;
                    _errorCode = STATUS_BAD_REQUEST;
                    return false;
                }
                return true; // Need more data
            }

            std::string line;
            if (crlf != std::string::npos && (lf == std::string::npos || crlf <= lf)) {
                line = _buffer.substr(0, crlf);
                _buffer.erase(0, crlf + 2);
            } else {
                line = _buffer.substr(0, lf);
                _buffer.erase(0, lf + 1);
            }

            if (line.empty()) {
                // End of headers
                finalizeHeaders();
                break;
            } else {
                parseHeaderLine(line);
                if (_state == PARSER_STATE_ERROR) {
                    return false;
                }
            }
        }
    }

    // 3. Body with Content-Length
    if (_state == PARSER_STATE_BODY_CONTENT_LENGTH) {
        size_t needed = _request.getContentLength() - _bodyBytesRead;
        size_t available = _buffer.size();
        size_t toCopy = (available < needed) ? available : needed;

        _request.appendBody(_buffer.substr(0, toCopy));
        _bodyBytesRead += toCopy;
        _buffer.erase(0, toCopy);

        if (_bodyBytesRead >= _request.getContentLength()) {
            _state = PARSER_STATE_COMPLETE;
            return true;
        }
        return true;
    }

    // 4. Body Chunked
    if (_state == PARSER_STATE_BODY_CHUNKED) {
        if (!_buffer.empty()) {
            std::string toFeed = _buffer;
            _buffer.clear();
            if (!_chunkedDecoder.feed(toFeed.data(), toFeed.size())) {
                _state = PARSER_STATE_ERROR;
                _errorCode = STATUS_BAD_REQUEST;
                return false;
            }
        }
        if (_chunkedDecoder.isDone()) {
            _request.setBody(_chunkedDecoder.getDecodedBody());
            _state = PARSER_STATE_COMPLETE;
            return true;
        }
        return true;
    }

    return _state == PARSER_STATE_COMPLETE;
}
