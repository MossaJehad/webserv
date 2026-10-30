#include "AutoindexHandler.hpp"
#include "FileSystem.hpp"
#include "Time.hpp"
#include "StringUtils.hpp"
#include <sstream>
#include <iomanip>

AutoindexHandler::AutoindexHandler() {}
AutoindexHandler::~AutoindexHandler() {}

HttpResponse AutoindexHandler::handle(const RequestContext& ctx) {
    return generateListing(ctx.getResolvedFsPath(), ctx.getRequest().getPath());
}

static std::string formatSize(size_t bytes) {
    if (bytes < 1024) {
        return StringUtils::toString(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return oss.str();
    }
}

HttpResponse AutoindexHandler::generateListing(const std::string& dirPath, const std::string& uriPath) {
    std::vector<DirEntry> entries;
    if (!FileSystem::listDirectory(dirPath, entries)) {
        HttpResponse res(500);
        res.setBody("<html><body><h1>500 Cannot read directory</h1></body></html>");
        res.setContentType("text/html");
        return res;
    }

    std::string cleanUri = uriPath;
    if (cleanUri.empty() || cleanUri[cleanUri.size() - 1] != '/') {
        cleanUri += "/";
    }
    // Everything interpolated into the page below is untrusted (request path and
    // on-disk file names), so it is escaped before rendering.
    std::string safeUri = StringUtils::htmlEscape(cleanUri);

    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "  <meta charset=\"utf-8\">\n"
         << "  <title>Index of " << safeUri << "</title>\n"
         << "  <style>\n"
         << "    body { font-family: monospace; padding: 20px; background: #fff; color: #222; }\n"
         << "    h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; font-size: 20px; }\n"
         << "    table { width: 100%; max-width: 900px; border-collapse: collapse; margin-top: 15px; }\n"
         << "    th, td { text-align: left; padding: 6px 12px; }\n"
         << "    th { border-bottom: 2px solid #ddd; }\n"
         << "    tr:hover { background-color: #f5f5f5; }\n"
         << "    a { color: #0366d6; text-decoration: none; }\n"
         << "    a:hover { text-decoration: underline; }\n"
         << "    .footer { margin-top: 30px; font-size: 12px; color: #777; border-top: 1px solid #eee; padding-top: 10px; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <h1>Index of " << safeUri << "</h1>\n"
         << "  <table>\n"
         << "    <tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

    if (cleanUri != "/") {
        std::string parentUri = cleanUri.substr(0, cleanUri.size() - 1);
        size_t lastSlash = parentUri.rfind('/');
        if (lastSlash != std::string::npos) {
            parentUri = parentUri.substr(0, lastSlash + 1);
        } else {
            parentUri = "/";
        }
        html << "    <tr><td>📁 <a href=\"" << StringUtils::htmlEscape(parentUri)
             << "\">../</a></td><td>-</td><td>-</td></tr>\n";
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        const DirEntry& e = entries[i];
        // The href needs percent-encoding (one path segment), the visible text
        // needs HTML escaping. Doing only one of the two would be a hole.
        std::string link = safeUri + StringUtils::urlEncode(e.name) + (e.isDirectory ? "/" : "");
        std::string displayName = StringUtils::htmlEscape(e.name) + (e.isDirectory ? "/" : "");
        std::string icon = e.isDirectory ? "📁 " : "📄 ";
        std::string dateStr = Time::formatHttpDate(e.lastModified);
        std::string sizeStr = e.isDirectory ? "-" : formatSize(e.size);

        html << "    <tr>"
             << "<td>" << icon << "<a href=\"" << link << "\">" << displayName << "</a></td>"
             << "<td>" << dateStr << "</td>"
             << "<td>" << sizeStr << "</td>"
             << "</tr>\n";
    }

    html << "  </table>\n"
         << "  <div class=\"footer\">webserv/1.0</div>\n"
         << "</body>\n"
         << "</html>\n";

    HttpResponse response(200);
    response.setBody(html.str());
    response.setContentType("text/html; charset=utf-8");
    return response;
}
