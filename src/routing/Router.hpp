// ====================================================================
// File:    src/routing/Router.hpp | Module: routing
// Purpose: match request to server (host header + listen port) then
//          location (longest path prefix). check method allowed.
//          resolve fs path (root + uri). build RequestContext.
// Owner:   Developer C   Deps: RequestContext, http/HttpRequest,
//          config/ServerConfig, util/FileSystem
// Note:    selection only, no I/O. < 250 lines.
// ====================================================================
