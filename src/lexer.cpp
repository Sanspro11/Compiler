#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "token.hpp"
#include "lexer.hpp"

std::vector<Token> Lexer::tokenize(std::ifstream& fileStream) {
    std::vector<Token> tokens;
    std::string currentToken;
    int line = 1;
    int column = 0;
    char ch;
    char lastCh = 0;
    bool inComment = false;
    try {
        while (fileStream.get(ch)) {
            ++column;

            if (ch == '\n' || ch == '\r') { 
                inComment = false;
                ++line;
                column = 0;
                currentToken.clear();
                lastCh = 0;
                continue;
            }
            if (inComment) 
                continue;

            if (lastCh == '/' && ch == '/') {
                inComment = true;
                continue;
            }

            if (ch == '/') {
                lastCh = ch;
                continue;
            }

            if (lastCh == '/' && ch != '/') {
                currentToken += "/";
            }

            if (inString && ch != '\"') {
                if (lastCh == '\\') {
                    if (ch == 'n') {
                        currentToken[currentToken.size()-1] = '\n';
                    }
                    continue;
                }
                lastCh = ch;
                currentToken += ch;
                continue;
            }

            if (ch == '\"') {
                if (!inString) {
                    inString = true;
                    continue;
                }
                tokens.push_back(createToken(currentToken));
                currentToken.clear();
                inString = false;
                continue;
            }

            if (ch == ' ') {
                if (!currentToken.empty()) {
                    tokens.push_back(createToken(currentToken));
                    currentToken.clear();
                }
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
            lastCh = ch;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "line: " << line << " column: " << column << 
        "\nerror while tokenizing: " << currentToken << std::endl; 
        exit(1);
    }
    return tokens;
}

Token Lexer::createToken(const std::string& str) {
    if (inString) { 
        return Token(tokenType::STRING, str);
    }
    else if (str == "return") {
        return Token(tokenType::RETURN, str);
    }
    else if (str == ";") {
        return Token(tokenType::SEMICOLON, str);
    }
    else if (str == "+" || str == "-" || str == "*" || str == "/") {
        return Token(tokenType::OPERATION, str);
    }
    else if (str == "{" || str == "}") {
        return Token(tokenType::BRACE, str);
    }
    else if (str == "(" || str == ")") {
        return Token(tokenType::PARENTHESES, str);
    }
    else if (str == ",") {
        return Token(tokenType::COMMA, str);
    }
    else if (str == "=") {
        return Token(tokenType::ASSIGNMENT, str);
    }
    else if (str == "&") {
        return Token(tokenType::ADDRESSOF, str);
    }
    else if (isType(str)) {
        return Token(tokenType::TYPE, str);
    }
    else if (std::isdigit(str[0])) { 
        return Token(tokenType::CONSTANT, str);
    } 
    return Token(tokenType::NAME, str);
}

bool Lexer::isKeyword(const std::string& token) {
    return keywords.find(token) != keywords.end();
}

bool Lexer::isSymbol(const char& ch) {
    return symbols.find(ch) != symbols.end();
}

bool Lexer::isType(const std::string& token) {
    return types.find(token) != types.end();
}


bool Lexer::inString = false;
std::unordered_map<std::string,bool> Lexer::keywords = {
    {"return",true},
};

std::unordered_map<std::string,bool> Lexer::types = {
    {"int",true},
    {"void",true},
    {"uint64_t",true},
    {"char",true}
};

std::unordered_map<char,bool> Lexer::symbols = {
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
    {'=',true},
    {'&',true},
};