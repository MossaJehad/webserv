#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include "ConfigTypes.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

class LocationConfig {
private:
    std::string _path;
    std::string _root;
    std::string _index;
    bool _autoindex;
    int _allowedMethods; // bitmask of HttpMethod
    int _redirectCode;
    std::string _redirectUrl;
    std::string _uploadDir;
    std::map<std::string, std::string> _cgiPass; // .ext -> interpreter_path (or empty string if direct executable)
    size_t _clientMaxBodySize;
    bool _hasBodySizeLimit;

public:
    LocationConfig();
    ~LocationConfig();

    const std::string& getPath() const;
    void setPath(const std::string& path);

    const std::string& getRoot() const;
    void setRoot(const std::string& root);

    const std::string& getIndex() const;
    void setIndex(const std::string& index);

    bool getAutoindex() const;
    void setAutoindex(bool autoindex);

    int getAllowedMethods() const;
    void setAllowedMethods(int methods);
    void addAllowedMethod(HttpMethod method);
    bool isMethodAllowed(HttpMethod method) const;

    int getRedirectCode() const;
    const std::string& getRedirectUrl() const;
    void setRedirect(int code, const std::string& url);
    bool hasRedirect() const;

    const std::string& getUploadDir() const;
    void setUploadDir(const std::string& uploadDir);
    bool allowsUpload() const;

    const std::map<std::string, std::string>& getCgiPass() const;
    void addCgiHandler(const std::string& ext, const std::string& binPath);
    bool isCgiExtension(const std::string& ext) const;
    std::string getCgiBin(const std::string& ext) const;

    size_t getClientMaxBodySize() const;
    void setClientMaxBodySize(size_t size);
    bool hasBodySizeLimit() const;
};

#endif
