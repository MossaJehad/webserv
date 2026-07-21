// ====================================================================
// File:    src/cgi/CgiEnvironment.hpp | Module: cgi
// Purpose: build CGI/1.1 meta-variable envp from request + context.
//          REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH/TYPE,
//          SCRIPT_NAME, PATH_INFO, SERVER_*, etc.
// Owner:   Developer C   Deps: routing/RequestContext, http/HttpRequest
// Note:    pure build, no exec. < 250 lines.
// ====================================================================
