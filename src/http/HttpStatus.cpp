#include "HttpStatus.hpp"

std::string HttpStatus::getReasonPhrase(int code) {
    switch (code) {
        case STATUS_OK:                    return "OK";
        case STATUS_CREATED:               return "Created";
        case STATUS_ACCEPTED:              return "Accepted";
        case STATUS_NO_CONTENT:            return "No Content";
        case STATUS_MOVED_PERMANENTLY:     return "Moved Permanently";
        case STATUS_FOUND:                 return "Found";
        case STATUS_SEE_OTHER:             return "See Other";
        case STATUS_NOT_MODIFIED:          return "Not Modified";
        case STATUS_TEMPORARY_REDIRECT:    return "Temporary Redirect";
        case STATUS_PERMANENT_REDIRECT:    return "Permanent Redirect";
        case STATUS_BAD_REQUEST:           return "Bad Request";
        case STATUS_UNAUTHORIZED:          return "Unauthorized";
        case STATUS_FORBIDDEN:             return "Forbidden";
        case STATUS_NOT_FOUND:             return "Not Found";
        case STATUS_METHOD_NOT_ALLOWED:    return "Method Not Allowed";
        case STATUS_REQUEST_TIMEOUT:       return "Request Timeout";
        case STATUS_CONFLICT:              return "Conflict";
        case STATUS_LENGTH_REQUIRED:       return "Length Required";
        case STATUS_PAYLOAD_TOO_LARGE:     return "Payload Too Large";
        case STATUS_URI_TOO_LONG:          return "URI Too Long";
        case STATUS_UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
        case STATUS_HEADERS_TOO_LARGE:     return "Request Header Fields Too Large";
        case STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case STATUS_NOT_IMPLEMENTED:       return "Not Implemented";
        case STATUS_BAD_GATEWAY:           return "Bad Gateway";
        case STATUS_SERVICE_UNAVAILABLE:   return "Service Unavailable";
        case STATUS_GATEWAY_TIMEOUT:       return "Gateway Timeout";
        case STATUS_HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        default:                           return "Unknown Status";
    }
}
