#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "configParser.hpp"



void print_parsed_data(const std::vector<ServerConfig>& servers) {
    std::cout << "========= PARSED CONFIGURATION RESULTS =========\n\n";
    std::cout << "Total Servers Found: " << servers.size() << "\n\n";

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& server = servers[i];
        std::cout << "-------------------------------------------\n";
        std::cout << "SERVER [" << i << "]\n";
        std::cout << "-------------------------------------------\n";
        std::cout << "  Listen Port : " << server.listen_port << "\n";
        
        std::cout << "  Server Names: ";
        for (size_t j = 0; j < server.server_names.size(); ++j) {
            std::cout << server.server_names[j] << " ";
        }
        std::cout << "\n";

        std::cout << "  Error Pages :\n";
        std::map<int, std::string>::const_iterator err_it;
        for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
            std::cout << "    Code " << err_it->first << " -> " << err_it->second << "\n";
        }

        // Print Locations
        std::cout << "  Locations (" << server.locations.size() << "):\n";
        for (size_t k = 0; k < server.locations.size(); ++k) {
            const LocationConfig& loc = server.locations[k];
            std::cout << "    * Path: '" << loc.path << "'\n";
            
            // Methods
            std::cout << "      Allowed Methods: ";
            for (size_t m = 0; m < loc.allowed_methods.size(); ++m) {
                std::cout << loc.allowed_methods[m] << " ";
            }
            std::cout << "\n";
            
            // Autoindex
            std::cout << "      Autoindex      : " << (loc.autoindex ? "on" : "off") << "\n";
            
            // HTTP Redirection
            if (loc.return_code != 0) {
                std::cout << "      Return/Redirect: " << loc.return_code << " -> " << loc.return_url << "\n";
            }
            
            // CGI Setup
            if (!loc.cgi_ext.empty()) {
                std::cout << "      CGI Extension  : " << loc.cgi_ext << "\n";
                std::cout << "      CGI Binary Path: " << loc.cgi_path << "\n";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

#include "configParser.hpp"
#include <iostream>
#include <vector>
#include <exception>
#include <cstdlib>
#include "configParser.hpp"
// #include "Server.hpp"
// #include "Location.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
int main() {
    ConfigParser parser;

    try {
        // نضع عملية القراءة داخل كمين الـ try
        std::vector<ServerConfig> servers = parser.parse("test.conf");

        std::cout << "--- Parsing Verification Successful! ---\n";
        std::cout << "Total Virtual Servers Found: " << servers.size() << "\n";
        for (size_t i = 0; i < servers.size(); ++i) {
            std::cout << "Server [" << i << "] Listening on Port: " << servers[i].listen_port << "\n";
        }
    } 
    catch (const std::exception& e) {
        // هنا نمسك الخطأ المنبعث من الـ Parser ونطبعه بأدب وبدون انهيار البرنامج
        std::cerr << "Process_server_directive-> " << e.what() << std::endl;
        return 1;
    }

    return 0;
}