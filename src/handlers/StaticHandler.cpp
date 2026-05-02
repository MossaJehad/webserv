#include "StaticHandler.hpp"
#include "../http/HttpResponse.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <ctime>

StaticHandler::StaticHandler() {}
StaticHandler::~StaticHandler() {}

void StaticHandler::handle(const ServerConfig& srv,
                           const LocationConfig& loc,
                           const HttpRequest& req,
                           HttpResponse& resp)
{
    const std::string& method = req.getMethod();
    if (method == "GET" || method == "HEAD")
        handleGet(srv, loc, req, resp);
    else if (method == "DELETE")
        handleDelete(loc, req, resp);
    else {
        resp.setStatus(405);
        resp.setBody(HttpResponse::buildError(405), "text/html");
    }
}

std::string StaticHandler::resolvePath(const LocationConfig& loc,
                                       const std::string& uriPath) const
{
    std::string root = loc.root;
    if (root.empty()) root = "./www/html";
    // Remove trailing slash from root
    while (root.size() > 1 && root[root.size()-1] == '/')
        root = root.substr(0, root.size()-1);

    // nginx `root` semantics: root + full_uri_path (no stripping of location prefix)
    std::string path = uriPath;
    if (path.empty() || path[0] != '/')
        path = "/" + path;

    return root + path;
}

bool StaticHandler::isDirectory(const std::string& path) const
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool StaticHandler::fileExists(const std::string& path) const
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

std::string StaticHandler::readFile(const std::string& path) const
{
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void StaticHandler::handleGet(const ServerConfig& srv,
                               const LocationConfig& loc,
                               const HttpRequest& req,
                               HttpResponse& resp)
{
    std::string fullPath = resolvePath(loc, req.getPath());

    if (isDirectory(fullPath)) {
        // Try index file
        std::string idx = loc.index.empty() ? "index.html" : loc.index;
        std::string idxPath = fullPath;
        if (idxPath[idxPath.size()-1] != '/') idxPath += '/';
        idxPath += idx;

        if (fileExists(idxPath)) {
            serveFile(idxPath, resp);
            return;
        }
        // Autoindex
        if (loc.autoindex) {
            serveAutoindex(fullPath, req.getPath(), resp);
            return;
        }
        // Check error page
        std::string ep = srv.getErrorPage(403);
        resp.setStatus(403);
        if (!ep.empty() && fileExists(ep)) {
            resp.setBody(readFile(ep), "text/html");
        } else {
            resp.setBody(HttpResponse::buildError(403), "text/html");
        }
        return;
    }

    if (!fileExists(fullPath)) {
        std::string ep = srv.getErrorPage(404);
        resp.setStatus(404);
        if (!ep.empty() && fileExists(ep)) {
            resp.setBody(readFile(ep), "text/html");
        } else {
            resp.setBody(HttpResponse::buildError(404), "text/html");
        }
        return;
    }

    serveFile(fullPath, resp);
}

void StaticHandler::serveFile(const std::string& fullPath, HttpResponse& resp)
{
    std::string content = readFile(fullPath);
    if (content.empty() && !fileExists(fullPath)) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }
    std::string mime = HttpResponse::mimeType(fullPath);
    resp.setStatus(200);
    resp.setBody(content, mime);
}

void StaticHandler::serveAutoindex(const std::string& fullPath,
                                   const std::string& uriPath,
                                   HttpResponse& resp)
{
    DIR* dir = opendir(fullPath.c_str());
    if (!dir) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }

    std::string uri = uriPath;
    if (!uri.empty() && uri[uri.size()-1] != '/') uri += '/';

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head><title>Index of " << uri << "</title>"
         << "<style>body{font-family:monospace;padding:20px;}table{width:100%;border-collapse:collapse;}"
         << "th,td{text-align:left;padding:4px 12px;}tr:hover{background:#f2f2f2;}"
         << "a{text-decoration:none;color:#0066cc;}hr{margin:10px 0;}</style></head>"
         << "<body><h1>Index of " << uri << "</h1><hr>"
         << "<table><tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>";

    if (uri != "/")
        html << "<tr><td><a href=\"../\">../</a></td><td>-</td><td>-</td></tr>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        std::string entPath = fullPath + "/" + name;
        struct stat st;
        stat(entPath.c_str(), &st);

        bool isDir = S_ISDIR(st.st_mode);
        std::string display = isDir ? name + "/" : name;
        std::string href    = uri + (isDir ? name + "/" : name);

        char timeBuf[64];
        struct tm* tm_info = localtime(&st.st_mtime);
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", tm_info);

        std::ostringstream size;
        if (isDir) size << "-";
        else size << st.st_size;

        html << "<tr><td><a href=\"" << href << "\">" << display << "</a></td>"
             << "<td>" << size.str() << "</td>"
             << "<td>" << timeBuf << "</td></tr>";
    }
    closedir(dir);

    html << "</table><hr><p>webserv</p></body></html>";
    resp.setStatus(200);
    resp.setBody(html.str(), "text/html");
}

void StaticHandler::handleDelete(const LocationConfig& loc,
                                  const HttpRequest& req,
                                  HttpResponse& resp)
{
    std::string fullPath = resolvePath(loc, req.getPath());

    if (!fileExists(fullPath)) {
        resp.setStatus(404);
        resp.setBody(HttpResponse::buildError(404), "text/html");
        return;
    }
    if (isDirectory(fullPath)) {
        resp.setStatus(403);
        resp.setBody(HttpResponse::buildError(403), "text/html");
        return;
    }
    if (unlink(fullPath.c_str()) != 0) {
        resp.setStatus(500);
        resp.setBody(HttpResponse::buildError(500), "text/html");
        return;
    }
    resp.setStatus(204);
    resp.setHeader("Content-Length", "0");
    resp.setBodyRaw("");
}
