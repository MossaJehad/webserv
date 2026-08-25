#include "MimeTypes.hpp"
#include "FileSystem.hpp"
#include "StringUtils.hpp"

std::map<std::string, std::string> MimeTypes::_types;
bool MimeTypes::_initialized = false;

void MimeTypes::init() {
    if (_initialized) return;

    _types[".html"] = "text/html";
    _types[".htm"]  = "text/html";
    _types[".css"]  = "text/css";
    _types[".js"]   = "application/javascript";
    _types[".json"] = "application/json";
    _types[".xml"]  = "application/xml";
    _types[".txt"]  = "text/plain";
    _types[".png"]  = "image/png";
    _types[".jpg"]  = "image/jpeg";
    _types[".jpeg"] = "image/jpeg";
    _types[".gif"]  = "image/gif";
    _types[".svg"]  = "image/svg+xml";
    _types[".ico"]  = "image/x-icon";
    _types[".webp"] = "image/webp";
    _types[".pdf"]  = "application/pdf";
    _types[".zip"]  = "application/zip";
    _types[".tar"]  = "application/x-tar";
    _types[".gz"]   = "application/gzip";
    _types[".mp3"]  = "audio/mpeg";
    _types[".mp4"]  = "video/mp4";
    _types[".webm"] = "video/webm";
    _types[".wav"]  = "audio/wav";
    _types[".bin"]  = "application/octet-stream";

    _initialized = true;
}

std::string MimeTypes::getType(const std::string& path) {
    if (!_initialized) {
        init();
    }
    std::string ext = StringUtils::toLower(FileSystem::getExtension(path));
    std::map<std::string, std::string>::const_iterator it = _types.find(ext);
    if (it != _types.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
