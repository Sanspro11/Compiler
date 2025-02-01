#pragma once
#include <vector>
#include "token.cpp"
#include "ASTnode.cpp"

class Parser {
public:
    Parser(std::vector<Token> tokensList) : tokens(tokensList) {}

    ProgramRoot* parse() {
        ProgramRoot* programRoot = new ProgramRoot();
        while (current().type != tokenType::ENDOFFILE) {
            //includes

            //functions
            if (current().type == tokenType::TYPE) { // make seperate in future
                ASTNode* function = parseFunction();
                programRoot->programElements.push_back(function);
            }

        }
        return programRoot;
    }

private:
    Token current() {
        if (index < tokens.size()) {
            return tokens[index];
        }
        return Token(tokenType::ENDOFFILE,"");
    }

    void advance() {
        ++index;
    }

    std::vector<Token> tokens;
    size_t index = 0;

    ASTNode* parseStatement() {
        if (current().type == tokenType::RETURN) {
            advance();
            ReturnStatement* returnStmt = new ReturnStatement();

            if (current().type == tokenType::CONSTANT) {
                returnStmt->expression = parseExpression();
            }
            else {
                returnStmt->expression = nullptr;
            }

            advance(); // ;

            return returnStmt;
        }
        else if (current().type == tokenType::NAME) {
            
            FunctionCall* funcCall = new FunctionCall();
            std::string funcName = current().value;
            funcCall->name = funcName;
            advance();
            // parameters here
            advance(); // (
            while (current().type != tokenType::PARENTHESES && current().value != ")") {
                ASTNode* expression = parseExpression();
                funcCall->parameters.push_back(expression);
            }
            advance(); // )

            advance(); // ;
            return funcCall;
        }

        return nullptr;  
    }

    ASTNode* parseFunction() {
        Function* function = new Function();
        function->returnType = current().value;
        advance();
        function->name = current().value;
        advance();
        // parameters here
        advance();
        advance();
        // parameters here
        function->codeBlock = parseCodeBlock();
        return function;
    }

    CodeBlock* parseCodeBlock() {
        CodeBlock* codeBlock = new CodeBlock();
        if (current().type == tokenType::BRACE && current().value == "{") {
            advance(); // advance {
            while (current().type != tokenType::BRACE && current().value != "}") {
                ASTNode* statement = parseStatement();
                codeBlock->statements.push_back(statement);
            }
            advance(); // advance }
        }
        return codeBlock;
    }

    ASTNode* parseExpression() {
        ASTNode* expression = nullptr;
        while (current().type != tokenType::SEMICOLON && current().type != tokenType::COMMA) {
            if (current().type == tokenType::CONSTANT) { 
                if (expression == nullptr) {
                    expression = new Constant(current().value);
                }
                else if (expression->type == NodeType::BinaryExpression) {
                    BinaryExpression* binExpr = (BinaryExpression*)expression;
                    if (binExpr->left->type == NodeType::Constant) {
                        Constant* otherNode = (Constant*)binExpr->left;
                        long long otherValue = std::stoi(otherNode->value);
                        const std::string& op = binExpr->op;
                        long long value = std::stoi(current().value); // assuming only constant numbers
                        value = calculateOperation(value,otherValue,op);
                        expression = new Constant(std::to_string(value));
                    }
                    else {
                        binExpr->right = new Constant(current().value);
                    }
                }
            }

            else if (current().type == tokenType::OPERATION) {
                BinaryExpression* binExpr = new BinaryExpression();
                binExpr->left = expression;
                binExpr->op = current().value;
                expression = binExpr;

            }
            advance();
        }
        return expression;
    }

    long long calculateOperation(const long long& value1,const  long long& value2,const std::string& operation) {
        if (operation == "+") 
            return value1 + value2;
        
        if (operation == "-") 
            return value1 - value2;
        
        if (operation == "*") 
            return value1 * value2;
        
        if (operation == "/") 
            return value1 / value2;
        return value1; // should never get to here
    }
};