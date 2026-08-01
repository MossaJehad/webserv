 #include "Config.hpp"

LocationConfig::LocationConfig() 
    : path(""), 
      root(""), 
      autoindex(false),
      return_code(0),
      return_url(""), 
      cgi_ext(""), 
      cgi_path("") 
{
    // القيمة الافتراضية القياسية داخل الفيكتور
    index.push_back("index.html");
    allowed_methods.push_back("GET");
}

// تهيئة إعدادات السيرفر الأب بقيم قياسية
ServerConfig::ServerConfig() 
    : listen_port(80),               // البورت الافتراضي للمواقع هو 80
      host("0.0.0.0"),               // الاستماع لجميع كروت الشبكة
      client_max_body_size(1048576)  // الحجم الأقصى الافتراضي 1 ميجابايت
{
    server_names.push_back("localhost");
}