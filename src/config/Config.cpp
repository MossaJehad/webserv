#include "Config.hpp"

// تهيئة إعدادات الـ Location الافتراضية
LocationConfig::LocationConfig() 
    : path(""), 
      root(""), 
      index("index.html"), // افتراضياً نبحث عن index.html
      autoindex(false),    // إغلاق استعراض المجلدات حماية للسيرفر
      return_code(0),
      return_url(""), 
      cgi_ext(""), 
      cgi_path("") 
{
    // بشكل افتراضي، مسموح فقط الـ GET إذا ما حدد المستخدم غير هيك
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