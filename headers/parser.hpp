#pragma once
#include <vector>
#include "ASTnode.hpp"
#include "token.hpp"

class Parser {
    public:
        Parser(std::vector<Token> tokensList) : tokens(tokensList) {}
        ProgramRoot* Parser::parse();

    private:
        std::vector<Token> tokens;
        size_t index = 0;
        Token current();
        void advance();
        void back();
        Token peekNext();
        ASTNode* parseStatement();
        ASTNode* parseFunction();
        CodeBlock* parseCodeBlock();
        ASTNode* parseExpression();
        ReturnStatement* parseReturnStatement();
        ASTNode* parseFunctionCall();
        ASTNode* parseDeclaration();
        ASTNode* parseAssignment();
        long long calculateOperation(const long long& value1, const long long& value2, const std::string& operation);


};