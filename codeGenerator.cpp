#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ASTnode.cpp"

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


// ELF symbol binding and type
inline unsigned char ELF64_ST_BIND(unsigned char bind) { return (bind << 4); }
inline unsigned char ELF64_ST_TYPE(unsigned char type) { return (type & 0xF); }
#define ELF64_R_INFO(sym, type)  (((uint64_t)(sym) << 32) | ((type) & 0xFFFFFFFF)) // Combine symbol and type

static const unsigned char LOCAL_SYMBOL  = 0; // Local symbol
static const unsigned char GLOBAL_SYMBOL = 1; // Global symbol

//symbols types
static const unsigned char OBJECT_SYMBOL_TYPE = 1; // Data object
static const unsigned char FUNCTION_SYMBOL_TYPE = 2; // Function
static const unsigned char SECTION_SYMBOL_TYPE= 3; // Section symbol

// relo types
static const unsigned char R_X86_64_PC32 = 2;
static const unsigned char R_X86_64_PLT32 = 4;


class codeGenerator {
public:
    std::string entryFunctionName;

    bool  generateObjectFile(ProgramRoot* root, const std::string filename) {

        // .text section ------------------------------------------------------------
        std::vector<uint8_t> textData;
        for (const ASTNode* element : root->programElements) {
            if (element->type == NodeType::Function) {
                std::vector<uint8_t> functionCode = generateCodeFromFunction((Function*)element);
                addCode(textData,functionCode);
            }
        }
        // take care of non-local functions
        {
            std::unordered_map<std::string,bool> finished;
            for (size_t i = 0; i < relaTextEntries.size(); ++i ) {
                std::string& funcName = relaFuncStrings[i];
                if (localFunctions.find(funcName) == localFunctions.end() && // not local function
                    finished.find(funcName) == finished.end()) {

                    finished[funcName] = true;
                    Symbol symbol{};

                    //symbol.st_name  = index in .strtab, taken care of later
                    symbol.st_info  = ELF64_ST_BIND(GLOBAL_SYMBOL) | ELF64_ST_TYPE(FUNCTION_SYMBOL_TYPE);
                    symbol.st_shndx = 0; // relocated
                    symbol.st_value = 0; // relocated
                    functionSymbols.push_back(symbol);
                    functionSymbolNames.push_back(funcName);
                }
            }
        }


        // .data section ------------------------------------------------------------
        std::vector<uint8_t> dataData = {
            0x78, 0x56, 0x34, 0x12 // random data
        };

        //strtab section ------------------------------------------------------------
        std::string strtabContents;
        strtabContents.push_back('\0');             // [0] empty
        /*
        strtabContents += "_start";  // offset=1
        strtabContents.push_back('\0');
        size_t offsetMain = 1;
        */

        size_t offsetMyGlobalData = strtabContents.size();
        strtabContents += "myGlobalData";
        strtabContents.push_back('\0');

        size_t offsetMyGlobalBss = strtabContents.size();
        strtabContents += "myGlobalBss";
        strtabContents.push_back('\0');

        // add a name for each function symbol
        for (size_t i = 0; i < functionSymbolNames.size(); ++i) {
            size_t offset = strtabContents.size();
            functionSymbols[i].st_name = offset; // put str offset in symbol
            strtabContents += functionSymbolNames[i];
            strtabContents.push_back('\0');
        }

        // add a name for each string symbol
        for (size_t i = 0; i < stringSymbols.size(); ++i) {
            size_t offset = strtabContents.size();
            std::string stringName = "string" + std::to_string(i);
            stringSymbols[i].st_name = offset; // put str offset in symbol
            strtabContents += stringName;
            strtabContents.push_back('\0');
        }


        // shstrtab section ------------------------------------------------------------
        // section header string table
        std::string shstrtabContents;
        shstrtabContents.push_back('\0');
        size_t offText    = 1;
        shstrtabContents += ".text";
        shstrtabContents.push_back('\0');
        size_t offData    = shstrtabContents.size();
        shstrtabContents += ".data";
        shstrtabContents.push_back('\0');
        size_t offBss     = shstrtabContents.size();
        shstrtabContents += ".bss";
        shstrtabContents.push_back('\0');
        size_t offSymtab  = shstrtabContents.size();
        shstrtabContents += ".symtab";
        shstrtabContents.push_back('\0');
        size_t offStrtab  = shstrtabContents.size();
        shstrtabContents += ".strtab";
        shstrtabContents.push_back('\0');
        size_t offRelaText = shstrtabContents.size();
        shstrtabContents += ".rela.text";
        shstrtabContents.push_back('\0');
        size_t offShstrtab = shstrtabContents.size();
        shstrtabContents += ".shstrtab";
        shstrtabContents.push_back('\0');
        size_t offRoData = shstrtabContents.size();
        shstrtabContents += ".rodata";
        shstrtabContents.push_back('\0');


        // .symtab section ------------------------------------------------------------
        std::vector<Symbol> symtab;
        symtab.resize(6 + functionSymbols.size() + stringSymbols.size()); // 6 static + functions + strings
        // NULL .text .data .bss data bss functions...

        // Making all symbols
        // 0) STN_UNDEF (all fields = 0)
        symtab[0] = Symbol{};

        // 1) .text section symbol
        {
            Symbol s{};
            s.st_name  = 0;  // Usually 0 for SECTION_SYMBOL_TYPE
            s.st_info  = ELF64_ST_BIND(LOCAL_SYMBOL) | ELF64_ST_TYPE(SECTION_SYMBOL_TYPE);
            s.st_other = 0;
            s.st_shndx = 1;  // index of .text
            s.st_value = 0;  // offset within .text
            s.st_size  = 0;  
            symtab[1] = s;
        }

        // 2) .data section symbol
        {
            Symbol s{};
            s.st_name  = 0;
            s.st_info  = ELF64_ST_BIND(LOCAL_SYMBOL) | ELF64_ST_TYPE(SECTION_SYMBOL_TYPE);
            s.st_shndx = 2;  // index of .data
            symtab[2] = s;
        }

        // 3) .bss section symbol
        {
            Symbol s{};
            s.st_name  = 0;
            s.st_info  = ELF64_ST_BIND(LOCAL_SYMBOL) | ELF64_ST_TYPE(SECTION_SYMBOL_TYPE);
            s.st_shndx = 3;  // index of .bss
            symtab[3] = s;
        }

        size_t symTabOffset = 4;

        // add all function string symbols
        for (size_t i = 0; i < stringSymbols.size(); ++i) {
            symtab[symTabOffset] = stringSymbols[i];
            stringNumToSymbolOffset.push_back(symTabOffset);
            ++symTabOffset;
        }
        // end of local symbols
        size_t endOfLocalSymbols = symTabOffset;
        // 4) myGlobalData (global object in .data)
        {
            Symbol s{};
            s.st_name  = offsetMyGlobalData;
            s.st_info  = ELF64_ST_BIND(GLOBAL_SYMBOL) | ELF64_ST_TYPE(OBJECT_SYMBOL_TYPE);
            s.st_shndx = 2;                 // .data
            s.st_value = 0;                 // offset in .data
            s.st_size  = dataData.size();   // 4 bytes
            symtab[symTabOffset] = s;
        }
        ++symTabOffset;

        // 5) myGlobalBss (global object in .bss)
        {
            Symbol s{};
            s.st_name  = offsetMyGlobalBss;
            s.st_info  = ELF64_ST_BIND(GLOBAL_SYMBOL) | ELF64_ST_TYPE(OBJECT_SYMBOL_TYPE);
            s.st_shndx = 3;      // .bss
            s.st_value = 0;      // offset in .bss
            s.st_size  = 4;      // 4 bytes reserved
            symtab[symTabOffset] = s;
        }
        ++symTabOffset;
        // add all function symbols
        for (size_t i = 0; i < functionSymbols.size(); ++i) {
            symtab[symTabOffset] = functionSymbols[i];
            std::string funcName = functionSymbolNames[i];
            nameToSymbolOffset[funcName] = symTabOffset;
            ++symTabOffset;
        }

        for (size_t i = 0; i < relaTextEntries.size(); ++i ) {

            if (localFunctions.find(relaFuncStrings[i]) != localFunctions.end()) { // local function
                uint32_t symIndex = nameToSymbolOffset[relaFuncStrings[i]];
                relaTextEntries[i].r_info = ELF64_R_INFO(symIndex,R_X86_64_PC32);
                // symbol index and R_X86_64_PC32 (2)
            }
            else { // library function
                uint32_t symIndex = nameToSymbolOffset[relaFuncStrings[i]];
                relaTextEntries[i].r_info = ELF64_R_INFO(symIndex,R_X86_64_PLT32);
                // no symbol index, PLT linking symbol
            }
        }
        for (size_t i = 0; i < stringRelaEntries.size(); ++i) {
            uint32_t symIndex = stringNumToSymbolOffset[i];
            stringRelaEntries[i].r_info = ELF64_R_INFO(symIndex,1);
        }



        const uint8_t* symtabRaw = reinterpret_cast<const uint8_t*>(symtab.data());
        size_t symtabSizeInBytes = symtab.size() * sizeof(Symbol);

        // elf header ------------------------------------------------------------
        const int numSections = 9;
        // null, .text, .data, .bss, .symtab, .strtab, .shstrtab, .rela.text 

        Elf64Header ehdr{};
        ehdr.e_ident[0] = 0x7F;
        ehdr.e_ident[1] = 'E';
        ehdr.e_ident[2] = 'L';
        ehdr.e_ident[3] = 'F';
        ehdr.e_ident[4] = 2; // elf 64 bit
        ehdr.e_ident[5] = 1; // elf little endian
        ehdr.e_ident[6] = 1; // elf version

        ehdr.e_type    = 1;  // relocatable
        ehdr.e_machine = 62; // x86_64 architecture
        ehdr.e_version = 1;  // elf version
        ehdr.e_entry   = 0;  // not used in .o
        ehdr.e_phoff   = 0;  // not used in .o
        ehdr.e_shoff = sizeof(Elf64Header); // start of section headers
        ehdr.e_flags   = 0;
        ehdr.e_ehsize  = sizeof(Elf64Header);
        ehdr.e_phentsize = 0;
        ehdr.e_phnum     = 0;
        ehdr.e_shentsize = sizeof(SectionHeader);
        ehdr.e_shnum     = numSections;
        // The section containing section names is .shstrtab (index=6)
        ehdr.e_shstrndx  = 7;


        // Section headers ------------------------------------------------------------
        std::vector<SectionHeader> shdr(numSections);

        // 0: null
        shdr[0].sh_type = 0; // section type NULL

        // 1: .text
        {
            SectionHeader &sh = shdr[1];
            sh.sh_name      = offText;
            sh.sh_type      = 1; // progbits section type (loaded into memory)
            sh.sh_flags     = 0x2 | 0x4; // loaded and excecutable
            sh.sh_offset    = 0;        // filled later
            sh.sh_size      = textData.size();
            sh.sh_addralign = 1;        // minimal alignment
        }

        // 2: .data
        {
            SectionHeader &sh = shdr[2];
            sh.sh_name      = offData;
            sh.sh_type      = 1; // progbits section type (loaded into memory)
            sh.sh_flags     = 0x2 | 0x1; // loaded and writable
            sh.sh_offset    = 0;        // filled later
            sh.sh_size      = dataData.size();
            sh.sh_addralign = 1;
        }

        // 3: .bss
        {
            SectionHeader &sh = shdr[3];
            sh.sh_name      = offBss;
            sh.sh_type      = 8; // nobits (for bss)
            sh.sh_flags     = 0x2 | 0x1; // loaded and writable
            sh.sh_offset    = 0;        // filled later
            sh.sh_size      = 4;  
            sh.sh_addralign = 1;
        }

        // 4: .symtab
        {
            SectionHeader &sh = shdr[4];
            sh.sh_name      = offSymtab;
            sh.sh_type      = 2; // symtab 
            sh.sh_link      = 5;
            sh.sh_info      = endOfLocalSymbols; // where global symbols starts 
            sh.sh_size      = symtabSizeInBytes;
            sh.sh_addralign = 8; // 8-byte alignment for Symbol
            sh.sh_entsize   = sizeof(Symbol);
        }

        // 5: .strtab
        {
            SectionHeader &sh = shdr[5];
            sh.sh_name      = offStrtab;
            sh.sh_type      = 3; // strtab
            sh.sh_size      = strtabContents.size();
            sh.sh_addralign = 1;
        }

        // 6: .rela.text 
        {
            SectionHeader &sh = shdr[6];
            sh.sh_name      = offRelaText;  
            sh.sh_type      = 4;           // SHT_RELA
            sh.sh_link      = 4; // index of .symtab
            sh.sh_info      = 1; // index of section to apply relocations (.text) (1)
            sh.sh_size      = (relaTextEntries.size() + stringRelaEntries.size()) * sizeof(Elf64_Rela);
            sh.sh_addralign = 8;
            sh.sh_entsize   = sizeof(Elf64_Rela);
        }

        // 7: .shstrtab
        {
            SectionHeader &sh = shdr[7];
            sh.sh_name      = offShstrtab;
            sh.sh_type      = 3; // strtab
            sh.sh_size      = shstrtabContents.size();
            sh.sh_addralign = 1;
        }

        // 8: .rodata
        {
            SectionHeader &sh = shdr[8];
            sh.sh_name      = offRoData;
            sh.sh_type      = 1; // SHT_PROGBITS (contains data)
            sh.sh_flags     = 0x02; // A (loaded into memory)           
            sh.sh_size      = rodataContents.size();
            sh.sh_addralign = 1;
        }
        // remember to add +1 to numSections when adding new section, and change shdr[i]


        // Writing to file -------------------------------------------
        uint64_t offset = sizeof(Elf64Header) + numSections * sizeof(SectionHeader);

        // .text
        shdr[1].sh_offset = offset;
        offset += shdr[1].sh_size;

        // .data
        shdr[2].sh_offset = offset;
        offset += shdr[2].sh_size;

        // .bss is SHT_NOBITS => doesn't occupy space in the file, so no offset increment
        shdr[3].sh_offset = offset; // Typically equals offset, but size is 0 in file

        // .symtab: align to 8 bytes if needed
        if (offset % 8 != 0) {
            uint64_t pad = 8 - (offset % 8);
            offset += pad;
        }
        shdr[4].sh_offset = offset;
        shdr[4].sh_size   = symtabSizeInBytes;
        offset += symtabSizeInBytes;

        // .strtab
        shdr[5].sh_offset = offset;
        offset += shdr[5].sh_size; // strtabContents.size()

        // .rela.text — also must be aligned to 8
        if (offset % 8 != 0) {
            uint64_t pad = 8 - (offset % 8);
            offset += pad;
        }
        shdr[6].sh_offset = offset;
        offset += shdr[6].sh_size;

        // .shstrtab
        shdr[7].sh_offset = offset;
        offset += shdr[7].sh_size;
        
        //.rodata
        shdr[8].sh_offset = offset;
        offset += shdr[8].sh_size;

        // 7) Write out the ELF file
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs) return false;

