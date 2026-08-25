#ifndef IOBUFFER_HPP
#define IOBUFFER_HPP

#include <string>
#include <vector>

class IoBuffer {
private:
    std::string _data;

public:
    IoBuffer();
    ~IoBuffer();

    void append(const char* data, size_t len);
    void append(const std::string& str);
    void consume(size_t len);

    const char* data() const;
    size_t size() const;
    bool empty() const;
    void clear();

    const std::string& str() const;
    size_t find(const std::string& needle, size_t pos = 0) const;
    std::string substr(size_t pos, size_t len = std::string::npos) const;
};

#endif
