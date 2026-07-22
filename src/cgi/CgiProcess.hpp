// ====================================================================
// File:    src/cgi/CgiProcess.hpp | Module: cgi
// Purpose: run script async. pipe + fork + execve, non-block pipes.
//          onWritable feeds request body to child stdin, onReadable
//          collects child stdout. registers its fds in reactor.
// Owner:   Developer C   Deps: net/IEventHandler, CgiEnvironment,
//          CgiResponseParser, connection/Connection, util/Logger
// Note:    implements IEventHandler. async = main coupling risk.
//          never block parent. < 250 lines.
// ====================================================================
