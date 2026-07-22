// ====================================================================
// File:    src/config/ConfigParser.hpp | Module: config
// Purpose: tokens -> vector<ServerConfig>. grammar (server{} location{}).
//          throws ConfigError on syntax error.
// Owner:   Developer A   Deps: ConfigTokenizer, ServerConfig,
//          LocationConfig, util/Exceptions
// Note:    risk of bloat. keep grammar split. < 250 lines.
// ====================================================================
