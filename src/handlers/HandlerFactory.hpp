// ====================================================================
// File:    src/handlers/HandlerFactory.hpp | Module: handlers
// Purpose: pick handler from context. order: redirect, cgi ext,
//          then method (GET static/autoindex, POST upload, DELETE).
// Owner:   Developer C   Deps: IRequestHandler + concrete handlers,
//          cgi/CgiProcess
// Note:    dispatch only. add handler without touching Connection.
// ====================================================================
