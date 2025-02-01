#pragma once
#include <string>
#include <iostream>
#include <vector>

enum class NodeType {
    ReturnStatement,
    Constant,
    Expression,
    BinaryExpression,
    Statement,
    CodeBlock,
    Function,
    ProgramRoot,
    Operation,
    FunctionCall,
};

struct ASTNode {
    virtual ~ASTNode() = default;
    NodeType type;
    virtual void print() const {};
};


struct ReturnStatement : public ASTNode {
    ASTNode* expression;  
    ReturnStatement() { type = NodeType::ReturnStatement; expression = nullptr; }
    void print() const override {
        std::cout << "Return statement: ";
        if (expression != nullptr){
            expression->print();
        }
        else {
           std::cout << "Void return";
        }
    }
};

struct Constant : public ASTNode {
    std::string value;  
    Constant(std::string v) : value(v) { type = NodeType::Constant; }
    void print() const override {
        std::cout << value;
    }
};

struct BinaryExpression : public ASTNode {
    ASTNode* left;
    std::string op;
    ASTNode* right;
    BinaryExpression(ASTNode* left, const std::string& op, ASTNode* right) :
    left(left), op(op), right(right) { type = NodeType::BinaryExpression; }
    BinaryExpression() { type = NodeType::BinaryExpression; }

    void print() const override {
        if (left) {
            left->print();
        }
        else {
            std::cout << "NULL";
        }
        std::cout << " " << op << " ";
        if (right) {
            right->print();
        }
        else {
            std::cout << "NULL";
        }
        std::cout << ")";
    }
};

struct CodeBlock : public ASTNode {
    std::vector<ASTNode*> statements;
    CodeBlock() { type = NodeType::CodeBlock; }
    void print() const override {
        std::cout << "CodeBlock:\n";
        for (int i = 0; i < statements.size(); ++i) {
            std::cout << i << ": ";
            statements[i]->print();
            std::cout << "\n";
        }
    }
};

struct ProgramRoot : public ASTNode {
    std::vector<ASTNode*> programElements;
    ProgramRoot() { type = NodeType::CodeBlock; }
    void print() const override {
        std::cout << "Program:\n";
        for (int i = 0; i < programElements.size(); ++i) {
            std::cout << i << ": ";
            programElements[i]->print();
            std::cout << "\n";
        }
    }
};

struct Function : public ASTNode {
    CodeBlock* codeBlock;
    std::string returnType;
    std::string name;
    std::vector<std::string> parameters;
    Function() { type = NodeType::Function; }
    void print() const override {
        std::cout << "Function: " << name << ", ";
        std::cout << "Return type: " << returnType << ",\n";
        codeBlock->print();
    }
};

struct FunctionCall : public ASTNode {
    std::string name;
    std::vector<ASTNode*> parameters;
    FunctionCall() { type = NodeType::FunctionCall; }
    void print() const override {
        std::cout << "FunctionCall: " << name;
        if (parameters.size() == 0) {
            std::cout << " with no parameters";
        }
        else {
            std::cout << " with parameters: ";
            for (const ASTNode* parameter : parameters) {
                parameter->print();
            }
            std::cout << "\n";
        }
    }
};