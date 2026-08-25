#include "ConfigTokenizer.hpp"
#include "FileSystem.hpp"
#include "Exceptions.hpp"
#include <cctype>

std::vector<Token> ConfigTokenizer::tokenize(const std::string& content) {
    std::vector<Token> tokens;
    size_t line = 1;
    size_t i = 0;
    size_t n = content.size();

    while (i < n) {
        char c = content[i];

        if (c == '\n') {
            line++;
            i++;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        if (c == '#') {
            // Skip comment till end of line
            while (i < n && content[i] != '\n') {
                i++;
            }
            continue;
        }

        if (c == '{') {
            tokens.push_back(Token(TOKEN_BRACE_OPEN, "{", line));
            i++;
            continue;
        }

        if (c == '}') {
            tokens.push_back(Token(TOKEN_BRACE_CLOSE, "}", line));
            i++;
            continue;
        }

        if (c == ';') {
            tokens.push_back(Token(TOKEN_SEMICOLON, ";", line));
            i++;
            continue;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            std::string word;
            while (i < n && content[i] != quote) {
                if (content[i] == '\n') line++;
                word += content[i];
                i++;
            }
            if (i >= n) {
                throw ConfigError("Unterminated quote in config file");
            }
            i++; // consume quote
            tokens.push_back(Token(TOKEN_WORD, word, line));
            continue;
        }

        // Normal word
        std::string word;
        while (i < n && !std::isspace(static_cast<unsigned char>(content[i])) &&
               content[i] != '{' && content[i] != '}' && content[i] != ';' &&
               content[i] != '#') {
            word += content[i];
            i++;
        }
        if (!word.empty()) {
            tokens.push_back(Token(TOKEN_WORD, word, line));
        }
    }

    tokens.push_back(Token(TOKEN_EOF, "", line));
    return tokens;
}

std::vector<Token> ConfigTokenizer::tokenizeFile(const std::string& filePath) {
    if (!FileSystem::exists(filePath)) {
        throw ConfigError("Configuration file not found: " + filePath);
    }
    std::string content;
    if (!FileSystem::readFile(filePath, content)) {
        throw ConfigError("Could not read configuration file: " + filePath);
    }
    return tokenize(content);
}
