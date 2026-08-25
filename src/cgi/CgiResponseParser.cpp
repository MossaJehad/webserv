#include "CgiResponseParser.hpp"
#include "StringUtils.hpp"
#include "HttpStatus.hpp"

HttpResponse CgiResponseParser::parse(const std::string& rawOutput) {
    HttpResponse response(200);

    size_t headerEnd = rawOutput.find("\r\n\r\n");
    size_t delimLen = 4;

    if (headerEnd == std::string::npos) {
        headerEnd = rawOutput.find("\n\n");
        delimLen = 2;
    }

    if (headerEnd == std::string::npos) {
        // No headers produced by script, treat entire output as body
        response.setBody(rawOutput);
        response.setContentType("text/html");
        return response;
    }

    std::string headerBlock = rawOutput.substr(0, headerEnd);
    std::string bodyBlock = rawOutput.substr(headerEnd + delimLen);

    std::vector<std::string> lines = StringUtils::split(headerBlock, '\n');
    bool hasStatus = false;
    bool hasLocation = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = StringUtils::trim(lines[i]);
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = StringUtils::trim(line.substr(0, colon));
        std::string val = StringUtils::trim(line.substr(colon + 1));
        std::string lowerKey = StringUtils::toLower(key);

        if (lowerKey == "status") {
            hasStatus = true;
            size_t space = val.find(' ');
            if (space != std::string::npos) {
                int code = StringUtils::toInt(val.substr(0, space), 200);
                std::string reason = StringUtils::trim(val.substr(space + 1));
                response.setStatusCode(code);
                response.setReasonPhrase(reason);
            } else {
                int code = StringUtils::toInt(val, 200);
                response.setStatusCode(code);
                response.setReasonPhrase(HttpStatus::getReasonPhrase(code));
            }
        } else if (lowerKey == "location") {
            hasLocation = true;
            response.getHeaders().set(key, val);
        } else if (lowerKey == "set-cookie") {
            response.getHeaders().add(key, val);
        } else {
            response.getHeaders().set(key, val);
        }
    }

    if (!hasStatus && hasLocation) {
        response.setStatusCode(302);
        response.setReasonPhrase(HttpStatus::getReasonPhrase(302));
    }

    response.setBody(bodyBlock);
    if (!response.getHeaders().has("Content-Type")) {
        response.setContentType("text/html");
    }

    return response;
}
