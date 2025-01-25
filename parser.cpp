#pragma once
#include <vector>
#include "token.hpp"
#include "ASTnode.hpp"

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
            
            std::string funcName = current().value;
            advance();
            // parameters here
            advance();
            advance();
            // parameters here
            FunctionCall* funcCall = new FunctionCall();
            funcCall->name = funcName;
            advance(); // ;
            return funcCall;
        }

        return nullptr;  // Handle other statements if needed
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
        if (current().type == tokenType::CONSTANT) {
            long long value = std::stoi(current().value);
            advance();
            while (current().type == tokenType::OPERATION) {
                std::string operation = current().value;
                advance();
                long long  otherValue = std::stoi(current().value);
                value = calculateOperation(value,otherValue,operation);
                advance();
            }
            return new Constant(std::to_string(value));
        }
        return nullptr;  // Handle other expressions as needed
    }

    long long calculateOperation(long long& value1, long long& value2, std::string& operation) {
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