        // ELF header
        ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
        // Section headers
        ofs.write(reinterpret_cast<const char*>(shdr.data()), shdr.size() * sizeof(SectionHeader));
        // .text
        ofs.write(reinterpret_cast<const char*>(textData.data()), textData.size());
        // .data
        ofs.write(reinterpret_cast<const char*>(dataData.data()), dataData.size());
        // .bss => SHT_NOBITS => nothing to write
        // pad if needed for alignment to .symtab
        {
            uint64_t currentPos = ofs.tellp();
            uint64_t neededPad = (8 - (currentPos % 8)) % 8;
            if (neededPad) {
                std::vector<char> pad(neededPad, 0);
                ofs.write(pad.data(), pad.size());
            }
        }
        // .symtab
        ofs.write(reinterpret_cast<const char*>(symtabRaw), symtabSizeInBytes);
        // .strtab
        ofs.write(strtabContents.data(), strtabContents.size());
        // .rela.text
        {
            // pad if needed
            uint64_t currentPos = ofs.tellp();
            uint64_t neededPad = (8 - (currentPos % 8)) % 8;
            if (neededPad) {
                std::vector<char> pad(neededPad, 0);
                ofs.write(pad.data(), pad.size());
            }
            for (auto &rel : relaTextEntries) {
                ofs.write(reinterpret_cast<const char*>(&rel), sizeof(rel));
            }
            for (auto &rel : stringRelaEntries) {
                ofs.write(reinterpret_cast<const char*>(&rel), sizeof(rel));
            }
        }
        // .shstrtab
        ofs.write(shstrtabContents.data(), shstrtabContents.size());

