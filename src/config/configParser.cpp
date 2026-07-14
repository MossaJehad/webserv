#include "configParser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdexcept>

using namespace std;

bool ConfigParser::is_valid_client_max_body_size(const std::string& size_str) {
	if (size_str.empty()) {
		return false;
	}
	for (size_t i = 0; i < size_str.length(); ++i) {
		if (!std::isdigit(size_str[i])) {
			return false;
		}
	}
	return true;
}

bool ConfigParser::is_valid_host(const std::string& host) {
    if (host == "localhost") {
        return true;
    }
    if (host.empty() || host[host.length() - 1] == '.') {
        return false;
    }

    std::stringstream ss(host);
    std::string segment;
    int segment_count = 0;

    while (std::getline(ss, segment, '.')) {
        segment_count++;
        if (segment.empty() || segment.length() > 3) {
            return false;
        }
        for (size_t i = 0; i < segment.length(); ++i) {
            if (!std::isdigit(segment[i])) {
                return false;
            }
        }
        int val = std::atoi(segment.c_str());
        if (val < 0 || val > 255) {
            return false;
        }
    }
    return (segment_count == 4);
}

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
		std::string extra;
		ss >> val;
		val = strip_semicolon(val);

		if (val.empty() || !is_valid_client_max_body_size(val)) {
			throw std::runtime_error("Invalid listen directive: missing or invalid port number.");
		}
		if (ss >> extra && extra != ";")
			throw std::runtime_error("Invalid listen directive: too many arguments.");
		int port = std::atoi(val.c_str());
		if (port > 0 && port <= 65535) {
			server.listen_port = port;
		} else {
 			throw std::runtime_error("Port number out of range (1-65535): " + val);
		}
	} else if (key == "host") {
		std::string val;
		ss >> val;
		std::string extra;
		if (ss >> extra && extra != ";") {
			throw std::runtime_error("host directive should only have one argument.");
		}
		val = strip_semicolon(val);
		if (!is_valid_host(val)) {
			throw std::runtime_error("Invalid host value: " + val);
		}
		server.host = val;
	} else if (key == "client_max_body_size") {
    	std::string val;
    	ss >> val;
		std::string extra;
		if (ss >> extra && extra != ";") {
			throw std::runtime_error("client_max_body_size directive should only have one argument.");
		}
    	val = strip_semicolon(val);
    	if (val.empty()) {
    	    throw std::runtime_error("client_max_body_size directive cannot be empty.");
    	}

    	size_t multiplier = 1; 
    	char last_char = val[val.length() - 1];

	    if (last_char == 'K' || last_char == 'k') {
	        multiplier = 1024;
	        val = val.substr(0, val.length() - 1);
	    } else if (last_char == 'M' || last_char == 'm') {
	        multiplier = 1024 * 1024;
	        val = val.substr(0, val.length() - 1);
	    } else if (last_char == 'G' || last_char == 'g') {
	        multiplier = 1024 * 1024 * 1024;
	        val = val.substr(0, val.length() - 1);
	    }

	    for (size_t i = 0; i < val.length(); i++) {
	        if (!std::isdigit(val[i])) {
	            throw std::runtime_error("Invalid client_max_body_size value: " + val);
	        }
	    }
		errno = 0;
		char* end_ptr;
		unsigned long size_value = std::strtoul(val.c_str(), &end_ptr, 10);
		if (*end_ptr != '\0') {
	        throw std::runtime_error("Invalid client_max_body_size value: " + val);
	    }
		unsigned long max_size = 4294967295UL / multiplier;
		if (errno == ERANGE || size_value > max_size) {
 		   throw std::runtime_error("client_max_body_size is out of range (Too large).");
		}
	    server.client_max_body_size = static_cast<size_t>(size_value * multiplier);
	}else if (key == "server_name") {
		std::string val;
		server.server_names.clear();
		while (ss >> val) {
			val = strip_semicolon(val);
            if (val.empty()) continue;
			server.server_names.push_back(val);
		}
	} else if (key == "error_page") { 
	    std::string token;
	    std::vector<std::string> tokens;

		while (ss >> token) {
	        token = strip_semicolon(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
	    }
        
	    if (tokens.size() < 2) {
	        throw std::runtime_error("Invalid error_page directive: missing code or path.");
	    }
	    std::string error_path = tokens.back();
	    for (size_t i = 0; i < tokens.size() - 1; ++i) {
	        std::string code_str = tokens[i];
	        for (size_t j = 0; j < code_str.length(); ++j) {
	            if (!std::isdigit(code_str[j])) {
	                throw std::runtime_error("Invalid error_page code (must be numeric): " + code_str);
	            }
	        }
	        int code = std::atoi(code_str.c_str());
	        if (code < 400 || code > 599) {
	            throw std::runtime_error("Error code out of range (400-599): " + code_str);
	        }
	        server.error_pages[code] = error_path;
	    }
	} else {
		// حماية ضد الأوامر المجهولة داخل السيرفر
		throw std::runtime_error("Unknown server directive found: '" + key + "'");
	}
}

