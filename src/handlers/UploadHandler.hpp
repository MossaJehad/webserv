#pragma once
#include <string>
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

class UploadHandler {
public:
    UploadHandler();
    ~UploadHandler();

    void handle(const ServerConfig& srv,
                const LocationConfig& loc,
                const HttpRequest& req,
                HttpResponse& resp);

private:
    bool handleMultipart(const std::string& boundary,
                         const LocationConfig& loc,
                         const HttpRequest& req,
                         HttpResponse& resp);

    bool handleRaw(const LocationConfig& loc,
                   const HttpRequest& req,
                   HttpResponse& resp);

    std::string extractFilename(const std::string& contentDisposition) const;
    bool writeFile(const std::string& path, const std::string& data) const;
    bool ensureDirectory(const std::string& path) const;
};
