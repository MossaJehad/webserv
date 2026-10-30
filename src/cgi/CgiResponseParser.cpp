#include "CgiResponseParser.hpp"
#include "ErrorResponse.hpp"
#include "StringUtils.hpp"
#include "HttpStatus.hpp"

namespace {

// A CGI script's output is untrusted input. Control characters in a header name
// or value would be written straight into our response, letting a script inject
// or truncate headers, so such fields are dropped.
bool isSafeHeaderField(const std::string& name, const std::string& value) {
    if (name.empty()) {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (c < 0x21 || c == 0x7F || c == ':') {
            return false;
        }
    }
    for (size_t i = 0; i < value.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        if ((c < 0x20 && c != '\t') || c == 0x7F) {
            return false;
        }
    }
    return true;
}

} // namespace

HttpResponse CgiResponseParser::parse(const std::string& rawOutput) {
    HttpResponse response(200);

    // RFC 3875 6.2: a script response must carry at least a Content-Type or a
    // Location. Producing nothing at all means the script failed, so report a
    // gateway error instead of a misleading empty 200.
    if (rawOutput.empty()) {
        return ErrorResponse::build(STATUS_BAD_GATEWAY);
    }

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

        // The Status field is no more trustworthy than the others: its reason
        // phrase is copied into our status line, so a control byte there could
        // forge or truncate the response head.
        if (!isSafeHeaderField(key, val)) {
            continue; // malformed field from the script: do not relay it
        }

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
