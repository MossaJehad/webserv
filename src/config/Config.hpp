#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

// كلاس إعدادات المسارات الفرعية (Location Block)
class LocationConfig {
public:
    std::string              path;             // الرابط المستهدف (مثل "/uploads")
    std::string              root;             // المجلد الحقيقي على القرص (مثل "./www")
    std::string              index;            // الملف الافتراضي (مثل "index.html")
    bool                     autoindex;        // استعراض المجلدات (true/false)
    std::vector<std::string> allowed_methods;  // الطرق المسموحة ("GET", "POST", "DELETE")
    
    // إعادة التوجيه (Redirection)
    int                      return_code;      // رقم حالة إعادة التوجيه (مثل 301)
    std::string              return_url;       // الرابط الجديد للاستبدال

    // إعدادات تشغيل السكربتات الديناميكية (CGI)
    std::string              cgi_ext;          // امتداد ملف الـ CGI (مثل ".py")
    std::string              cgi_path;         // مسار مفسر اللغة (مثل "/usr/bin/python3")

    LocationConfig();
};

// كلاس إعدادات السيرفر العام (Server Block)
class ServerConfig {
public:
    int                               listen_port;          // بورت الاستماع (مثل 8080)
    std::string                       host;                 // عنوان الـ IP الخاص بالسيرفر
    std::vector<std::string>          server_names;         // أسماء النطاقات (الدومينات)
    size_t                            client_max_body_size; // الحد الأقصى لحجم المرفوعات بالبايت
    
    // خريطة لربط رقم الخطأ بصفحة HTML المخصصة (مثال: 404 -> "/errors/404.html")
    std::map<int, std::string>        error_pages;

    // فيكتور يحتوي على كل الـ Locations التابعة لهذا السيرفر
    std::vector<LocationConfig>       locations;

    ServerConfig();
};

#endif