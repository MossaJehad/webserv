#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig {
public:
    std::string              path;             
    std::string              root;             
    std::vector<std::string> index;            
    bool                     autoindex;        
    std::vector<std::string> allowed_methods;  
    
    int                      return_code;      
    std::string              return_url;       

    std::string              cgi_ext;          
    std::string              cgi_path;         

    LocationConfig();
};

class ServerConfig {
public:
    int                         listen_port;          
    std::string                 host;                 
    std::vector<std::string>    server_names;         
    size_t                      client_max_body_size; 
    std::map<int, std::string>  error_pages;
    std::vector<LocationConfig> locations;            

    ServerConfig();
};

#endif