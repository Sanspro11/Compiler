#include "preprocessor.hpp"
#include <string>
#include <unordered_map>
#include <sstream>

std::string Preprocessor::preProcess(std::ifstream& fileStream) {
    std::string line;
    std::string finalCode;
    inString = false;
    while (std::getline(fileStream,line)) {
        if (line.find("#define") == 0) {
            addMacro(line);
            finalCode += '\n'; // so the row numbers would match
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

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];

        if (ch == '/' && (i+1 < line.size() && line[i+1] == '/')) { // comment
            break;
        }

        if (ch == '/' && (i+1 < line.size() && line[i+1] == '*')) { /* comment start */
            inComment = true;
            continue;
        }

        if (ch == '*' && (i+1 < line.size() && line[i+1] == '/')) { /* comment end */
            inComment = false;
            ++i;
            continue;
        }

        if (inComment) {
            continue;
        }

        if (ch == '"' && (i == 0 || line[i - 1] != '\\')) { // string
            inString = !inString;
        }

        if (inString) {
            newLine += ch;
            continue;
        }

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