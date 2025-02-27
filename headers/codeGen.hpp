#pragma once
#include <string>
#include <unordered_map>
#include "ASTnode.hpp"

#pragma pack(push, 1) // no padding between struct properties

struct Elf64Header {
    unsigned char e_ident[16]; // ELF identification
    uint16_t e_type;           // Object file type
    uint16_t e_machine;        // Machine architecture
    uint32_t e_version;        // ELF version
    uint64_t e_entry;          // Entry point address (unused in relocatable)
    uint64_t e_phoff;          // Program header table offset
    uint64_t e_shoff;          // Section header table offset
    uint32_t e_flags;          // Processor-specific flags
    uint16_t e_ehsize;         // ELF header size
    uint16_t e_phentsize;      // Program header size
    uint16_t e_phnum;          // Number of program headers
    uint16_t e_shentsize;      // Section header size
    uint16_t e_shnum;          // Number of section headers
    uint16_t e_shstrndx;       // Section name string table index
};

struct SectionHeader {
    uint32_t sh_name;      // Section name (index into shstrtab)
    uint32_t sh_type;      // Section type
    uint64_t sh_flags;     // Section flags
    uint64_t sh_addr;      // Virtual address in memory (unused in relocatable)
    uint64_t sh_offset;    // Offset in file
    uint64_t sh_size;      // Size of section
    uint32_t sh_link;      // Link to other section
    uint32_t sh_info;      // Additional section info (e.g. symtab "local" cutoff)
    uint64_t sh_addralign; // Alignment of section
    uint64_t sh_entsize;   // Entry size (if section holds a table)
};

struct Symbol {
    uint32_t st_name;  // Symbol name (index into .strtab)
    unsigned char st_info; 
    unsigned char st_other; 
    uint16_t st_shndx; // Section index
    uint64_t st_value; // Value (address or offset)
    uint64_t st_size;  // Size of symbol
};

struct Elf64_Rela {
    uint64_t r_offset;  // Offset in the section to be relocated
    uint64_t r_info;    // Symbol table index and type of relocation
    int64_t  r_addend;  // Constant addend
};

#pragma pack(pop) // return the padding

class codeGen {
    public:
        std::string entryFunctionName;

        bool generateObjectFile(ProgramRoot* root, const std::string filename);

    private:
        // Variables
        std::vector<Elf64_Rela> relaTextEntries;
        std::vector<Elf64_Rela> stringRelaEntries;
        std::vector<std::string> relaFuncStrings;

        std::vector<Symbol> functionSymbols;
        std::vector<Symbol> stringSymbols;
        std::vector<std::string> functionSymbolNames;
        std::unordered_map<std::string,bool> localFunctions;

        std::string rodataContents;

        std::unordered_map<std::string,size_t> nameToSymbolOffset;
        std::vector<size_t> stringNumToSymbolOffset;
        size_t currentFunctionOffset = 0;
        size_t currentStringsOffset = 0;

        std::unordered_map<std::string,size_t> variableToOffset;
        std::unordered_map<std::string,std::string> variableToType;

        static std::unordered_map<uint8_t,std::string> positionToRegister;
        static std::unordered_map<std::string,std::vector<uint8_t>> register64BitMov;
        static std::unordered_map<std::string,std::vector<uint8_t>> register64BitLeaStub;
        static std::unordered_map<std::string,std::vector<uint8_t>> movRegRaxMap;
        static std::unordered_map<std::string,std::vector<uint8_t>> movRaxRegMap;
        static std::unordered_map<std::string,uint8_t> typeSizes;
        static std::unordered_map<std::string,std::vector<uint8_t>> pushRegCode;
        static std::unordered_map<std::string,std::vector<uint8_t>> popRegCode;
        static std::unordered_map<std::string,std::string> oppositeJumpType;
        static std::unordered_map<std::string,std::vector<uint8_t>> jumpType;



