#ifndef CHUNKEDDECODER_HPP
#define CHUNKEDDECODER_HPP

#include <string>

enum ChunkedState {
    CHUNK_STATE_SIZE,
    CHUNK_STATE_DATA,
    CHUNK_STATE_CRLF,
    CHUNK_STATE_TRAILERS,
    CHUNK_STATE_DONE,
    CHUNK_STATE_ERROR
};

class ChunkedDecoder {
private:
    ChunkedState _state;
    size_t _chunkSize;
    size_t _chunkBytesRead;
    std::string _buffer;
    std::string _decodedBody;
    size_t _maxBodySize;

public:
    explicit ChunkedDecoder(size_t maxBodySize = 0);
    ~ChunkedDecoder();

    // Feeds raw chunked bytes from socket. Returns true if more data needed or done, false on error.
    bool feed(const char* data, size_t len);
    bool isDone() const;
    bool isError() const;
    const std::string& getDecodedBody() const;
    void reset();
};

#endif
