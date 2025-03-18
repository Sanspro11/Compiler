#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "token.hpp"

class Lexer {
    public:
        Lexer(std::string& sourceCode) : sourceCode(sourceCode) {};
        std::vector<Token> tokenize();

    private:
        static std::unordered_map<std::string,tokenType> keywords;
        static std::unordered_map<char,bool> symbols;
        static std::unordered_map<char,char> escapeChars;
        bool inString;
        size_t row;
        size_t column;
        std::string sourceCode;

        Token createToken(const std::string& str);
        bool isSymbol(const char chr);
        bool isKeyword(const std::string& token);
};