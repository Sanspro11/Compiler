#pragma once
#include <vector>
#include <unordered_map>
#include "token.hpp"

class Lexer {
    public:
        static std::vector<Token> tokenize(std::ifstream& fileStream);

    private:
        static std::unordered_map<std::string,bool> keywords;
        static std::unordered_map<char,bool> symbols;
        static std::unordered_map<std::string,bool> types;
        static std::unordered_map<char,char> specialChars;
        static bool inString;

        static Token createToken(const std::string& str);
        static bool isKeyword(const std::string& token);
        static bool isSymbol(const char& ch);
        static bool isType(const std::string& token);
};