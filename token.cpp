#pragma once
#include <string>
#include <iostream>

enum tokenType {
    RETURN,
    CONSTANT,
    SEMICOLON,
    OPERATION,
    BRACE,
    PARENTHESES,
    TYPE,
    NAME,
    COMMA,
    ENDOFFILE
};

struct Token {
public:

    tokenType type;
    std::string value;

    void print() const {
        std::cout << "Type: " << type << " Value: " << value;
    }
    Token(tokenType t, const std::string& v) : type(t), value(v) {}
};