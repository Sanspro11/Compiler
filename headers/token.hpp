#pragma once
#include <string>
#include <iostream>

enum tokenType {
    RETURN,
    CONSTANT,
    SEMICOLON,
    OPERATION,
    CURLY_BRACKET,
    PARENTHESES,
    TYPE,
    NAME,
    COMMA,
    STRING,
    ASSIGNMENT,
    ADDRESSOF,
    IF,
    ELSE,
    WHILE,
    COMPARISON,
    SQUARE_BRACKET,
    ENDOFFILE
};

struct Token {
public:

    tokenType type;
    size_t row;
    size_t column;
    std::string value;

    void print() const {
        std::cout << row << ":" << column << " Type: " << type << " Val: " << value;
    }
    Token(tokenType t, const std::string& v, size_t row, size_t column) :
    type(t), value(v), row(row), column(column) {}
};