#pragma once
#include <string>
#include <iostream>
#include <vector>

enum class NodeType {
    ReturnStatement,
    Constant,
    Expression,
    Statement,
    CodeBlock,
    Function,
    ProgramRoot,
    Operation,
    FunctionCall
};

struct ASTNode {
    virtual ~ASTNode() = default;
    NodeType type;
    virtual void print() {};
};


struct ReturnStatement : public ASTNode {
    ASTNode* expression;  
    ReturnStatement() { type = NodeType::ReturnStatement; }
    void print() override {
        std::cout << "Return statement: ";
        expression->print();
    }
};

struct Constant : public ASTNode {
    std::string value;  
    Constant(std::string v) : value(v) { type = NodeType::Constant; }
    void print() override {
        std::cout << "Constant: " << value;
    }
};

struct Expression : public ASTNode {
    ASTNode* value;
    ASTNode* value2;
    Expression(ASTNode* v) : value(v) { type = NodeType::Expression; }
    void print() override {
        std::cout << "Expression: ";
        value->print();
    }
};

struct CodeBlock : public ASTNode {
    std::vector<ASTNode*> statements;
    CodeBlock() { type = NodeType::CodeBlock; }
    void print() override {
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
    void print() override {
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
    void print() override {
        std::cout << "Function: " << name << ", ";
        std::cout << "Return type: " << returnType << ",\n";
        codeBlock->print();
    }
};

struct FunctionCall : public ASTNode {
    std::string name;
    std::vector<std::string> parameters;
    FunctionCall() { type = NodeType::FunctionCall; }
    void print() override {
        std::cout << "FunctionCall: " << name;
    }
};