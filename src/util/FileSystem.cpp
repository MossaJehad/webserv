#include "FileSystem.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

bool FileSystem::exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool FileSystem::isDir(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool FileSystem::isFile(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

bool FileSystem::isReadable(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}

bool FileSystem::isWritable(const std::string& path) {
    return access(path.c_str(), W_OK) == 0;
}

bool FileSystem::isExecutable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}

bool FileSystem::readFile(const std::string& path, std::string& content) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }
    content.clear();
    char buf[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
        content.append(buf, bytesRead);
    }
    close(fd);
    return bytesRead >= 0;
}

bool FileSystem::writeFile(const std::string& path, const std::string& content) {
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return false;
    }
    size_t totalWritten = 0;
    while (totalWritten < content.size()) {
        ssize_t written = write(fd, content.data() + totalWritten, content.size() - totalWritten);
        if (written <= 0) {
            close(fd);
            return false;
        }
        totalWritten += written;
    }
    close(fd);
    return true;
}

bool FileSystem::appendFile(const std::string& path, const std::string& content) {
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        return false;
    }
    size_t totalWritten = 0;
    while (totalWritten < content.size()) {
        ssize_t written = write(fd, content.data() + totalWritten, content.size() - totalWritten);
        if (written <= 0) {
            close(fd);
            return false;
        }
        totalWritten += written;
    }
    close(fd);
    return true;
}

bool FileSystem::deleteFile(const std::string& path) {
    return std::remove(path.c_str()) == 0;
}

bool FileSystem::deleteDirRecursive(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }
    struct dirent* entry;
    bool success = true;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string fullPath = joinPaths(path, name);
        if (isDir(fullPath)) {
            if (!deleteDirRecursive(fullPath)) {
                success = false;
            }
        } else {
            if (std::remove(fullPath.c_str()) != 0) {
                success = false;
            }
        }
    }
    closedir(dir);
    if (std::remove(path.c_str()) != 0) {
        success = false;
    }
    return success;
}

static bool compareDirEntries(const DirEntry& a, const DirEntry& b) {
    if (a.isDirectory != b.isDirectory) {
        return a.isDirectory > b.isDirectory; // Directories first
    }
    return a.name < b.name;
}

bool FileSystem::listDirectory(const std::string& path, std::vector<DirEntry>& entries) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }
    entries.clear();
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") {
            continue;
        }
        std::string fullPath = joinPaths(path, name);
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0) {
            DirEntry de;
            de.name = name;
            de.isDirectory = S_ISDIR(st.st_mode);
            de.size = static_cast<size_t>(st.st_size);
            de.lastModified = st.st_mtime;
            entries.push_back(de);
        }
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end(), compareDirEntries);
    return true;
}

size_t FileSystem::getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
    return static_cast<size_t>(st.st_size);
}

std::time_t FileSystem::getLastModifiedTime(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_mtime;
}

std::string FileSystem::joinPaths(const std::string& p1, const std::string& p2) {
    if (p1.empty()) return p2;
    if (p2.empty()) return p1;
    if (p1[p1.size() - 1] == '/' && p2[0] == '/') {
        return p1 + p2.substr(1);
    }
    if (p1[p1.size() - 1] != '/' && p2[0] != '/') {
        return p1 + "/" + p2;
    }
    return p1 + p2;
}

std::string FileSystem::normalizePath(const std::string& path) {
    if (path.empty()) {
        return "/";
    }
    std::vector<std::string> segments;
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '/') {
            if (!current.empty()) {
                if (current == "..") {
                    if (!segments.empty()) {
                        segments.pop_back();
                    }
                } else if (current != ".") {
                    segments.push_back(current);
                }
                current.clear();
            }
        } else {
            current += path[i];
        }
    }
    if (!current.empty()) {
        if (current == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else if (current != ".") {
            segments.push_back(current);
        }
    }
    std::string result;
    for (size_t i = 0; i < segments.size(); ++i) {
        result += "/" + segments[i];
    }
    if (result.empty()) {
        result = "/";
    }
    if (path.size() > 1 && path[path.size() - 1] == '/' && result[result.size() - 1] != '/') {
        result += "/";
    }
    return result;
}

std::string FileSystem::getExtension(const std::string& path) {
    size_t dotPos = path.rfind('.');
    size_t slashPos = path.rfind('/');
    if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos)) {
        return path.substr(dotPos);
    }
    return "";
}

std::string FileSystem::getFilename(const std::string& path) {
    size_t slashPos = path.rfind('/');
    if (slashPos != std::string::npos) {
        return path.substr(slashPos + 1);
    }
    return path;
}

std::string FileSystem::getDirname(const std::string& path) {
    size_t slashPos = path.rfind('/');
    if (slashPos != std::string::npos) {
        if (slashPos == 0) return "/";
        return path.substr(0, slashPos);
    }
    return ".";
}