void ConfigParser::process_location_directive(LocationConfig& location, const std::string& key, std::string& line_content) {
	std::stringstream ss(line_content);
	if (key == "root") {
		std::string val;
		ss >> val;
		val = strip_semicolon(val);
		location.root = val;

		if (location.root.empty()) {
			throw std::runtime_error("root directive cannot be empty.");
		}
	} else if (key == "index") {
		std::string val;
		location.index.clear(); 
		while (ss >> val) {
			val = strip_semicolon(val);
			if (val.empty()) continue; 
			location.index.push_back(val);
		}
		if (location.index.empty()) {
			throw std::runtime_error("index directive cannot be empty.");
		}
	} else if (key == "autoindex") {
    	std::string val;
		std::string extra;
    	ss >> val;
		
		if (ss >> extra && extra != ";") {
			throw std::runtime_error("autoindex directive should only have one argument: 'on' or 'off'.");
		}
    	val = strip_semicolon(val);
    	if (val == "on") location.autoindex = true;
    	else if (val == "off") location.autoindex = false;
    	else {
			throw std::runtime_error("autoindex must be 'on' or 'off'");
    	}
    } else if (key == "allow_methods") {
        std::string method;
        location.allowed_methods.clear();
        while (ss >> method) {
            method = strip_semicolon(method);
			if (method.empty()) continue;
            if (!is_valid_method(method)) {
				throw std::runtime_error("Unsupported or invalid HTTP method found: " + method);
            }
            location.allowed_methods.push_back(method);
        }
		if (location.allowed_methods.empty()) {
			throw std::runtime_error("allow_methods directive cannot be empty.");
		}
    } else if (key == "return") {
        std::string code_str, url;
        ss >> code_str >> url;
        url = strip_semicolon(url);
        location.return_code = std::atoi(code_str.c_str());
        location.return_url = url;
    } else if (key == "cgi_ext") {
        std::string val;
        ss >> val;
        location.cgi_ext = strip_semicolon(val);
    } else if (key == "cgi_path") {
        std::string val;
        ss >> val;
        location.cgi_path = strip_semicolon(val);
    } else {
		throw std::runtime_error("Unknown location directive found: '" + key + "'");
	}
}std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath) {
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

        // 1. معالجة بلوك السيرفر المرنة (تتخطى أي أسطر فارغة وفراغات قبل القوس)
        if (line == "server" || line == "server {") {
            if (line == "server") {
                std::string next_line;
                bool found_brace = false;
                while (std::getline(file, next_line)) {
                    next_line = trim(next_line);
                    if (next_line.empty()) continue; // تخطي الأسطر الفارغة المتروكة
                    if (next_line != "{") {
                        throw std::runtime_error("Syntax Error: Expected '{' after 'server'.");
                    }
                    found_brace = true;
                    break;
                }
                if (!found_brace) {
                    throw std::runtime_error("Syntax Error: Expected '{' after 'server' but reached EOF.");
                }
            }
            if (in_server) {
                throw std::runtime_error("Direct nesting of server blocks is illegal!");
            }
            in_server = true;
            current_server = ServerConfig();
            continue;
        } 
        
        // 2. معالجة بلوك اللوكيشن المرنة (تتخطى الأسطر الفارغة، وتحمي المسارات الغائبة والمشوهة)
        else if (line.find("location") == 0) {
            if (!in_server) {
                throw std::runtime_error("Location blocks must reside inside a server scope!");
            }
            if (in_location) {
                throw std::runtime_error("Direct nesting of location blocks is illegal!");
            }

            // لو القوس مش على نفس السطر، ندخل في حلقة لتخطي الفراغات والأسطر الفاضية
            if (line.find("{") == std::string::npos) {
                std::string next_line;
                bool found_brace = false;
                while (std::getline(file, next_line)) {
                    next_line = trim(next_line);
                    if (next_line.empty()) continue; // تخطي الأسطر الفارغة المتروكة عمداً
                    if (next_line != "{") {
                        throw std::runtime_error("Syntax Error: Expected '{' after location path.");
                    }
                    found_brace = true;
                    break;
                }
                if (!found_brace) {
                    throw std::runtime_error("Syntax Error: Expected '{' after location path but reached EOF.");
                }
            }

            in_location = true;
            current_location = LocationConfig();

            std::stringstream ss(line);
            std::string dummy, path_str;
            ss >> dummy;    // سحب كلمة "location"
            ss >> path_str; // سحب المسار

            // تنظيف أي قوس ملتصق بالمسار إذا وُجد مثل /cgi-bin{
            if (!path_str.empty() && path_str[path_str.length() - 1] == '{') {
                path_str = path_str.substr(0, path_str.length() - 1);
            }
            path_str = trim(path_str);

            // الحماية المطلقة: لو غاب المسار تماماً وكان مكانه قوس أو نص فارغ
            if (path_str.empty() || path_str == "{") {
                throw std::runtime_error("Syntax Error: Location block is missing a path definition.");
            }

            current_location.path = path_str;
            continue;
        } 
        
        // 3. معالجة قوس الإغلاق وحماية الـ CGI المتلازم
        else if (line == "}") {
            if (in_location) {
                if ((current_location.cgi_ext.empty() && !current_location.cgi_path.empty()) || 
                    (!current_location.cgi_ext.empty() && current_location.cgi_path.empty())) {
                    throw std::runtime_error("CGI Validation Error in location '" + current_location.path + "': Both cgi_ext and cgi_path must be provided together.");
                }
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

        // 4. حارس الفاصلة المنقوطة الإلزامية (فقط للـ Directives والتعليمات العادية)
        if (line[line.length() - 1] != ';') {
            throw std::runtime_error("Syntax Error: Directive line must end with a semicolon ';': -> " + line);
        }

        // 5. تقطيع وتوجيه السطور على الـ Directives المناسبة
        std::stringstream ss(line);
        std::string key;
        ss >> key;

        std::string line_content = line.substr(line.find(key) + key.length());
        line_content = trim(line_content);

        if (in_location) {
            process_location_directive(current_location, key, line_content);
        } else if (in_server) {
            process_server_directive(current_server, key, line_content);
        } else {
            throw std::runtime_error("Syntax Error: Directive found outside of any server block: " + line);
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