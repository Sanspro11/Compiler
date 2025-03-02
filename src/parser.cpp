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
    return Token(tokenType::ENDOFFILE,"",0,0);
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
    return Token(tokenType::ENDOFFILE,"",0,0);
}

void Parser::require(const tokenType type, const std::string& name) {
    if (current().type != type) {
        std::cerr << "Row: " << current().row << ", Column: " << current().column << "\n";
        std::cerr << "Expected a : " << name << "\n";
        exit(1);
    }
    advance();
}

ASTNode* Parser::parseStatement() {
    ASTNode* statement = nullptr;
    if (current().type == tokenType::RETURN) {
        statement = parseReturnStatement();
        require(tokenType::SEMICOLON,";");
    }
    else if (current().type == tokenType::TYPE) { // declaration
        statement = parseDeclaration();
    }
    else if (current().type == tokenType::NAME) {
        if (peekNext().type == tokenType::PARENTHESES && peekNext().value == "(") {
            statement = parseFunctionCall();
        }
        else {
            statement = parseAssignment();
        }
        require(tokenType::SEMICOLON,";");
    }
    else if (current().type == tokenType::IF){
        statement = parseIfStatement();
    }
    else if (current().type == tokenType::WHILE){
        statement = parseWhileStatement();
    }

    return statement;  
}

ASTNode* Parser::parseFunction() {
    Function* function = new Function();
    function->returnType = current().value;
    require(tokenType::TYPE,"type");
    function->name = current().value;
    require(tokenType::NAME,"name");
    // parameters here
    require(tokenType::PARENTHESES,"(");
    while (current().type != tokenType::PARENTHESES && current().value != ")") {
        ASTNode* d = (ASTNode*)parseDeclaration();
        function->parameters.push_back(d);
    }
    require(tokenType::PARENTHESES,")");
    // parameters here
    function->codeBlock = parseCodeBlock();
    return function;
}

CodeBlock* Parser::parseCodeBlock() {
    CodeBlock* codeBlock = new CodeBlock();
    require(tokenType::CURLY_BRACKET,"{");
    while (current().type != tokenType::CURLY_BRACKET && current().value != "}") {
        ASTNode* statement = parseStatement();
        codeBlock->statements.push_back(statement);
    }
    require(tokenType::CURLY_BRACKET,"}");
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
                    value = calculateOperation(otherValue,value,op);
                    expression = new Constant(std::to_string(value));
                }
                else {
                    binExpr->right = new Constant(current().value);
                }
            }
            advance(); // constant
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
            advance(); // string
        }

        else if (current().type == tokenType::OPERATION) {
            if (expression == nullptr) {
                expression = new UnaryExpression(current().value);
            }
            else {
                BinaryExpression* binExpr = new BinaryExpression();
                binExpr->left = expression;
                binExpr->op = current().value;
                expression = binExpr;
            }
            advance(); // operation
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
                ASTNode* identifier = parseIdentifier();
                if (expression == nullptr) {
                    expression = identifier;
                }
                else if (expression->type == NodeType::BinaryExpression) {
                    BinaryExpression* binExpr = (BinaryExpression*)expression;
                    binExpr->right = identifier;
                }
                else if (expression->type == NodeType::UnaryExpression) {
                    UnaryExpression* unaryExpr = (UnaryExpression*)expression;
                    unaryExpr->expression = identifier;
                }
            }
        }

    }
    return expression;
}

bool Parser::shouldExpressionContinue() {
    return current().type != tokenType::SEMICOLON
    && current().type != tokenType::COMMA 
    && !(current().type == tokenType::PARENTHESES && current().value == ")")
    && current().type != tokenType::COMPARISON
    && !(current().type == tokenType::SQUARE_BRACKET && current().value == "]");
}

ReturnStatement* Parser::parseReturnStatement() {
    advance(); // return
    ReturnStatement* returnStmt = new ReturnStatement();
    returnStmt->expression = parseExpression();
    return returnStmt;
}

ASTNode* Parser::parseFunctionCall() {
    FunctionCall* funcCall = new FunctionCall();
    funcCall->name = current().value;
    require(tokenType::NAME,"name");
    require(tokenType::PARENTHESES,"(");
    while (current().type != tokenType::PARENTHESES && current().value != ")") {
        ASTNode* expression = parseExpression();
        funcCall->arguments.push_back(expression);
        if (current().type == tokenType::COMMA) {
            advance(); // ,
        }
    }
    require(tokenType::PARENTHESES,")");
    return funcCall;
}

