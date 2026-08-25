#ifndef CGIENVIRONMENT_HPP
#define CGIENVIRONMENT_HPP

#include "RequestContext.hpp"
#include <string>
#include <vector>
#include <map>

class CgiEnvironment {
private:
    std::map<std::string, std::string> _envMap;

public:
    CgiEnvironment();
    ~CgiEnvironment();

    void build(const RequestContext& ctx);
    char** createEnvp() const;
    static void freeEnvp(char** envp);
    const std::map<std::string, std::string>& getMap() const;
};

#endif
