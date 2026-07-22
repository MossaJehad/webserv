// ====================================================================
// File:    src/net/Socket.hpp | Module: net
// Purpose: RAII fd wrapper. close on destroy, set O_NONBLOCK,
//          setsockopt SO_REUSEADDR.
// Owner:   Developer B   Deps: <sys/socket.h>, <fcntl.h>
// Note:    no copy. owns one fd. < 250 lines.
// ====================================================================
