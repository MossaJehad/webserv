#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include <string>
#include <vector>
#include <map>

class HttpHeaders {
private:
    // Store original casing for output, but index by lowercase for lookup
    std::map<std::string, std::string> _headers; // lowercase_name -> value
    std::vector<std::pair<std::string, std::string> > _headerList; // original name, value
    std::vector<std::string> _cookies; // Cookie headers

public:
    HttpHeaders();
    ~HttpHeaders();

    void set(const std::string& name, const std::string& value);
    void add(const std::string& name, const std::string& value);
    std::string get(const std::string& name) const;
    bool has(const std::string& name) const;
    void remove(const std::string& name);
    void clear();

    const std::vector<std::pair<std::string, std::string> >& getList() const;
    const std::map<std::string, std::string>& getMap() const;
    const std::vector<std::string>& getCookies() const;
};

#endif
