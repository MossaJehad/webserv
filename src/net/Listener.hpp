// ====================================================================
// File:    src/net/Listener.hpp | Module: net
// Purpose: passive socket per host:port. socket+bind+listen.
//          onReadable() = accept -> new Connection.
// Owner:   Developer B   Deps: Socket, IEventHandler, config/ServerConfig,
//          connection/ConnectionManager
// Note:    implements IEventHandler. < 250 lines.
// ====================================================================
