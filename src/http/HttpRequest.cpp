#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>

HttpRequest::HttpRequest()
    : _state(PARSE_REQUEST_LINE), _errorCode(0),
      _contentLength(-1), _chunked(false),
      _chunkSize(0), _chunkSizeParsed(false)
{}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset()
{
    _state = PARSE_REQUEST_LINE;
    _raw.clear();
    _method.clear();
    _uri.clear();
    _path.clear();
    _query.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _errorCode = 0;
    _contentLength = -1;
    _chunked = false;
    _chunkSize = 0;
    _chunkSizeParsed = false;
}

bool HttpRequest::isComplete() const { return _state == PARSE_COMPLETE; }
bool HttpRequest::hasError()    const { return _state == PARSE_ERROR; }
int  HttpRequest::getErrorCode() const { return _errorCode; }

const std::string& HttpRequest::getMethod()  const { return _method; }
const std::string& HttpRequest::getUri()     const { return _uri; }
const std::string& HttpRequest::getPath()    const { return _path; }
const std::string& HttpRequest::getQuery()   const { return _query; }
const std::string& HttpRequest::getVersion() const { return _version; }
const std::string& HttpRequest::getBody()    const { return _body; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return _headers; }

static const std::string EMPTY_HDR;
const std::string& HttpRequest::getHeader(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(name);
    if (it != _headers.end()) return it->second;
    return EMPTY_HDR;
}

bool HttpRequest::isKeepAlive() const
{
    std::string conn = getHeader("connection");
    if (_version == "HTTP/1.1")
        return conn != "close";
    return conn == "keep-alive";
}

std::string HttpRequest::toLower(const std::string& s) const
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(r[i])));
    return r;
}

bool HttpRequest::feed(const char* data, size_t len)
{
    _raw.append(data, len);

    while (_state != PARSE_COMPLETE && _state != PARSE_ERROR) {
        if (_state == PARSE_REQUEST_LINE) {
            size_t pos = _raw.find("\r\n");
            if (pos == std::string::npos) return false;
            if (!parseRequestLine(_raw.substr(0, pos))) {
                _state = PARSE_ERROR;
                return false;
            }
            _raw = _raw.substr(pos + 2);
            _state = PARSE_HEADERS;
        } else if (_state == PARSE_HEADERS) {
            size_t pos = _raw.find("\r\n");
            if (pos == std::string::npos) return false;
            if (pos == 0) {
                // End of headers
                _raw = _raw.substr(2);
                // Determine body type
                std::string te = getHeader("transfer-encoding");
                std::string cl = getHeader("content-length");
                if (te == "chunked") {
                    _chunked = true;
                    _state = PARSE_CHUNKED;
                } else if (!cl.empty()) {
                    _contentLength = std::atol(cl.c_str());
                    if (_contentLength <= 0) {
                        _state = PARSE_COMPLETE;
                    } else {
                        _state = PARSE_BODY;
                    }
                } else {
                    _state = PARSE_COMPLETE;
                }
                continue;
            }
            std::string headerLine = _raw.substr(0, pos);
            _raw = _raw.substr(pos + 2);
            if (!parseHeaderLine(headerLine)) {
                _state = PARSE_ERROR;
                return false;
            }
        } else if (_state == PARSE_BODY) {
            if (!processBody()) return false;
        } else if (_state == PARSE_CHUNKED) {
            if (!processChunked()) return false;
        } else {
            break;
        }
    }
    return _state == PARSE_COMPLETE;
}

bool HttpRequest::parseRequestLine(const std::string& line)
{
    std::istringstream ss(line);
    ss >> _method >> _uri >> _version;
    if (_method.empty() || _uri.empty() || _version.empty()) {
        _errorCode = 400; return false;
    }
    if (_method != "GET" && _method != "POST" && _method != "DELETE" &&
        _method != "PUT" && _method != "HEAD" && _method != "OPTIONS") {
        _errorCode = 501; return false;
    }
    if (_version != "HTTP/1.0" && _version != "HTTP/1.1") {
        _errorCode = 505; return false;
    }
    parseUri(_uri);
    return true;
}

bool HttpRequest::parseHeaderLine(const std::string& line)
{
    size_t colon = line.find(':');
    if (colon == std::string::npos) { _errorCode = 400; return false; }
    std::string name  = toLower(line.substr(0, colon));
    std::string value = line.substr(colon + 1);
    // trim leading whitespace from value
    size_t start = value.find_first_not_of(" \t");
    if (start != std::string::npos) value = value.substr(start);
    // trim trailing
    size_t end = value.find_last_not_of(" \t\r");
    if (end != std::string::npos) value = value.substr(0, end + 1);
    _headers[name] = value;
    return true;
}

void HttpRequest::parseUri(const std::string& raw)
{
    size_t q = raw.find('?');
    if (q != std::string::npos) {
        _path  = raw.substr(0, q);
        _query = raw.substr(q + 1);
    } else {
        _path  = raw;
        _query = "";
    }
    // Basic URL decode of path
    std::string decoded;
    for (size_t i = 0; i < _path.size(); ++i) {
        if (_path[i] == '%' && i + 2 < _path.size()) {
            char hex[3] = { _path[i+1], _path[i+2], 0 };
            decoded += static_cast<char>(std::strtol(hex, NULL, 16));
            i += 2;
        } else {
            decoded += _path[i];
        }
    }
    _path = decoded;
}

bool HttpRequest::processBody()
{
    if ((long)_raw.size() >= _contentLength) {
        _body = _raw.substr(0, (size_t)_contentLength);
        _raw  = _raw.substr((size_t)_contentLength);
        _state = PARSE_COMPLETE;
        return true;
    }
    return false;
}

bool HttpRequest::processChunked()
{
    while (true) {
        if (!_chunkSizeParsed) {
            size_t pos = _raw.find("\r\n");
            if (pos == std::string::npos) return false;
            std::string sizeLine = _raw.substr(0, pos);
            _raw = _raw.substr(pos + 2);
            _chunkSize = static_cast<size_t>(std::strtoul(sizeLine.c_str(), NULL, 16));
            _chunkSizeParsed = true;
        }
        if (_chunkSize == 0) {
            // Trailing CRLF
            if (_raw.size() >= 2) _raw = _raw.substr(2);
            _state = PARSE_COMPLETE;
            return true;
        }
        if (_raw.size() < _chunkSize + 2) return false;
        _body += _raw.substr(0, _chunkSize);
        _raw = _raw.substr(_chunkSize + 2);
        _chunkSizeParsed = false;
    }
}
