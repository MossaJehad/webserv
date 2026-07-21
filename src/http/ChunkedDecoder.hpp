// ====================================================================
// File:    src/http/ChunkedDecoder.hpp | Module: http
// Purpose: decode Transfer-Encoding: chunked body incrementally.
//          feed(bytes) -> appended decoded bytes / done / error.
// Owner:   Developer C   Deps: util/StringUtils
// Note:    isolated from RequestParser. < 250 lines.
// ====================================================================
