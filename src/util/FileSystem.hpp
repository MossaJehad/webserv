#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>
#include <vector>
#include <ctime>

struct DirEntry {
    std::string name;
    bool isDirectory;
    size_t size;
    std::time_t lastModified;
};

class FileSystem {
public:
    static bool exists(const std::string& path);
    static bool isDir(const std::string& path);
    static bool isFile(const std::string& path);
    static bool isReadable(const std::string& path);
    static bool isWritable(const std::string& path);
    static bool isExecutable(const std::string& path);
    static bool readFile(const std::string& path, std::string& content);
    static bool writeFile(const std::string& path, const std::string& content);
    static bool appendFile(const std::string& path, const std::string& content);
    static bool deleteFile(const std::string& path);
    static bool deleteDirRecursive(const std::string& path);
    static bool listDirectory(const std::string& path, std::vector<DirEntry>& entries);
    static size_t getFileSize(const std::string& path);
    static std::time_t getLastModifiedTime(const std::string& path);
    static std::string joinPaths(const std::string& p1, const std::string& p2);
    static std::string normalizePath(const std::string& path);
    static std::string getExtension(const std::string& path);
    static std::string getFilename(const std::string& path);
    static std::string getDirname(const std::string& path);
};

#endif
