#ifndef CONFIGTOKENIZER_HPP
#define CONFIGTOKENIZER_HPP

#include <string>
#include <vector>

enum TokenType {
    TOKEN_WORD,
    TOKEN_BRACE_OPEN,
    TOKEN_BRACE_CLOSE,
    TOKEN_SEMICOLON,
    TOKEN_EOF
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;

    Token(TokenType t = TOKEN_EOF, const std::string& v = "", size_t l = 0)
        : type(t), value(v), line(l) {}
};

class ConfigTokenizer {
public:
    static std::vector<Token> tokenize(const std::string& content);
    static std::vector<Token> tokenizeFile(const std::string& filePath);
};

#endif
