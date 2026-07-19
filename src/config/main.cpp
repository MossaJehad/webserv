#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <exception>
#include <cstdlib>
#include "configParser.hpp"

void print_parsed_data(const std::vector<ServerConfig>& servers) {
    std::cout << "\n========= PARSED CONFIGURATION RESULTS =========\n\n";
    std::cout << "Total Servers Found: " << servers.size() << "\n\n";

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& server = servers[i];
        std::cout << "-------------------------------------------\n";
        std::cout << "SERVER [" << i << "]\n";
        std::cout << "-------------------------------------------\n";
        std::cout << "  Listen Port : " << server.listen_port << "\n";
        std::cout << "  Host        : " << server.host << "\n";
        std::cout << "  Max Body Size: " << server.client_max_body_size << " Bytes\n";
        
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

        std::cout << "  Locations (" << server.locations.size() << "):\n";
        for (size_t k = 0; k < server.locations.size(); ++k) {
            const LocationConfig& loc = server.locations[k];
            std::cout << "    * Path: '" << loc.path << "'\n";
            std::cout << "      Root           : " << loc.root << "\n";
            std::cout << "      Index Files    : ";
            for (size_t m = 0; m < loc.index.size(); ++m) {
                std::cout << loc.index[m] << " ";
            }
            std::cout << "\n";            
            std::cout << "      Autoindex      : " << (loc.autoindex ? "on" : "off") << "\n";
            
            std::cout << "      Allowed Methods: ";
            for (size_t m = 0; m < loc.allowed_methods.size(); ++m) {
                std::cout << loc.allowed_methods[m] << " ";
            }
            std::cout << "\n";
            
            if (loc.return_code != 0) {
                std::cout << "      Return/Redirect: " << loc.return_code << " -> " << loc.return_url << "\n";
            }
            if (!loc.cgi_ext.empty()) {
                std::cout << "      CGI Extension  : " << loc.cgi_ext << "\n";
                std::cout << "      CGI Binary Path: " << loc.cgi_path << "\n";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    ConfigParser parser;

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>\n";
        return 1;
    }
    
    std::string filepath;
    if (argc == 2) {
        filepath = argv[1];
    } else {
        filepath = "test.conf";
    }

    try {
        std::vector<ServerConfig> servers = parser.parse(filepath);

        std::vector<std::string> virtual_lines = parser.get_preprocessed_lines(filepath);
        parser.save_preprocessed_file("preprocessed_output.conf", virtual_lines);

        std::cout << "--- Parsing Verification Successful! ---\n";
        print_parsed_data(servers);
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl; 
        return 1;
    }

    return 0;
}