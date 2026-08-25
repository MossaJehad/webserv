#include "IoBuffer.hpp"

IoBuffer::IoBuffer() {}
IoBuffer::~IoBuffer() {}

void IoBuffer::append(const char* data, size_t len) {
    if (data && len > 0) {
        _data.append(data, len);
    }
}

void IoBuffer::append(const std::string& str) {
    _data.append(str);
}

void IoBuffer::consume(size_t len) {
    if (len >= _data.size()) {
        _data.clear();
    } else {
        _data.erase(0, len);
    }
}

const char* IoBuffer::data() const {
    return _data.data();
}

size_t IoBuffer::size() const {
    return _data.size();
}

bool IoBuffer::empty() const {
    return _data.empty();
}

void IoBuffer::clear() {
    _data.clear();
}

const std::string& IoBuffer::str() const {
    return _data;
}

size_t IoBuffer::find(const std::string& needle, size_t pos) const {
    return _data.find(needle, pos);
}

std::string IoBuffer::substr(size_t pos, size_t len) const {
    return _data.substr(pos, len);
}
