#ifndef CGIRESPONSEPARSER_HPP
#define CGIRESPONSEPARSER_HPP

#include "HttpResponse.hpp"
#include <string>

class CgiResponseParser {
public:
    static HttpResponse parse(const std::string& rawOutput);
};

#endif
