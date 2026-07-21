// ====================================================================
// File:    src/net/Reactor.hpp | Module: net
// Purpose: THE poll() loop. one poll for all fds. dispatch ready
//          fds to handlers (read AND write same cycle). sweep dead.
// Owner:   Developer B   Deps: PollRegistry, IEventHandler, core/Signal,
//          connection/ConnectionManager, util/Logger
// Note:    single multiplex point. no errno-after-io. < 250 lines.
// ====================================================================
