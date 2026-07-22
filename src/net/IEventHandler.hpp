// ====================================================================
// File:    src/net/IEventHandler.hpp | Module: net
// Purpose: interface for any pollable fd. onReadable(), onWritable(),
//          fd(), wantsWrite(), isDead(). reactor talks only to this.
// Owner:   Developer B   Deps: none
// Note:    CORE CONTRACT. implemented by Listener, Connection,
//          CgiProcess. abstract, no state. < 250 lines.
// ====================================================================
