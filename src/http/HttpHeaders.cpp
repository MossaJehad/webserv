#include "HttpHeaders.hpp"
#include "StringUtils.hpp"

HttpHeaders::HttpHeaders() {}
HttpHeaders::~HttpHeaders() {}

void HttpHeaders::set(const std::string& name, const std::string& value) {
    std::string lowerName = StringUtils::toLower(name);
    std::string trimmedVal = StringUtils::trim(value);
    _headers[lowerName] = trimmedVal;

    // Update list: replace if exists, else append
    bool found = false;
    for (size_t i = 0; i < _headerList.size(); ++i) {
        if (StringUtils::toLower(_headerList[i].first) == lowerName) {
            _headerList[i].second = trimmedVal;
            found = true;
            break;
        }
    }
    if (!found) {
        _headerList.push_back(std::make_pair(name, trimmedVal));
    }

    if (lowerName == "cookie" || lowerName == "set-cookie") {
        _cookies.push_back(trimmedVal);
    }
}

void HttpHeaders::add(const std::string& name, const std::string& value) {
    std::string lowerName = StringUtils::toLower(name);
    std::string trimmedVal = StringUtils::trim(value);

    std::map<std::string, std::string>::iterator it = _headers.find(lowerName);
    if (it != _headers.end()) {
        it->second += ", " + trimmedVal;
    } else {
        _headers[lowerName] = trimmedVal;
    }
    _headerList.push_back(std::make_pair(name, trimmedVal));

    if (lowerName == "cookie" || lowerName == "set-cookie") {
        _cookies.push_back(trimmedVal);
    }
}

std::string HttpHeaders::get(const std::string& name) const {
    std::string lowerName = StringUtils::toLower(name);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowerName);
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

bool HttpHeaders::has(const std::string& name) const {
    return _headers.find(StringUtils::toLower(name)) != _headers.end();
}

void HttpHeaders::remove(const std::string& name) {
    std::string lowerName = StringUtils::toLower(name);
    _headers.erase(lowerName);
    for (size_t i = 0; i < _headerList.size(); ) {
        if (StringUtils::toLower(_headerList[i].first) == lowerName) {
            _headerList.erase(_headerList.begin() + i);
        } else {
            ++i;
        }
    }
}

void HttpHeaders::clear() {
    _headers.clear();
    _headerList.clear();
    _cookies.clear();
}

const std::vector<std::pair<std::string, std::string> >& HttpHeaders::getList() const {
    return _headerList;
}

const std::map<std::string, std::string>& HttpHeaders::getMap() const {
    return _headers;
}

const std::vector<std::string>& HttpHeaders::getCookies() const {
    return _cookies;
}
