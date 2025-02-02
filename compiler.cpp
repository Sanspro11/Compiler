#include <vector>
#include <fstream>
#include <string>
#include "token.cpp"
#include "ASTnode.cpp"
#include "parser.cpp"
#include "lexer.cpp"
#include "codeGenerator.cpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: compiler <filename>";
        exit(1);
    }

    std::string filename = argv[1];
    std::ifstream fileStream = std::ifstream(filename);
    if (!fileStream.is_open()) {
        std::cerr << "Error opening file: " << filename;
        exit(1);
    }

    std::vector<Token> tokens = lexer::tokenize(fileStream);
    fileStream.close();
    std::cout << "List of tokens:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << i << ". ";
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

    codeGenerator programGenerator = codeGenerator();
    programGenerator.entryFunctionName = "main";
    bool success = programGenerator.generateObjectFile(treeRoot,filename);
    if (!success) {
        std::cout << "Error while making object file " << filename;
        exit(1);
    }
    
    std::cout << "Object file " << filename << " successfully created\n";
    exit(0);
}