ASTNode* Parser::parseDeclaration() {
    // int x;
    // int x = 1;
    // int* x = malloc(1024);
    // int x[100];
    std::string type = current().value;
    require(tokenType::TYPE,"type");
    bool isPointer = false;
    bool isLocalArray = false;
    size_t localArrSize = 0;
    while (current().type == tokenType::OPERATION && current().value == "*") {
        isPointer = true;
        advance(); // *
    }
    std::string name = current().value;
    if (peekNext().type == tokenType::SEMICOLON || peekNext().type == tokenType::COMMA // int a; int a,
    || peekNext().type == tokenType::PARENTHESES && peekNext().value == ")") { // int a)
        advance(); // varName
        if (!(current().type == tokenType::PARENTHESES && current().value == ")")) {
            advance(); // ; or , 
        }
    }
    else if (peekNext().type == tokenType::SQUARE_BRACKET && peekNext().value == "[") {
        advance(); // varName
        advance(); // [
        isLocalArray = true;
        if (current().type == tokenType::SQUARE_BRACKET && current().value == "]" ) { // in function parameter
            // int a[],
            advance(); // ]
            if (current().type == tokenType::COMMA)  {
                advance(); // ,
            }
            else if (current().type == tokenType::PARENTHESES) {
                // no advance
            }
            else {
                require(tokenType::COMMA,", or )");
            }
        }
        else {
            if (current().type != tokenType::CONSTANT) {
                std::cerr << current().row << ":" << current().column;
                std::cerr << " Local array size must be a constant\n";
                exit(1);
            }
            Constant* node = (Constant*)parseExpression();
            localArrSize = std::stoull(node->value);
            require(tokenType::SQUARE_BRACKET,"]");
            require(tokenType::SEMICOLON,";");
            // currently only supports declaration for local arrays
        }
    }
    // if the next token is not a semicolon, stop at the variable name,
    // so the next parseStatement() would begin at "x = 1";
    return new VariableDeclaration(type,name,isPointer,isLocalArray,localArrSize);
}

ASTNode* Parser::parseAssignment() {
    ASTNode* identifier = parseIdentifier();
    require(tokenType::ASSIGNMENT,"=");
    ASTNode* expression = parseExpression();
    return new Assignment(identifier,expression);
}

ASTNode* Parser::parseIfStatement() {
    advance(); // if 
    require(tokenType::PARENTHESES,"(");
    IfStatement* ifStatement = new IfStatement();
    ifStatement->expression = parseComparison();
    require(tokenType::PARENTHESES,")");
    ifStatement->codeBlock = parseCodeBlock();
    if (current().type == tokenType::ELSE) {
        advance(); // else
        ifStatement->elseBlock = parseCodeBlock();
    }
    return ifStatement;
}

ASTNode* Parser::parseWhileStatement() {
    advance(); // while
    require(tokenType::PARENTHESES,"(");
    WhileStatement* whileStatement = new WhileStatement();
    whileStatement->expression = parseComparison();
    require(tokenType::PARENTHESES,")");
    whileStatement->codeBlock = parseCodeBlock();
    return whileStatement;
}

ASTNode* Parser::parseComparison() {
    // currently supports only 2 expressions
    ComparisonExpression* expr = new ComparisonExpression();
    expr->left = parseExpression();
    expr->op = current().value; // > < == >= <=
    require(tokenType::COMPARISON," comparison operator");
    expr->right = parseExpression();
    // || && checks here in the future
    return expr;
}

ASTNode* Parser::parseIdentifier() {
    // name
    // name[expr]
    std::string name = current().value;
    Identifier* identifier = new Identifier(name);
    advance(); // name
    if (!(current().type == tokenType::SQUARE_BRACKET && current().value == "[")) {
        return identifier;
    }
    require(tokenType::SQUARE_BRACKET,"[");
    ASTNode* expression = parseExpression();
    require(tokenType::SQUARE_BRACKET,"]");
    return new ArrayAccess(identifier,expression);
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

    if (operation == "%") 
        return value1 % value2;
    return value1; // should never get to here
}