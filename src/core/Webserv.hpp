// ====================================================================
// File:    src/core/Webserv.hpp | Module: core
// Purpose: top orchestrator. load config -> build listeners ->
//          register in reactor -> run. holds object lifetimes.
// Owner:   Developer A
// Deps:    config/ConfigParser, net/Reactor, net/Listener,
//          connection/ConnectionManager
// Note:    thin orchestrator. no domain logic. < 250 lines.
// ====================================================================
