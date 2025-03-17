#include "preprocessor.hpp"
#include <string>
#include <unordered_map>
#include <sstream>

std::string Preprocessor::preProcess(std::ifstream& fileStream) {
    std::string line;
    std::string finalCode;
    while (std::getline(fileStream,line)) {
        if (line.find("#define") == 0) {
            addMacro(line);
            continue;
        }
        finalCode += handleLine(line);
    }
    return finalCode;
}

std::string Preprocessor::handleLine(const std::string& line) {
    std::string newLine;
    std::string currentWord;
    bool inWord = false;
    for (char ch : line) {
        if (isalnum(ch) || ch == '_') {
            currentWord += ch;
            inWord = true;
        }
        else {
            if (inWord) {
                if (macros.find(currentWord) != macros.end()) {
                    newLine += macros[currentWord];
                }
                else {
                    newLine += currentWord;
                }
                currentWord.clear();
                inWord = false;
            }
            newLine += ch;
        }
    }

    newLine += '\n';
    return newLine;
}

void Preprocessor::addMacro(const std::string& line) {
    // #define x 123
    std::stringstream stream(line);
    std::string key;
    std::string value;
    stream >> key; // #define
    stream >> key; 
    stream >> value; 
    macros[key] = value;
}