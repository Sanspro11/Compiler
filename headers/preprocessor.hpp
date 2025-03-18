#pragma once
#include <string>
#include <fstream>
#include <unordered_map>

class Preprocessor {
    public:
        std::string preProcess(std::ifstream& fileStream);

    private:
        bool inString;
        bool inComment;
        std::unordered_map<std::string,std::string> macros;
        std::string handleLine(const std::string& line);
        void addMacro(const std::string& line);
};