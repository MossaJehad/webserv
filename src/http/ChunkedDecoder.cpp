#include "ChunkedDecoder.hpp"
#include "StringUtils.hpp"

ChunkedDecoder::ChunkedDecoder(size_t maxBodySize)
    : _state(CHUNK_STATE_SIZE),
      _chunkSize(0),
      _chunkBytesRead(0),
      _maxBodySize(maxBodySize) {}

ChunkedDecoder::~ChunkedDecoder() {}

void ChunkedDecoder::reset() {
    _state = CHUNK_STATE_SIZE;
    _chunkSize = 0;
    _chunkBytesRead = 0;
    _buffer.clear();
    _decodedBody.clear();
}

bool ChunkedDecoder::isDone() const {
    return _state == CHUNK_STATE_DONE;
}

bool ChunkedDecoder::isError() const {
    return _state == CHUNK_STATE_ERROR;
}

const std::string& ChunkedDecoder::getDecodedBody() const {
    return _decodedBody;
}

bool ChunkedDecoder::feed(const char* data, size_t len) {
    if (_state == CHUNK_STATE_DONE || _state == CHUNK_STATE_ERROR) {
        return _state == CHUNK_STATE_DONE;
    }

    _buffer.append(data, len);

    while (!_buffer.empty()) {
        if (_state == CHUNK_STATE_SIZE) {
            size_t crlf = _buffer.find("\r\n");
            if (crlf == std::string::npos) {
                // Check if buffer is getting abnormally large without crlf
                if (_buffer.size() > 1024) {
                    _state = CHUNK_STATE_ERROR;
                    return false;
                }
                return true; // Need more data
            }

            std::string line = _buffer.substr(0, crlf);
            _buffer.erase(0, crlf + 2);

            // Strip any chunk extensions (e.g. 1a;ext=val)
            size_t semi = line.find(';');
            if (semi != std::string::npos) {
                line = line.substr(0, semi);
            }
            line = StringUtils::trim(line);

            if (line.empty()) {
                _state = CHUNK_STATE_ERROR;
                return false;
            }

            _chunkSize = StringUtils::hexToSizeT(line);
            _chunkBytesRead = 0;

            if (_chunkSize == 0) {
                _state = CHUNK_STATE_TRAILERS;
            } else {
                if (_maxBodySize > 0 && (_decodedBody.size() + _chunkSize > _maxBodySize)) {
                    _state = CHUNK_STATE_ERROR;
                    return false;
                }
                _state = CHUNK_STATE_DATA;
            }
        } else if (_state == CHUNK_STATE_DATA) {
            size_t needed = _chunkSize - _chunkBytesRead;
            size_t available = _buffer.size();
            size_t toCopy = (available < needed) ? available : needed;

            _decodedBody.append(_buffer, 0, toCopy);
            _chunkBytesRead += toCopy;
            _buffer.erase(0, toCopy);

            if (_maxBodySize > 0 && _decodedBody.size() > _maxBodySize) {
                _state = CHUNK_STATE_ERROR;
                return false;
            }

            if (_chunkBytesRead == _chunkSize) {
                _state = CHUNK_STATE_CRLF;
            }
        } else if (_state == CHUNK_STATE_CRLF) {
            if (_buffer.size() < 2) {
                return true; // Need more data
            }
            if (_buffer[0] == '\r' && _buffer[1] == '\n') {
                _buffer.erase(0, 2);
                _state = CHUNK_STATE_SIZE;
            } else if (_buffer[0] == '\n') {
                _buffer.erase(0, 1);
                _state = CHUNK_STATE_SIZE;
            } else {
                _state = CHUNK_STATE_ERROR;
                return false;
            }
        } else if (_state == CHUNK_STATE_TRAILERS) {
            // Read until empty line "\r\n"
            if (_buffer.size() >= 2 && _buffer.substr(0, 2) == "\r\n") {
                _buffer.erase(0, 2);
                _state = CHUNK_STATE_DONE;
                return true;
            }
            size_t crlf = _buffer.find("\r\n");
            if (crlf == std::string::npos) {
                return true; // Need more data
            }
            if (crlf == 0) {
                _buffer.erase(0, 2);
                _state = CHUNK_STATE_DONE;
                return true;
            }
            // Skip trailer line
            _buffer.erase(0, crlf + 2);
        } else {
            break;
        }
    }

    return true;
}
