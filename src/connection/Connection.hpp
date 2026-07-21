// ====================================================================
// File:    src/connection/Connection.hpp | Module: connection
// Purpose: per-client state machine. READING -> PROCESS -> WAIT_CGI?
//          -> WRITING -> KEEPALIVE|CLOSE. owns in/out IoBuffer,
//          RequestParser, HttpResponse. bridges http+routing+handler.
// Owner:   Developer B   Deps: Socket, IoBuffer, IEventHandler,
//          http/RequestParser, http/ResponseSerializer, routing/Router,
//          handlers/HandlerFactory
// Note:    orchestrates only, no parse/route logic inline. fattest
//          class - watch line count. < 250 lines.
// ====================================================================