        // Functions
        void addCode(std::vector<uint8_t>& code, const std::vector<uint8_t>& codeToAdd);
        void parseExpressionToReg(std::vector<uint8_t>& code, ASTNode* expression, std::string reg);
        void parseComparsionExpressionCmp(std::vector<uint8_t>& code, ASTNode* expression);
        void addConstantStringToRegToCode(std::vector<uint8_t>& code, const Constant* constant, const std::string& reg);
        void addReturnStatementToCode(std::vector<uint8_t>& code, ReturnStatement* returnStatement);
        void addFunctionCallToCode(std::vector<uint8_t>& code, FunctionCall* functionCall);
        void addAssignmentToCode(std::vector<uint8_t>& code, Assignment* assignment);
        void addCodeBlockToCode(std::vector<uint8_t>& code, CodeBlock* codeBlock);
        void addDeclarationsToCode(std::vector<uint8_t>& code, CodeBlock* codeBlock);
        void addIfStatementToCode(std::vector<uint8_t>& code, IfStatement* ifStatement);
        void addWhileStatementToCode(std::vector<uint8_t>& code, WhileStatement* whileStatement);
        std::vector<uint8_t> generateCodeFromFunction(Function* function);


        // Code translation functions
        std::vector<uint8_t> exitSyscall(uint8_t num);
        std::vector<uint8_t> call();
        std::vector<uint8_t> movabs(const std::string& reg, uint64_t num);
        std::vector<uint8_t> leaStub(const std::string& reg);
        std::vector<uint8_t> pushReg(const std::string& reg);
        std::vector<uint8_t> popReg(const std::string& reg);
        std::vector<uint8_t> leave();
        std::vector<uint8_t> leaveFunction();
        std::vector<uint8_t> startFunction();
        std::vector<uint8_t> ret();
        std::vector<uint8_t> movRspRbp();
        std::vector<uint8_t> movRaxOffsetRbp(uint32_t offset, uint8_t size);
        std::vector<uint8_t> movOffsetRbpRax(uint32_t offset, uint8_t size);
        std::vector<uint8_t> leaRaxOffsetRbp(uint32_t offset);
        std::vector<uint8_t> subRsp(uint32_t num);
        std::vector<uint8_t> movRegRax(const std::string& reg);
        std::vector<uint8_t> movRaxReg(const std::string& reg);
        std::vector<uint8_t> addRaxRbx();
        std::vector<uint8_t> subRaxRbx();
        std::vector<uint8_t> mulRbx();
        std::vector<uint8_t> divRbx();
        std::vector<uint8_t> cmpRaxRbx();
        std::vector<uint8_t> jump(const std::string& type);
        std::vector<uint8_t> oppositeJump(const std::string& type);
        std::vector<uint8_t> movRaxQwordRax();
        std::vector<uint8_t> movPtrRaxRbx(uint8_t size);


        void addNumToCode(std::vector<uint8_t>& code, uint64_t num, uint8_t size);
        void changeJmpOffset(std::vector<uint8_t>& code, size_t codeOffset, uint32_t jmpSize);

        // Macros
        // ELF symbol binding and type
        inline unsigned char ELF64_ST_BIND(unsigned char bind) { return (bind << 4); }
        inline unsigned char ELF64_ST_TYPE(unsigned char type) { return (type & 0xF); }
        #define ELF64_R_INFO(sym, type)  (((uint64_t)(sym) << 32) | ((type) & 0xFFFFFFFF)) // Combine symbol and type

        static const unsigned char LOCAL_SYMBOL  = 0; // Local symbol
        static const unsigned char GLOBAL_SYMBOL = 1; // Global symbol

        // symbols types
        static const unsigned char OBJECT_SYMBOL_TYPE = 1; // Data object
        static const unsigned char FUNCTION_SYMBOL_TYPE = 2; // Function
        static const unsigned char SECTION_SYMBOL_TYPE= 3; // Section symbol

        // relo types
        static const unsigned char R_X86_64_PC32 = 2;
        static const unsigned char R_X86_64_PLT32 = 4;

};