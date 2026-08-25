#include "ConfigValidator.hpp"
#include "Exceptions.hpp"
#include "FileSystem.hpp"
#include "StringUtils.hpp"
#include <set>

static std::string detectInterpreter(const std::string& ext) {
    if (ext == ".py") {
        if (FileSystem::exists("/usr/bin/python3")) return "/usr/bin/python3";
        if (FileSystem::exists("/usr/bin/python")) return "/usr/bin/python";
    } else if (ext == ".sh") {
        if (FileSystem::exists("/bin/bash")) return "/bin/bash";
        if (FileSystem::exists("/bin/sh")) return "/bin/sh";
    } else if (ext == ".php") {
        if (FileSystem::exists("/usr/bin/php-cgi")) return "/usr/bin/php-cgi";
        if (FileSystem::exists("/usr/bin/php")) return "/usr/bin/php";
    }
    return "";
}

void ConfigValidator::validate(std::vector<ServerConfig>& servers) {
    if (servers.empty()) {
        throw ConfigError("No server configurations found");
    }

    for (size_t s = 0; s < servers.size(); ++s) {
        ServerConfig& server = servers[s];

        if (server.getHost().empty()) {
            server.setHost(DEFAULT_HOST);
        }

        if (server.getPorts().empty()) {
            server.addPort(DEFAULT_PORT);
        }

        if (server.getRoot().empty()) {
            server.setRoot(DEFAULT_ROOT);
        }

        if (server.getIndex().empty()) {
            server.setIndex(DEFAULT_INDEX);
        }

        if (server.getClientMaxBodySize() == 0) {
            server.setClientMaxBodySize(DEFAULT_CLIENT_MAX_BODY_SIZE);
        }

        // If no locations, add default root location
        if (server.getLocations().empty()) {
            LocationConfig rootLoc;
            rootLoc.setPath("/");
            rootLoc.setRoot(server.getRoot());
            rootLoc.setIndex(server.getIndex());
            rootLoc.addAllowedMethod(METHOD_GET);
            rootLoc.addAllowedMethod(METHOD_POST);
            rootLoc.addAllowedMethod(METHOD_DELETE);
            server.addLocation(rootLoc);
        }

        // Validate and fill defaults for each location
        for (size_t l = 0; l < server.getLocations().size(); ++l) {
            LocationConfig& loc = server.getLocations()[l];

            if (loc.getPath().empty()) {
                throw ConfigError("Location block path cannot be empty");
            }

            if (loc.getRoot().empty()) {
                loc.setRoot(server.getRoot());
            }

            if (loc.getIndex().empty()) {
                loc.setIndex(server.getIndex());
            }

            if (loc.getAllowedMethods() == 0) {
                loc.addAllowedMethod(METHOD_GET);
                loc.addAllowedMethod(METHOD_POST);
                loc.addAllowedMethod(METHOD_DELETE);
            }

            // Fill default CGI interpreters if binary path was not explicitly set
            const std::map<std::string, std::string>& cgiMap = loc.getCgiPass();
            std::map<std::string, std::string> updatedCgi = cgiMap;
            for (std::map<std::string, std::string>::const_iterator it = cgiMap.begin(); it != cgiMap.end(); ++it) {
                if (it->second.empty()) {
                    std::string autoBin = detectInterpreter(it->first);
                    if (!autoBin.empty()) {
                        updatedCgi[it->first] = autoBin;
                    }
                }
            }
            for (std::map<std::string, std::string>::const_iterator it = updatedCgi.begin(); it != updatedCgi.end(); ++it) {
                loc.addCgiHandler(it->first, it->second);
            }
        }
    }
}