        // .rodata
        ofs.write(rodataContents.data(), rodataContents.size());

        ofs.close();
        return true;
    }

private:
    // all relocations
    std::vector<Elf64_Rela> relaTextEntries;
    std::vector<Elf64_Rela> stringRelaEntries;
    std::vector<std::string> relaFuncStrings;

    std::vector<Symbol> functionSymbols;
    std::vector<Symbol> stringSymbols;
    std::vector<std::string> functionSymbolNames;
    std::unordered_map<std::string,bool> localFunctions;
    // functions that are in this file, and not linked

    std::string rodataContents;

    std::unordered_map<std::string,size_t> nameToSymbolOffset;
    std::vector<size_t> stringNumToSymbolOffset;
    size_t currentCodeOffset = 0;
    size_t currentStringsOffset = 0;
    
    static std::unordered_map<uint8_t,std::string> positionToRegister;
    static std::unordered_map<std::string,std::vector<uint8_t>> register64BitMov;
    static std::unordered_map<std::string,std::vector<uint8_t>> register64BitLeaStub;
    
    


    void addCode(std::vector<uint8_t>& code,const std::vector<uint8_t>& codeToAdd) {
        code.insert(code.end(),codeToAdd.begin(),codeToAdd.end());
    }

    std::vector<uint8_t> exitSyscall(uint8_t num) {
        std::vector<uint8_t> code = {
            0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00, // MOV RAx, 0x3C (ExIT SYSCALL)
            0x48, 0xC7, 0xC7, num, 0x00, 0x00, 0x00, // MOV RDI, NUM (0-255)
            0x0F, 0x05 // SYSCALL
        };
        return code;
    }

    std::vector<uint8_t> call() {
        std::vector<uint8_t> code = {
            0xe8, 0x00, 0x00, 0x00, 0x00  // call 0x00000000
        };
        return code;
        // the address of the call is being relocated by .rela.text
    } // call 0x00000000

    std::vector<uint8_t> movabs(const std::string& reg, uint64_t num) {
        auto movCode = register64BitMov[reg];
        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&num);
        for (size_t i = 0; i < 8; ++i) {
            movCode.push_back(*(bytePtr + i));
        }
        return movCode;
    } // movabs reg, num

    std::vector<uint8_t> leaStub(const std::string& reg) { 
        auto leaCode = register64BitLeaStub[reg];
        addCode(leaCode,{0x00,0x00,0x00,0x00});
        return leaCode;
        // stub lea for relocation
    }

    std::vector<uint8_t> pushRbp() { 
        return {0x55};
    } // push rbp

    std::vector<uint8_t> ret() { 
        return {0xC3};
    } // ret

    std::vector<uint8_t> movRspRbp() { 
        return {0x48,0x89,0xE5};
    } // mov rbp, rsp

    std::vector<uint8_t> movRaxQwordRbpOffset(uint32_t offset) { 
        std::vector<uint8_t> code = {0x48,0x8b,0x85};
        offset = ~offset;
        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&offset);
        for (size_t i = 0; i < 4; ++i) {
            code.push_back(*(bytePtr + i));
        }
        return code;
    } // mov rax, [rbp-0xOFFSET]

    std::vector<uint8_t> movRbpQwordOffsetRax(uint32_t offset) { 
        std::vector<uint8_t> code = {0x48,0x89,0x85};
        offset = ~offset;
        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&offset);
        for (size_t i = 0; i < 4; ++i) {
            code.push_back(*(bytePtr + i));
        }
        return code;
    } // mov [rbp-0xOFFSET], rax

    std::vector<uint8_t> subRsp(uint32_t num) { 
        std::vector<uint8_t> code = {0x48,0x81,0xEC};
        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&num);
        for (size_t i = 0; i < 4; ++i) {
            code.push_back(*(bytePtr + i));
        }
        return code;
    } // sub Rsp, num


    
    void addConstantStringToRegToCode(std::vector<uint8_t>& code,const Constant* constant, const std::string& reg) { 
        const std::string& value = constant->value;

        rodataContents += value;
        rodataContents.push_back('\x00');
        
        // symbol for the string
        Symbol sym{};
        sym.st_name  = 0; // handled later
        sym.st_info  = ELF64_ST_BIND(LOCAL_SYMBOL) | ELF64_ST_TYPE(OBJECT_SYMBOL_TYPE);
        sym.st_shndx = 8; // .rodata section index
        sym.st_value = currentStringsOffset; 
        sym.st_size  = value.size();
        currentStringsOffset += value.size() + 1; // include null terminator
        stringSymbols.push_back(sym);
        
        auto movabsCodeStub = movabs(reg,0);
        size_t stringAddressOffset = code.size() + currentCodeOffset + register64BitMov[reg].size();
        
        // add relocation entry
        Elf64_Rela rel{};
        rel.r_offset = stringAddressOffset;  
        rel.r_addend = 0;
        // rel.r_info is added later
        stringRelaEntries.push_back(rel);
        addCode(code,movabsCodeStub);
    } 

    void addReturnStatementToCode(std::vector<uint8_t>& code ,ReturnStatement*& returnStatement, bool inMain) {
        if (inMain) { // supposes int return
            Constant* constantValue;
            // only supports constant returns
            if (returnStatement->expression->type == NodeType::BinaryExpression) {
                // implement expression
            }
            else if (returnStatement->expression->type == NodeType::Constant && 
            ((Constant*)returnStatement->expression)->constantType == "uint64_t") {
                constantValue = (Constant*)returnStatement->expression;
            }
            std::string stringValue = constantValue->value;
            uint8_t value = std::stoll(stringValue);
            auto exitCode = exitSyscall(value);
            addCode(code,exitCode);
        }
        else {
            addCode(code,ret());
        }
    }

    void addFunctionCallToCode(std::vector<uint8_t>& code,FunctionCall*& functionCall) {
        std::vector<ASTNode*>& args = functionCall->arguments;

        for (size_t i = 0; i < args.size() && i <= 5; ++i) {
            if (args[i]->type == NodeType::Constant) {
                Constant* constant = (Constant*)args[i];
                if (constant->constantType == "uint64_t") {
                    uint64_t value = std::stoll(constant->value);
                    std::string reg = positionToRegister[i];
                    auto movCode = movabs(reg,value);
                    addCode(code,movCode);
                }
                else if (constant->constantType == "string") {
                    std::string reg = positionToRegister[i]; 
                    addConstantStringToRegToCode(code,constant,reg);
                }
            }
        }

        //call
        // add .rela.text entry
        Elf64_Rela rel{};
        rel.r_offset = currentCodeOffset + code.size() + 1;
        rel.r_addend = -4; // constant
        // add reloc.info later (.symtab index + relocation type)
        relaTextEntries.push_back(rel);
        relaFuncStrings.push_back(functionCall->name);
        auto callCode = call();
        addCode(code,callCode);
    }


    std::vector<uint8_t> generateCodeFromFunction(Function* function) {
        std::vector<uint8_t> code;
        bool inMain = function->name == entryFunctionName;

        auto stackStartCode = pushRbp();
        addCode(stackStartCode,movRspRbp());
        addCode(code,stackStartCode);
        for (const ASTNode* statement : function->codeBlock->statements) {
            if (statement->type == NodeType::ReturnStatement) {
                ReturnStatement* returnStatement = (ReturnStatement*)statement;
                addReturnStatementToCode(code,returnStatement,inMain);
            }

            else if (statement->type == NodeType::FunctionCall) {
                FunctionCall* functionCall = (FunctionCall*)statement;
                addFunctionCallToCode(code,functionCall);
            }
        }


        // end of function
        if (inMain) {
            auto exitCode = exitSyscall(0);
            addCode(code,exitCode);
        }
        else {
            addCode(code,ret());
        }
        // add symbol entry
        Symbol symbol{};

        //symbol.st_name  = index in .strtab, taken care of later
        symbol.st_info  = ELF64_ST_BIND(GLOBAL_SYMBOL) | ELF64_ST_TYPE(FUNCTION_SYMBOL_TYPE);
        symbol.st_shndx = 1;                // in .text
        symbol.st_value = currentCodeOffset;// offset from start of .text
        symbol.st_size  = code.size();  // function size
        functionSymbols.push_back(symbol);
        functionSymbolNames.push_back(function->name);
        localFunctions[function->name] = true;

        currentCodeOffset += code.size();
        return code;
    }
};
std::unordered_map<uint8_t,std::string> codeGenerator::positionToRegister = {
    {0,"rdi"},
    {1,"rsi"},
    {2,"rdx"},
    {3,"rcx"},
    {4,"r9"},
    {5,"r8"}
};
std::unordered_map<std::string,std::vector<uint8_t>> codeGenerator::register64BitMov {
    {"rax",{0x48,0xb8}},
    {"rdi",{0x48,0xbf}},
    {"rsi",{0x48,0xbe}},
    {"rdx",{0x48,0xba}},
    {"rcx",{0x48,0xb9}},
    {"r9",{0x49,0xb9}},
    {"r8",{0x49,0xb8}}
};

std::unordered_map<std::string,std::vector<uint8_t>> codeGenerator::register64BitLeaStub {
    {"rdi",{0x48,0x8d,0x3c,0x25}},
    {"rsi",{0x48,0x8d,0x34,0x25}},
    {"rdx",{0x48,0x8d,0x14,0x25}},
    {"rcx",{0x48,0x8d,0x0c,0x25}},
    {"r9",{0x4c,0x8d,0x0c,0x25}},
    {"r8",{0x4c,0x8d,0x04,0x25}}
};