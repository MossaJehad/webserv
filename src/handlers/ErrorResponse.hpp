// ====================================================================
// File:    src/handlers/ErrorResponse.hpp | Module: handlers
// Purpose: build error response. custom error_page if configured,
//          else generated default page. single source of error pages.
// Owner:   Developer C   Deps: http/HttpResponse, http/HttpStatus,
//          config/ServerConfig, util/FileSystem
// Note:    central. used by all error paths. < 250 lines.
// ====================================================================
