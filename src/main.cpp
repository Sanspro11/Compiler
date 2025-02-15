#include <vector>
#include <fstream>
#include <string>
#include "token.hpp"
#include "ASTnode.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "codeGen.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: compiler <filename>\n";
        exit(1);
    }

    std::string filename = argv[1];
    std::ifstream fileStream = std::ifstream(filename);
    if (!fileStream.is_open()) {
        std::cerr << "Error opening file: " << filename;
        exit(1);
    }

    std::vector<Token> tokens = Lexer::tokenize(fileStream);
    fileStream.close();
    std::cout << "List of tokens:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        tokens[i].print();
        std::cout << "\n";
    }
    std::cout << "\n";

    Parser parser = Parser(tokens);
    ProgramRoot* treeRoot = parser.parse();
    treeRoot->print();

    size_t nameSize = filename.size();
    if (filename.substr(nameSize-2,nameSize-1) == ".c") {
        filename[nameSize-1] = 'o';
    }
    else {
        filename += ".o";
    }

    codeGen programGenerator = codeGen();
    programGenerator.entryFunctionName = "main";
    bool success = programGenerator.generateObjectFile(treeRoot,filename);
    if (!success) {
        std::cout << "Error while making object file " << filename;
        exit(1);
    }
    std::cout << "Object file " << filename << " successfully created\n";
    exit(0);
}