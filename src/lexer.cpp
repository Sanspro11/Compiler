#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "token.hpp"
#include "lexer.hpp"

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    std::string current;
    inString = false;
    row = 1;
    column = 0;
    try {
        for (size_t i = 0; i < sourceCode.size(); ++i) {
            ++column;
            char ch = sourceCode[i];

            if (ch == '\n' || ch == '\r' || ch == '\t') { 
                if (ch == '\t') {
                    column += 4;
                }
                else {
                    ++row;
                    column = 0;
                }
                current.clear();
                continue;
            }

            if (inString) {
                if (ch == '\"') { // string end
                    tokens.push_back(createToken(current));
                    current.clear();
                    inString = false;
                    continue;
                }
                if (ch == '\\') { // escape char
                    ++i;
                    ch = sourceCode[i];
                    if (escapeChars.find(ch) != escapeChars.end()) {
                        current += escapeChars[ch];
                        continue;
                    }
                }
                current += ch;
                continue;
            }

            if (ch == '\"') { // string start
                inString = true;
                continue;
            }

            if (ch == '\'') { // char
                ++i;
                ch = sourceCode[i];
                tokens.push_back(createToken(std::to_string(ch)));
                ++i;
                continue;
            }

            if (isSymbol(ch))  {
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                    current.clear();
                }
                if (isKeyword(std::string(1,ch) + sourceCode[i+1])) { // 2 chars symbol
                    ++i;
                    tokens.push_back(createToken(std::string(1,ch) + sourceCode[i]));
                }
                else {
                    tokens.push_back(createToken(std::string(1,ch)));
                }
                continue;
            }

            if (ch == ' ') {
                if (!current.empty()) {
                    tokens.push_back(createToken(current));
                }
                current.clear();
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

std::unordered_map<std::string,tokenType> Lexer::keywords = {
    {"return",tokenType::RETURN},
    {"if",tokenType::IF},
    {"while",tokenType::WHILE},
    {"struct",tokenType::STRUCT},
    {";",tokenType::SEMICOLON},
    {"+",tokenType::OPERATION},
    {"-",tokenType::OPERATION},
    {"*",tokenType::OPERATION},
    {"/",tokenType::OPERATION},
    {"%",tokenType::OPERATION},
    {"&",tokenType::OPERATION},
    {"(",tokenType::PARENTHESES},
    {")",tokenType::PARENTHESES},
    {"{",tokenType::CURLY_BRACKET},
    {"}",tokenType::CURLY_BRACKET},
    {"[",tokenType::SQUARE_BRACKET},
    {"]",tokenType::SQUARE_BRACKET},
    {",",tokenType::COMMA},
    {".",tokenType::DOT},
    {"=",tokenType::ASSIGNMENT},
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
    {'%',true},
    {'(',true},
    {')',true},
    {'{',true},
    {'}',true},
    {'[',true},
    {']',true},
    {',',true},
    {';',true},
    {'=',true},
    {'&',true},
    {'<',true},
    {'>',true},
    {'.',true},
};

std::unordered_map<char,char> Lexer::escapeChars = {
    {'n','\n'}, // new line
    {'r','\r'}, // carriage return
    {'t','\t'}, // tab
    {'v','\v'}, // vertical tab
    {'a','\a'}, // alert
};