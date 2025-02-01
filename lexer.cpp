#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include "token.cpp"
#include <unordered_map>
class lexer {
public:
    static std::vector<Token> tokenize(std::ifstream& fileStream) {
        std::vector<Token> tokens;
        std::string currentToken;
        int line = 1;
        int column = 0;
        bool inString = false;
        char ch;
        try {
            while (fileStream.get(ch)) {
                ++column;
                if (ch == ' ') {
                    if (!currentToken.empty()) {
                        tokens.push_back(createToken(currentToken));
                        currentToken.clear();
                    }
                    continue;
                }
                if (ch == '\n' || ch == '\r') { 
                    ++line;
                    column = 1;
                    continue;
                }

                if (isKeyword(currentToken) || isSymbol(ch) || isType(currentToken)) {
                    if ((isSymbol(ch)) && !currentToken.empty()) {
                        tokens.push_back(createToken(currentToken));
                        currentToken.clear();
                    }
                    currentToken += ch;
                    tokens.push_back(createToken(currentToken));
                    currentToken.clear();
                    continue;
                }
                currentToken += ch;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "line: " << line << " column: " << column << 
            "\nerror while tokenizing: " << currentToken << std::endl; 
            exit(1);
        }
        return tokens;
    }

private:
    static std::unordered_map<std::string,bool> keywords;
    static std::unordered_map<char,bool> symbols;
    static std::unordered_map<std::string,bool> types;


    static Token createToken(const std::string& str) {
        if (str == "return") {
            return Token(tokenType::RETURN, str);
        } else if (str == ";") {
            return Token(tokenType::SEMICOLON, str);
        } else if (str == "+" || str == "-" || str == "*" || str == "/") {
            return Token(tokenType::OPERATION, str);
        } else if (str == "{" || str == "}") {
            return Token(tokenType::BRACE, str);
        } else if (str == "(" || str == ")") {
            return Token(tokenType::PARENTHESES, str);
        } else if (str == ",") {
            return Token(tokenType::COMMA, str);
        } else if (isType(str)) {
            return Token(tokenType::TYPE, str);
        } else if (std::isdigit(str[0])) {
            return Token(tokenType::CONSTANT, str);
        } 
        return Token(tokenType::NAME, str);
    }

    static bool isKeyword(const std::string& token) {
        return keywords.find(token) != keywords.end();
    }

    static bool isSymbol(const char& ch) {
        return symbols.find(ch) != symbols.end();
    }

    static bool isType(const std::string& token) {
        return types.find(token) != types.end();
    }
};

std::unordered_map<std::string,bool> lexer::keywords = {
    {"return",true},
};

std::unordered_map<std::string,bool> lexer::types = {
    {"int",true},
    {"void",true}
};

std::unordered_map<char,bool> lexer::symbols = {
    {'+',true},
    {'-',true},
    {'*',true},
    {'/',true},
    {'(',true},
    {')',true},
    {'{',true},
    {'}',true},
    {',',true},
    {';',true},
};