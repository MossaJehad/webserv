// ====================================================================
// File:    src/net/PollRegistry.hpp | Module: net
// Purpose: fd <-> IEventHandler map. add/remove/update interest.
//          build pollfd[] for poll(). lookup handler by fd.
// Owner:   Developer B   Deps: IEventHandler, <poll.h>, <map>, <vector>
// Note:    no loop logic here. < 250 lines.
// ====================================================================
