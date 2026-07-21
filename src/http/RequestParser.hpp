// ====================================================================
// File:    src/http/RequestParser.hpp | Module: http
// Purpose: incremental parser. feed(bytes) -> state NEED_MORE|DONE|
//          ERROR. states: request-line, headers, body. uses
//          ChunkedDecoder for chunked bodies. enforces max_body.
// Owner:   Developer C   Deps: HttpRequest, HttpHeaders, ChunkedDecoder,
//          util/StringUtils, util/Exceptions
// Note:    bloat risk. extract HeaderParser if needed. < 250 lines.
// ====================================================================
