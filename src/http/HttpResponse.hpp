#pragma once
#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void setStatus(int code, const std::string& msg = "");
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body, const std::string& contentType = "text/html");
    void setBodyRaw(const std::string& body);
    void appendBody(const std::string& chunk);

    std::string build() const;
    bool isReady() const;

    static std::string statusMessage(int code);
    static std::string mimeType(const std::string& path);
    static std::string buildError(int code, const std::string& customPage = "");

    int getStatusCode() const;

private:
    int _statusCode;
    std::string _statusMsg;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _ready;
};
