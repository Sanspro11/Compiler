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
        void require(const tokenType type, const std::string& name);
        ASTNode* parseStatement();
        ASTNode* parseFunction();
        CodeBlock* parseCodeBlock();
        ASTNode* parseExpression();
        bool shouldExpressionContinue();
        ReturnStatement* parseReturnStatement();
        ASTNode* parseFunctionCall();
        ASTNode* parseDeclaration();
        ASTNode* parseAssignment();
        ASTNode* parseIfStatement();
        ASTNode* parseWhileStatement();
        ASTNode* parseComparison();
        ASTNode* parseIdentifier();
        long long calculateOperation(const long long& value1, const long long& value2, const std::string& operation);


};