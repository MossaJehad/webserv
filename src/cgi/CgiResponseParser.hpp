// ====================================================================
// File:    src/cgi/CgiResponseParser.hpp | Module: cgi
// Purpose: parse CGI stdout (header block + body) into HttpResponse.
//          honor Status and Content-Type headers from script.
// Owner:   Developer C   Deps: http/HttpResponse, http/HttpHeaders
// Note:    pure transform. < 250 lines.
// ====================================================================
