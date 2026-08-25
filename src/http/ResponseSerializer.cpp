#include "ResponseSerializer.hpp"
#include "Time.hpp"
#include "StringUtils.hpp"
#include <sstream>

std::string ResponseSerializer::serialize(HttpResponse& response) {
    if (response.isRaw()) {
        return response.getBody();
    }

    std::ostringstream oss;

    // Status Line
    oss << "HTTP/1.1 " << response.getStatusCode() << " " << response.getReasonPhrase() << "\r\n";

    // Standard headers
    if (!response.getHeaders().has("Server")) {
        oss << "Server: webserv/1.0\r\n";
    }
    if (!response.getHeaders().has("Date")) {
        oss << "Date: " << Time::currentHttpDate() << "\r\n";
    }
    if (!response.getHeaders().has("Content-Length")) {
        oss << "Content-Length: " << response.getBody().size() << "\r\n";
    }
    if (!response.getHeaders().has("Content-Type")) {
        oss << "Content-Type: text/html\r\n";
    }
    if (!response.getHeaders().has("Connection")) {
        oss << "Connection: " << (response.isKeepAlive() ? "keep-alive" : "close") << "\r\n";
    }

    // Custom headers & Cookies
    const std::vector<std::pair<std::string, std::string> >& headerList = response.getHeaders().getList();
    for (size_t i = 0; i < headerList.size(); ++i) {
        oss << headerList[i].first << ": " << headerList[i].second << "\r\n";
    }

    // End of headers
    oss << "\r\n";

    // Body
    oss << response.getBody();

    return oss.str();
}
