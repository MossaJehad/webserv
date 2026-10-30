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
    bool _tooLarge;

public:
    explicit ChunkedDecoder(size_t maxBodySize = 0);
    ~ChunkedDecoder();

    // Feeds raw chunked bytes from socket. Returns true if more data needed or done, false on error.
    bool feed(const char* data, size_t len);
    bool isDone() const;
    bool isError() const;
    // Distinguishes "body exceeds client_max_body_size" (413) from a malformed
    // chunked stream (400).
    bool isTooLarge() const;
    const std::string& getDecodedBody() const;
    void setMaxBodySize(size_t size);
    // Hands back bytes received past the terminal chunk, which belong to the
    // next pipelined request rather than to this body.
    std::string takeLeftover();
    void reset();
};

#endif
