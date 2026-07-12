#include "configParser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>    //    الحل الحاسم للـ std::atoi
#include <stdexcept>  //    الحل الحاسم للـ throw std::runtime_error

using namespace std;

std::string ConfigParser::trim(const std::string& str){
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, (last - first + 1));
}

std::string ConfigParser::strip_semicolon(const std::string& str){
	std::string trimStr = trim(str);
	if (!trimStr.empty() && trimStr.length() > 0 && trimStr[trimStr.size() - 1] == ';') {
		return trimStr.substr(0, trimStr.size() - 1);
	}
	return trimStr;
}

bool ConfigParser::is_valid_method(const std::string& method) {
	return (method == "GET" || method == "POST" || method == "DELETE");
}

void ConfigParser::process_server_directive(ServerConfig& server, const std::string& key, std::string& line_content) {
	std::stringstream ss(line_content);
	if (key == "listen") {
		std::string val;
		ss >> val;

		val = strip_semicolon(val);
		if (val.empty()) {
			throw std::runtime_error("Missing port number after 'listen' directive.");
		}
		for (size_t i = 0; i < val.length(); i++)
		{
			if (!std::isdigit(val[i])) {
				std::cerr << "Process_server_directive-> Invalid port number: " << val << std::endl;
				throw std::runtime_error("Invalid port number (must be numeric): " + val);
			}
		}
		int port = std::atoi(val.c_str());
		if (port > 0 && port <= 65535) {
			server.listen_port = port;
		}
		else {
 			throw std::runtime_error("Port number out of range (0-65535): " + val);
		}
	} else if (key == "host") {
		std::string val;
		ss >> val;
		val = strip_semicolon(val);
		server.host = val;
	} else if (key == "client_max_body_size") {
		std::string val;
		ss >> val;

		val = strip_semicolon(val);
		for (size_t i = 0; i < val.length(); i++) {
			if (!std::isdigit(val[i])) {
				throw std::runtime_error("client_max_body_size must be numeric: " + val);
			}
		}
		server.client_max_body_size = std::atoi(val.c_str());
	}
	else if (key == "server_name") {
		std::string val;
		server.server_names.clear();
		while (ss >> val) {
			val = strip_semicolon(val);
			server.server_names.push_back(val);
		}
	}
	else if (key == "error_page") {	
		std::string val_code, val_path;
		if (ss >> val_code >> val_path) {
			val_code = strip_semicolon(val_code);
			val_path = strip_semicolon(val_path);
			
			for (size_t i = 0; i < val_code.length(); i++) {
				if (!std::isdigit(val_code[i])) {
					throw std::runtime_error("Error code must be numeric: " + val_code);
				}
			}
			int code = std::atoi(val_code.c_str());
			server.error_pages[code] = val_path;
		}
	}
}

void ConfigParser::process_location_directive(LocationConfig& location, const std::string& key, std::string& line_content) 
{
	std::stringstream ss(line_content);
	if (key == "root") {
		std::string val;
		ss >> val;
		val = strip_semicolon(val);
		location.root = val;
	}
	else if (key == "index") {
		std::string val;
		ss >> val;
		val = strip_semicolon(val);
		location.index = val;
	}
	else if (key == "autoindex") {
    	std::string val;
    	ss >> val;
    	val = strip_semicolon(val);
    	if (val == "on") location.autoindex = true;
    	else if (val == "off") location.autoindex = false;
    	else {
			throw std::runtime_error("autoindex must be 'on' or 'off'");
    	}
    }
    else if (key == "allow_methods") {
        std::string method;
        location.allowed_methods.clear();
        while (ss >> method) {
            method = strip_semicolon(method);
            if (!is_valid_method(method)) {
				//    تعديل الـ Valgrind الآمن بدلاً من exit(1)
				throw std::runtime_error("Unsupported or invalid HTTP method found: " + method);
            }
            location.allowed_methods.push_back(method);
        }
    }
    else if (key == "return") {
        std::string code_str, url;
        ss >> code_str >> url;
        url = strip_semicolon(url);
        location.return_code = std::atoi(code_str.c_str());
        location.return_url = url;
    }
    else if (key == "cgi_ext") {
        std::string val;
        ss >> val;
        location.cgi_ext = strip_semicolon(val);
    }
    else if (key == "cgi_path") {
        std::string val;
        ss >> val;
        location.cgi_path = strip_semicolon(val);
    }
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath) {
    std::vector<ServerConfig> parsed_servers;
    std::ifstream file(filepath.c_str());

    if (!file.is_open()) {
		throw std::runtime_error("Critical Error: Could not open configuration file at: " + filepath);
    }

    std::string line;
    ServerConfig current_server;
    LocationConfig current_location;

    bool in_server = false;
    bool in_location = false;

    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        line = trim(line);

        if (line.empty()) continue;

        if (line == "server {") {
            if (in_server) {
				throw std::runtime_error("Direct nesting of server blocks is illegal!");
            }
            in_server = true;
            current_server = ServerConfig();
            continue;
        }
        else if (line.find("location") == 0 && line.find("{") != std::string::npos) {
            if (!in_server) {
				throw std::runtime_error("Location blocks must reside inside a server scope!");
            }
            in_location = true;
            current_location = LocationConfig();

            std::stringstream ss(line);
            std::string dummy, path_str;
            ss >> dummy >> path_str;
            current_location.path = path_str;
            continue;
        }
        else if (line == "}") {
            if (in_location) {
                current_server.locations.push_back(current_location);
                in_location = false;
            } else if (in_server) {
                parsed_servers.push_back(current_server);
                in_server = false;
            } else {
				throw std::runtime_error("Unmatched closing bracket '}' found.");
            }
            continue;
        }

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        std::string line_content = line.substr(line.find(key) + key.length());
        line_content = trim(line_content);

        if (in_location) {
            process_location_directive(current_location, key, line_content);
        } 
        else if (in_server) {
            process_server_directive(current_server, key, line_content);
        }
    }

    file.close();

    if (in_server || in_location) {
		throw std::runtime_error("Parser reached EOF, but braces are still open!");
    }

    return parsed_servers;
}

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}