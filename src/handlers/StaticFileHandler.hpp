// ====================================================================
// File:    src/handlers/StaticFileHandler.hpp | Module: handlers
// Purpose: GET static file. resolve index, read file, set mime,
//          200 / 403 / 404. delegate dir to Autoindex if enabled.
// Owner:   Developer C   Deps: IRequestHandler, util/FileSystem,
//          util/MimeTypes, ErrorResponse
// Note:    one job. < 250 lines.
// ====================================================================
