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
    VariableDeclaration,
    Identifier,
    UnaryExpression,
    Assignment,
    ComparisonExpression,
    IfStatement,
    WhileStatement,
    ArrayAccess,
    Struct,
    PropertyAccess,
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
        std::cout << "Return: ";
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
    std::string constantType = "uint64_t";
    Constant(std::string v) : value(v) { type = NodeType::Constant; }
    void print() const override {
        if (constantType == "string") {
            std::cout << "\"" << value << "\"";
        }
        else {
            std::cout << value;
        }
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
        std::cout << "(";
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

struct UnaryExpression : public ASTNode {
    ASTNode* expression;
    std::string op;
    UnaryExpression(const std::string& op) : op(op) { type = NodeType::UnaryExpression; }
    UnaryExpression() { type = NodeType::UnaryExpression; }

    void print() const override {
        std::cout << "(" << op << " ";
        if (expression) {
            expression->print();
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


struct FunctionCall : public ASTNode {
    std::string name;
    std::vector<ASTNode*> arguments;
    FunctionCall() { type = NodeType::FunctionCall; }
    void print() const override {
        std::cout << "FunctionCall: " << name;
        if (arguments.size() == 0) {
            std::cout << " with no arguments";
        }
        else {
            std::cout << " with arguments: ";
            for (const ASTNode* parameter : arguments) {
                parameter->print();
                std::cout << ", ";
            }
            std::cout << "\n";
        }
    }
};

struct Identifier : public ASTNode {
    std::string name;
    Identifier(const std::string& name) : name(name) { 
        type = NodeType::Identifier; 
    }
    void print() const override {
        std::cout << name;
    }
};

struct Assignment : public ASTNode {
    ASTNode* identifier;
    ASTNode* expression; 

    Assignment(ASTNode* target, ASTNode* expression)
        : identifier(target), expression(expression)
    {
        type = NodeType::Assignment;
    }

    void print() const override {
        std::cout << "Assignment: ";
        if (identifier) {
            identifier->print();
        }
        std::cout << " = ";
        if (expression) {
            expression->print();
        }
    }
};

struct VariableDeclaration : public ASTNode {
    std::string varType;
    std::string varName;
    size_t pointerCount;
    bool isLocalArray;
    bool isStruct;
    size_t localArrSize;

    VariableDeclaration(const std::string& varType, const std::string& varName,
    size_t pointerCount = 0, bool isLocalArray = false, size_t localArrSize = 0, bool isStruct = false)
    : varType(varType), varName(varName), pointerCount(pointerCount), isLocalArray(isLocalArray),
    localArrSize(localArrSize), isStruct(isStruct)
    {type = NodeType::VariableDeclaration;}

    void print() const override {
        std::cout << "VariableDeclaration: "; 
        if (isStruct) 
            std::cout << "struct ";
        std::cout << varType;
        for (size_t i = 0; i < pointerCount; ++i)
            std::cout << "*";
        std::cout << " " << varName;
        if (isLocalArray)
            std::cout << "[" << localArrSize << "]";
    }
};

struct Function : public ASTNode {
    CodeBlock* codeBlock;
    std::string returnType;
    std::string name;
    std::vector<ASTNode*> parameters;
    Function() { type = NodeType::Function;}
    void print() const override {
        std::cout << "Function: " << name << ", ";
        std::cout << "Return type: " << returnType << ",\n";
        if (!parameters.empty()) {
            std::cout << "with parameters: ";
            for (const ASTNode* d : parameters) {
                d->print();
                std::cout << ", ";
            }
        }
        codeBlock->print();
    }
};

struct ComparisonExpression : public ASTNode {
    ASTNode* left;  
    ASTNode* right;  
    std::string op;
    ComparisonExpression() { type = NodeType::ComparisonExpression; left = nullptr; right = nullptr; }
    void print() const override {
        std::cout << "(";
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

struct IfStatement : public ASTNode {
    ASTNode* expression;
    CodeBlock* codeBlock;
    CodeBlock* elseBlock;
    IfStatement() { type = NodeType::IfStatement; codeBlock = nullptr; elseBlock = nullptr;}
    void print() const override {
        std::cout << "If: ";
        expression->print();
        std::cout << "then: ";
        codeBlock->print();
        if (elseBlock) {
            std::cout << "else: ";
            elseBlock->print();
        }
    }
};

struct WhileStatement : public ASTNode {
    ASTNode* expression;
    CodeBlock* codeBlock;
    WhileStatement() { type = NodeType::WhileStatement; codeBlock = nullptr;}
    void print() const override {
        std::cout << "While: ";
        expression->print();
        codeBlock->print();
    }
};

struct ArrayAccess : public ASTNode {
    ASTNode* array;
    ASTNode* index;
    
    ArrayAccess(ASTNode* array, ASTNode* index) 
        : array(array), index(index) { 
        type = NodeType::ArrayAccess; 
    }
    
    void print() const override {
        std::cout << "ArrayAccess: ";
        if (array) {
            array->print();
        } else {
            std::cout << "NULL";
        }
        std::cout << "[";
        if (index) {
            index->print();
        } else {
            std::cout << "NULL";
        }
        std::cout << "]";
    }
};

struct Struct : public ASTNode {
    std::string name;
    std::vector<ASTNode*> properties;

    Struct(const std::string& name, std::vector<ASTNode*> properties) :
    name(name), properties(properties) { type = NodeType::Struct;}

    void print() const override {
        std::cout << "Struct: " << name << " with varaibles:\n";
        for (size_t i = 0; i < properties.size(); ++i) {
            std::cout << (i+1) << ". ";
            properties[i]->print();
            std::cout << '\n';
        }
    }
};

struct PropertyAccess : public ASTNode {
    ASTNode* Struct;
    std::string property;
    
    PropertyAccess(ASTNode* Struct, const std::string& property) 
        : Struct(Struct), property(property) { 
        type = NodeType::PropertyAccess; 
    }
    
    void print() const override {
        std::cout << "Property Access: ";
        if (Struct) {
            Struct->print();
        } else {
            std::cout << "NULL";
        }
        std::cout << '.' << property;
    }
};