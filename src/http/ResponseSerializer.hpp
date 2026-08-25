#ifndef RESPONSESERIALIZER_HPP
#define RESPONSESERIALIZER_HPP

#include "HttpResponse.hpp"
#include <string>

class ResponseSerializer {
public:
    static std::string serialize(HttpResponse& response);
};

#endif
