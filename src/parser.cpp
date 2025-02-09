#pragma once
#include <vector>
#include "parser.hpp"
#include "token.hpp"
#include "ASTnode.hpp"

ProgramRoot* Parser::parse() {
    ProgramRoot* programRoot = new ProgramRoot();
    while (current().type != tokenType::ENDOFFILE) {
        //includes

        //functions
        if (current().type == tokenType::TYPE) { // make separate in future
            ASTNode* function = parseFunction();
            programRoot->programElements.push_back(function);
        }

    }
    return programRoot;
}


Token Parser::current() {
    if (index < tokens.size()) {
        return tokens[index];
    }
    return Token(tokenType::ENDOFFILE,"");
}

void Parser::advance() {
    ++index;
}
void Parser::back() {
    --index;
}
Token Parser::peekNext() {
    if (index+1 < tokens.size()) {
        return tokens[index+1];
    }
    return Token(tokenType::ENDOFFILE,"");
}

ASTNode* Parser::parseStatement() {
    if (current().type == tokenType::RETURN) {
        return parseReturnStatement();
    }
    else if (current().type == tokenType::TYPE) { // declaration
        return parseDeclaration();
    }
    else if (current().type == tokenType::NAME) {
        if (peekNext().type == tokenType::ASSIGNMENT) {
            return parseAssignment();
        }
        return parseFunctionCall();
    }

    return nullptr;  
}

ASTNode* Parser::parseFunction() {
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

CodeBlock* Parser::parseCodeBlock() {
    CodeBlock* codeBlock = new CodeBlock();
    if (current().type == tokenType::BRACE && current().value == "{") {
        advance(); // {
        while (current().type != tokenType::BRACE && current().value != "}") {
            ASTNode* statement = parseStatement();
            advance(); // ;
            codeBlock->statements.push_back(statement);
        }
        advance(); // }
    }
    return codeBlock;
}

ASTNode* Parser::parseExpression() {
    ASTNode* expression = nullptr;
    while (shouldExpressionContinue()){
        if (current().type == tokenType::CONSTANT) { 
            if (expression == nullptr) {
                expression = new Constant(current().value);
            }
            else if (expression->type == NodeType::BinaryExpression) {
                BinaryExpression* binExpr = (BinaryExpression*)expression;
                if (binExpr->left->type == NodeType::Constant
                    && ((Constant*)binExpr->left)->constantType == "uint64_t") {
                    Constant* otherNode = (Constant*)binExpr->left;
                    long long otherValue = std::stoll(otherNode->value);
                    const std::string& op = binExpr->op;
                    long long value = std::stoll(current().value); // assuming only constant numbers
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

        else if (current().type == tokenType::STRING) {
            if (expression == nullptr) {
                expression = new Constant(current().value);
                ((Constant*)expression)->constantType = "string";
                
            }
            else if (expression->type == NodeType::BinaryExpression) {
                BinaryExpression* binExpr = (BinaryExpression*)expression;
                binExpr->right = new Constant(current().value);
                ((Constant*)binExpr->right)->constantType = "string";
            }
        }

        else if (current().type == tokenType::NAME) {
            if (peekNext().type == tokenType::PARENTHESES && peekNext().value == "(") { // functionCall
                if (expression == nullptr) {
                    expression = parseFunctionCall();
                }
                else if (expression->type == NodeType::BinaryExpression) {
                    BinaryExpression* binExpr = (BinaryExpression*)expression;
                    binExpr->right = parseFunctionCall();
                }
            }

            else { // variable
                if (expression == nullptr) {
                    expression = new Identifier(current().value);
                }
                else if (expression->type == NodeType::BinaryExpression) {
                    BinaryExpression* binExpr = (BinaryExpression*)expression;
                    binExpr->right = new Identifier(current().value);
                }
                else if (expression->type == NodeType::UnaryExpression) {
                    UnaryExpression* unaryExpr = (UnaryExpression*)expression;
                    unaryExpr->expression = new Identifier(current().value);
                }
            }
        }

        else if (current().type == tokenType::ADDRESSOF) {
            if (expression == nullptr) {
                expression = new UnaryExpression("&");
            }
            else if (expression->type == NodeType::BinaryExpression) {
                BinaryExpression* binExpr = (BinaryExpression*)expression;
                binExpr->right = new UnaryExpression("&");
            }
            
        }
        if (shouldExpressionContinue()) {
            advance();
        }
    }
    // token here should be ";" or "," or ")"
    return expression;
}
bool Parser::shouldExpressionContinue() {
    return current().type != tokenType::SEMICOLON
    && current().type != tokenType::COMMA 
    && !(current().type == tokenType::PARENTHESES && current().value == ")");
}

ReturnStatement* Parser::parseReturnStatement() {
    advance(); // return
    ReturnStatement* returnStmt = new ReturnStatement();
    returnStmt->expression = parseExpression();
    return returnStmt;
}

ASTNode* Parser::parseFunctionCall() {
    FunctionCall* funcCall = new FunctionCall();
    std::string funcName = current().value;
    funcCall->name = funcName;
    advance(); // name
    // parameters here
    advance(); // (
    while (current().type != tokenType::PARENTHESES && current().value != ")") {
        ASTNode* expression = parseExpression();
        funcCall->arguments.push_back(expression);
        if (current().type == tokenType::COMMA) {
            advance(); // ,
        }
    }
    advance(); // )
    return funcCall;
}

ASTNode* Parser::parseDeclaration() {
    // int x;
    // int x = 1;
    std::string type = current().value;
    advance(); // type
    std::string name = current().value;
    if (peekNext().type == tokenType::SEMICOLON) { // int a;
        advance(); // varName
    }
    // if the next token is not a semicolon, stop at the variable name,
    // so the next parseStatement() would begin at "x = 1";
    return new VariableDeclaration(type,name);
}

ASTNode* Parser::parseAssignment() {
    // x = expression
    std::string name = current().value;
    Identifier* identifier = new Identifier(name);
    advance(); // varName
    advance(); // =
    ASTNode* expression = parseExpression();
    return new Assignment(identifier,expression);
}

long long Parser::calculateOperation(const long long& value1, const long long& value2, const std::string& operation) {
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