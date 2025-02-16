#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "token.hpp"
#include "lexer.hpp"

std::vector<Token> Lexer::tokenize(std::ifstream& fileStream) {
    std::vector<Token> tokens;
    std::string current;
    char ch;
    bool inComment = false;
    try {
        while (fileStream.get(ch)) {
            ++column;

            if (ch == '\n' || ch == '\r') { 
                inComment = false;
                ++row;
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

            if (ch == '=' || ch == '<' || ch == '>' || ch == '!') { // 2 char symbols
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                    current.clear();
                }
                current += ch;
                fileStream.get(ch);
                current += ch;
                if (current == "==" || current == "<=" || current == ">=" || current == "!=") {
                    tokens.push_back(createToken(current));
                }
                else {
                    tokens.push_back(createToken(std::string(1,current[0])));
                }
                current.clear();
                continue;
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
        std::cerr << "line: " << row << " column: " << column << 
        "\nerror while tokenizing: " << current << std::endl; 
        exit(1);
    }
    return tokens;
}

Token Lexer::createToken(const std::string& str) {
    size_t col = column - str.size();
    if (inString) { 
        return Token(tokenType::STRING, str, row, col);
    }
    else if (isKeyword(str)) {
        return Token(keywords[str], str, row, col);
    }
    else if (std::isdigit(str[0])) { 
        return Token(tokenType::CONSTANT, str, row, col);
    } 
    return Token(tokenType::NAME, str, row, col);
}

bool Lexer::isSymbol(const char chr) {
    return symbols.find(chr) != symbols.end();
}

bool Lexer::isKeyword(const std::string& token) {
    return keywords.find(token) != keywords.end();
}

bool Lexer::inString = false;
size_t Lexer::row = 1;
size_t Lexer::column = 0;

std::unordered_map<std::string,tokenType> Lexer::keywords = {
    {"return",tokenType::RETURN},
    {"if",tokenType::IF},
    {"while",tokenType::WHILE},
    {";",tokenType::SEMICOLON},
    {"+",tokenType::OPERATION},
    {"-",tokenType::OPERATION},
    {"*",tokenType::OPERATION},
    {"/",tokenType::OPERATION},
    {"{",tokenType::BRACE},
    {"}",tokenType::BRACE},
    {"(",tokenType::PARENTHESES},
    {")",tokenType::PARENTHESES},
    {",",tokenType::COMMA},
    {"=",tokenType::ASSIGNMENT},
    {"&",tokenType::ADDRESSOF},
    {"==",tokenType::COMPARISON},
    {"!=",tokenType::COMPARISON},
    {">",tokenType::COMPARISON},
    {"<",tokenType::COMPARISON},
    {">=",tokenType::COMPARISON},
    {"<=",tokenType::COMPARISON},
    {"int",tokenType::TYPE},
    {"void",tokenType::TYPE},
    {"uint64_t",tokenType::TYPE},
    {"uint32_t",tokenType::TYPE},
    {"uint16_t",tokenType::TYPE},
    {"uint8_t",tokenType::TYPE},
    {"char",tokenType::TYPE}
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