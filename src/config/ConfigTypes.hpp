#ifndef CONFIGTYPES_HPP
#define CONFIGTYPES_HPP

#include <string>
#include <vector>
#include <set>

enum HttpMethod {
    METHOD_UNKNOWN = 0,
    METHOD_GET     = 1 << 0,
    METHOD_POST    = 1 << 1,
    METHOD_DELETE  = 1 << 2,
    METHOD_HEAD    = 1 << 3,
    METHOD_PUT     = 1 << 4
};

const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 10 * 1024 * 1024; // 10MB
const int DEFAULT_PORT = 8080;
const char DEFAULT_HOST[] = "0.0.0.0";
const char DEFAULT_INDEX[] = "index.html";
const char DEFAULT_ROOT[] = "www/site";

inline HttpMethod stringToMethod(const std::string& s) {
    if (s == "GET") return METHOD_GET;
    if (s == "POST") return METHOD_POST;
    if (s == "DELETE") return METHOD_DELETE;
    if (s == "HEAD") return METHOD_HEAD;
    if (s == "PUT") return METHOD_PUT;
    return METHOD_UNKNOWN;
}

inline std::string methodToString(HttpMethod m) {
    switch (m) {
        case METHOD_GET: return "GET";
        case METHOD_POST: return "POST";
        case METHOD_DELETE: return "DELETE";
        case METHOD_HEAD: return "HEAD";
        case METHOD_PUT: return "PUT";
        default: return "UNKNOWN";
    }
}

#endif
