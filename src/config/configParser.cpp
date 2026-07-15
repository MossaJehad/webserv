#include "configParser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <cerrno>
#include <cctype> 

std::vector<std::string> ConfigParser::preprocess_file(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Critical Error: Could not open configuration file at: " + filepath);
    }

    std::string raw_content;
    std::string line;
    
    // 1. قراءة الملف بالكامل وتطهيره من التعليقات (#) فوراً
    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        // دمج السطور مع ترك مسافة أمان تمنع التصاق الأوامر بين نهاية سطر وبداية آخر
        raw_content += line + " "; 
    }
    file.close();

    std::vector<std::string> virtual_lines;
    std::string current_chunk = "";

    // 2. تفتيت النص برمجياً بناءً على الرموز الحاكمة (Tokenizer)
    for (size_t i = 0; i < raw_content.length(); ++i) {
        char ch = raw_content[i];

        if (ch == ';') {
            std::string trimmed = trim(current_chunk);
            if (!trimmed.empty()) {
                virtual_lines.push_back(trimmed + ";");
            }
            current_chunk = "";
        } else if (ch == '{') {
            std::string trimmed = trim(current_chunk);
            if (!trimmed.empty()) {
                virtual_lines.push_back(trimmed + " {");
            } else {
                virtual_lines.push_back("{");
            }
            current_chunk = "";
        } else if (ch == '}') {
            // للتأكد من عدم وجود أمر تائه بدون فاصلة منقوطة قبل قوس الإغلاق
            std::string trimmed = trim(current_chunk);
            if (!trimmed.empty()) {
                throw std::runtime_error("Syntax Error: Missing semicolon ';' before '}' near: " + trimmed);
            }
            virtual_lines.push_back("}");
            current_chunk = "";
        } else {
            current_chunk += ch;
        }
    }

    // 3. فحص وجود أي نصوص زائدة في نهاية الملف لم تُغلق بـ ; أو }
    std::string trimmed_leftover = trim(current_chunk);
    if (!trimmed_leftover.empty()) {
        throw std::runtime_error("Syntax Error: Unexpected EOF or missing closing delimiter ';' near: " + trimmed_leftover);
    }

    return virtual_lines;
}

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
		if (val.length() > 5) {
			throw std::runtime_error("Port number out of range (1-65535): " + val);
		}
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
 
		if (val.empty()) {
    		throw std::runtime_error("Invalid client_max_body_size value (missing digits).");
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
	} else if (key == "server_name") {
    std::string val;
    server.server_names.clear();
    while (ss >> val) {
        val = strip_semicolon(val);
        if (val.empty()) continue;
        server.server_names.push_back(val);
    }
    if (server.server_names.empty()) {
        throw std::runtime_error("server_name directive cannot be empty.");
    }
}
 else if (key == "error_page") { 
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
			if (code_str.length() > 3) {
				throw std::runtime_error("Error code out of range (400-599): " + code_str);
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

        // [تأمين وحل الثغرة]: فحص النفايات الخلفية للـ root
        std::string extra;
        if (ss >> extra) {
            throw std::runtime_error("root directive should only have one argument: '" + val + "' followed by '" + extra + "'");
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
		
		if (ss >> extra) {
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
        // =================================================================
        // [تعديل الـ return المحمي والمطابق لـ Nginx بالكامل]
        // =================================================================
        std::string token1, token2;
        ss >> token1;

        if (token1.empty()) {
            throw std::runtime_error("return directive cannot be empty.");
        }

        // تفقد إذا كان هناك معامل ثانٍ في السطر
        if (ss >> token2) {
            std::string extra;
            // حظر النفايات الخلفية والتوكنز الثالثة الزائدة
            if (ss >> extra) {
                throw std::runtime_error("return directive has too many arguments: " + extra);
            }
            token2 = strip_semicolon(token2);

            // الحالة أ: كود متبوع برابط أو نص (مثل return 301 http://amman.com;)
            if (token1.length() > 3) { // أكواد HTTP القياسية تتكون من 3 خانات فقط
                throw std::runtime_error("Invalid HTTP status code length in return directive: " + token1);
            }
            for (size_t i = 0; i < token1.length(); ++i) {
                if (!std::isdigit(token1[i])) {
                    throw std::runtime_error("Invalid HTTP status code in return directive (must be numeric): " + token1);
                }
            }

            int code = std::atoi(token1.c_str());
            if (code < 100 || code > 599) {
                throw std::runtime_error("HTTP status code in return directive is out of range (100-599): " + token1);
            }

            location.return_code = code;
            location.return_url = token2;
        } 
        else {
            // الحالة ب: وجود معامل واحد فقط (كود حالة فقط أو رابط مباشر فقط)
            token1 = strip_semicolon(token1);
            bool is_numeric = true;
            for (size_t i = 0; i < token1.length(); ++i) {
                if (!std::isdigit(token1[i])) {
                    is_numeric = false;
                    break;
                }
            }

            if (is_numeric) {
                // معامل واحد رقمي (مثال: return 404;)
                if (token1.length() > 3) {
                    throw std::runtime_error("Invalid HTTP status code length in return directive: " + token1);
                }
                int code = std::atoi(token1.c_str());
                if (code < 100 || code > 599) {
                    throw std::runtime_error("HTTP status code out of range (100-599): " + token1);
                }
                location.return_code = code;
                location.return_url = "";
            } 
            else {
                // معامل واحد غير رقمي (مثال: return http://google.com;)
                // يجب أن يبدأ الرابط بـ http:// أو https:// أو / ليعتبر مساراً صحيحاً
                if (token1.find("http://") != 0 && token1.find("https://") != 0 && token1.find("/") != 0) {
                    throw std::runtime_error("Invalid return directive argument (must be status code or valid URL): " + token1);
                }
                location.return_code = 302; // الافتراضي التلقائي لـ Nginx عند غياب الكود
                location.return_url = token1;
            }
        }
    }
	else if (key == "cgi_ext") {
        std::string val;
        ss >> val;
        if (val.empty()) {
            throw std::runtime_error("cgi_ext directive cannot be empty.");
        }

        // [تأمين وحل الثغرة]: فحص النفايات الخلفية للـ cgi_ext
        std::string extra;
        if (ss >> extra) {
            throw std::runtime_error("cgi_ext directive should only have one argument: '" + val + "' followed by '" + extra + "'");
        }
        location.cgi_ext = strip_semicolon(val);
    } else if (key == "cgi_path") {
        std::string val;
        ss >> val;
        if (val.empty()) {
            throw std::runtime_error("cgi_path directive cannot be empty.");
        }

        // [تأمين وحل الثغرة]: فحص النفايات الخلفية للـ cgi_path
        std::string extra;
        if (ss >> extra) {
            throw std::runtime_error("cgi_path directive should only have one argument: '" + val + "' followed by '" + extra + "'");
        }
        location.cgi_path = strip_semicolon(val);
    } else {
		throw std::runtime_error("Unknown location directive found: '" + key + "'");
	}
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath) {
    std::vector<ServerConfig> parsed_servers;
    std::vector<std::string> virtual_lines;

    // استدعاء دالة التطهير المسبق وسحب الأسطر الافتراضية
    virtual_lines = preprocess_file(filepath);

    ServerConfig current_server;
    LocationConfig current_location;

    bool in_server = false;
    bool in_location = false;

    // الدوران على الأسطر الافتراضية المجهزة والنظيفة تماماً
    for (size_t line_idx = 0; line_idx < virtual_lines.size(); ++line_idx) {
        std::string line = virtual_lines[line_idx];

        // 1. معالجة بلوك السيرفر
        if (line == "server" || line == "server {") {
            if (line == "server") {
                // الفحص الآمن للسطر التالي في الذاكرة دون الحاجة لعمل getline من الملف
                if (line_idx + 1 < virtual_lines.size() && virtual_lines[line_idx + 1] == "{") {
                    line_idx++; // تخطي قوس الفتح لأنه تمت معالجته
                } else {
                    throw std::runtime_error("Syntax Error: Expected '{' after 'server'.");
                }
            }
            if (in_server) {
                throw std::runtime_error("Direct nesting of server blocks is illegal!");
            }
            in_server = true;
            current_server = ServerConfig();
            continue;
        } 
        
        // 2. معالجة بلوك اللوكيشن
        else if (line.find("location") == 0) {
            if (!in_server) {
                throw std::runtime_error("Location blocks must reside inside a server scope!");
            }
            if (in_location) {
                throw std::runtime_error("Direct nesting of location blocks is illegal!");
            }

            std::stringstream loc_ss(line);
            std::string directive_word, path_str;
            loc_ss >> directive_word;

            if (directive_word != "location") {
                throw std::runtime_error("Unknown directive found: '" + directive_word + "'");
            }

            loc_ss >> path_str;

            // تنظيف القوس الملتصق بالمسار إذا وُجد مثل /cgi-bin{
            if (!path_str.empty() && path_str[path_str.length() - 1] == '{') {
                path_str = path_str.substr(0, path_str.length() - 1);
            }
            path_str = trim(path_str);

            if (path_str.empty() || path_str == "{") {
                throw std::runtime_error("Syntax Error: Location block is missing a path definition.");
            }

            std::string loc_extra;
            if (loc_ss >> loc_extra && loc_extra != "{") {
                throw std::runtime_error("Syntax Error: Unexpected tokens in location definition: " + line);
            }

            // لو كان القوس في السطر التالي
            if (line.find("{") == std::string::npos) {
                if (line_idx + 1 < virtual_lines.size() && virtual_lines[line_idx + 1] == "{") {
                    line_idx++; // تخطي قوس الفتح
                } else {
                    throw std::runtime_error("Syntax Error: Expected '{' after location path.");
                }
            }

            in_location = true;
            current_location = LocationConfig();
            current_location.path = path_str;
            continue;
        } 
        
        // 3. معالجة قوس الإغلاق
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

        // 4. معالجة الـ Directives العادية (والتي نضمن 100% أنها تنتهي بـ ';')
        if (line[line.length() - 1] != ';') {
            throw std::runtime_error("Syntax Error: Directive line must end with a semicolon ';': -> " + line);
        }

        // قص الفاصلة وتجهيز السطر النظيف تماماً للـ Directives
        line = line.substr(0, line.length() - 1);
        line = trim(line);

        if (line.empty()) {
            throw std::runtime_error("Syntax Error: Empty directive line before semicolon.");
        }

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

    if (in_server || in_location) {
        throw std::runtime_error("Parser reached EOF, but braces are still open!");
    }
	if (parsed_servers.empty()) {
        throw std::runtime_error("Configuration Error: No 'server' blocks were defined in the file.");
    }

	for (size_t i = 0; i < parsed_servers.size(); ++i) {
        const ServerConfig& srv = parsed_servers[i];

        // 1. التأكد من أن السيرفر يحتوي على بلوك لوكيشن واحد على الأقل لخدمة الطلبات
        if (srv.locations.empty()) {
            std::stringstream err_ss;
            err_ss << "Configuration Error: Server listening on port " << srv.listen_port 
                   << " has no location blocks defined. At least one location is required.";
            throw std::runtime_error(err_ss.str());
        }

        // 2. فحص كل لوكيشن للتأكد من توفر المجلد الفعلي (root) أو التوجيه (return)
        for (size_t j = 0; j < srv.locations.size(); ++j) {
            const LocationConfig& loc = srv.locations[j];

            // اللوكيشن يجب أن يمتلك إما root لخدمة الملفات، أو return لإعادة التوجيه، أو cgi_path لتشغيل الـ CGI
            if (loc.root.empty() && loc.return_code == 0 && loc.cgi_path.empty()) {
                throw std::runtime_error("Configuration Error: Location '" + loc.path + 
                    "' in server configuration is empty. It must define a 'root' directory, a 'return' redirection, or a 'cgi_path'.");
            }
        }
    }
    return parsed_servers;
}


void ConfigParser::save_preprocessed_file(const std::string& output_filepath, const std::vector<std::string>& virtual_lines) {
    std::ofstream outfile(output_filepath.c_str());
    
    if (!outfile.is_open()) {
        throw std::runtime_error("Test Error: Could not create output file at: " + output_filepath);
    }

    for (size_t i = 0; i < virtual_lines.size(); ++i) {
        outfile << virtual_lines[i] << "\n";
    }

    outfile.close();
    std::cout << "[Test Success] Preprocessed configuration successfully saved to: " << output_filepath << std::endl;
}

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}