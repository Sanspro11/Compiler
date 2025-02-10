#pragma once
#include <vector>
#include <unordered_map>
#include "token.hpp"

class Lexer {
    public:
        static std::vector<Token> tokenize(std::ifstream& fileStream);

    private:
        static std::unordered_map<char,bool> symbols;
        static std::unordered_map<std::string,bool> types;
        static std::unordered_map<char,char> escapeChars;
        static bool inString;

        static Token createToken(const std::string& str);
        static bool isSymbol(const char chr);
        static bool isType(const std::string& token);
};