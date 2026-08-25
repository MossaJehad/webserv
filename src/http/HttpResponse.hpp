#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpStatus.hpp"
#include "HttpHeaders.hpp"
#include <string>

class HttpResponse {
private:
    int _statusCode;
    std::string _reasonPhrase;
    HttpHeaders _headers;
    std::string _body;
    bool _keepAlive;
    bool _isRaw;

public:
    HttpResponse();
    explicit HttpResponse(int statusCode);
    ~HttpResponse();

    int getStatusCode() const;
    void setStatusCode(int statusCode);

    const std::string& getReasonPhrase() const;
    void setReasonPhrase(const std::string& reasonPhrase);

    const HttpHeaders& getHeaders() const;
    HttpHeaders& getHeaders();

    const std::string& getBody() const;
    void setBody(const std::string& body);
    void appendBody(const std::string& data);

    bool isKeepAlive() const;
    void setKeepAlive(bool keepAlive);

    bool isRaw() const;
    void setIsRaw(bool isRaw);

    void setContentType(const std::string& mimeType);
    void setContentLength(size_t length);
    void setCookie(const std::string& name, const std::string& value, const std::string& path = "/", int maxAge = -1);

    static HttpResponse redirect(const std::string& location, int code = 302);
};

#endif
