#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "token.hpp"
#include "lexer.hpp"

std::vector<Token> Lexer::tokenize(std::ifstream& fileStream) {
    std::vector<Token> tokens;
    std::string current;
    int line = 1;
    int column = 0;
    char ch;
    bool inComment = false;
    try {
        while (fileStream.get(ch)) {
            ++column;

            if (ch == '\n' || ch == '\r') { 
                inComment = false;
                ++line;
                column = 0;
                current.clear();
                continue;
            }

            if (inComment) 
                continue;

            if (inString && ch != '\"') {
                if (ch == '\\') {
                    fileStream.get(ch);
                    if (escapeChars.find(ch) != escapeChars.end()) {
                        current += escapeChars[ch];
                    }
                    else {
                        current += ch; 
                    }
                    continue;
                }
                current += ch;
                continue;
            }

            if (ch == '/') {
                fileStream.get(ch);
                if (ch == '/') {
                    inComment = true;
                    continue;
                }

                tokens.push_back(createToken(current));
                current.clear();
                tokens.push_back(createToken("/"));
                current += ch;
                continue;
            }

            // int a = 0;
            // if (a >= a);
            if (ch == '=' || ch == '<' || ch == '>') { // 2 char symbols
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                    current.clear();
                }
                current += ch;
                fileStream.get(ch);
                current += ch;
                if (current == "==" || current == "<=" || current == ">=") {
                    tokens.push_back(createToken(current));
                }
                else {
                    tokens.push_back(createToken(std::string(1,current[0])));
                }
                current.clear();
            }

            if (ch == '\"') {
                if (!inString) {
                    inString = true;
                    continue;
                }
                tokens.push_back(createToken(current));
                current.clear();
                inString = false;
                continue;
            }

            if (ch == ' ') {
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                }
                current.clear();
                continue;
            }

            if (ch == '\'') {
                fileStream.get(ch); 
                tokens.push_back(createToken(std::to_string(ch)));
                fileStream.get(ch); 
                continue;
            }

            if (isSymbol(ch))  {
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                    current.clear();
                }
                tokens.push_back(createToken(std::string(1,ch)));
                continue;
            }

            current += ch;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "line: " << line << " column: " << column << 
        "\nerror while tokenizing: " << current << std::endl; 
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
    else if (str == "if") {
        return Token(tokenType::IF, str);
    }
    else if (str == "while") {
        return Token(tokenType::WHILE, str);
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
    else if (str == "<" || str == ">" || str == "==" || str == "<=" || str == ">=") {
        return Token(tokenType::COMPARISON, str);
    }
    else if (isType(str)) {
        return Token(tokenType::TYPE, str);
    }
    else if (std::isdigit(str[0])) { 
        return Token(tokenType::CONSTANT, str);
    } 
    return Token(tokenType::NAME, str);
}


bool Lexer::isSymbol(const char chr) {
    return symbols.find(chr) != symbols.end();
}

bool Lexer::isType(const std::string& token) {
    return types.find(token) != types.end();
}


bool Lexer::inString = false;

std::unordered_map<std::string,bool> Lexer::types = {
    {"int",true},
    {"void",true},
    {"uint64_t",true},
    {"uint32_t",true},
    {"uint16_t",true},
    {"uint8_t",true},
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
    {'<',true},
    {'>',true},
};

std::unordered_map<char,char> Lexer::escapeChars = {
    {'n','\n'}, // new line
    {'r','\r'}, // carriage return
    {'t','\t'}, // tab
    {'v','\v'}, // vertical tab
    {'a','\a'}, // alert
};