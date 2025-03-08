#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ASTnode.hpp"
#include "codeGen.hpp"


bool codeGen::generateObjectFile(ProgramRoot* root, const std::string filename) {

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

    // making all symbols
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
    // null, .text, .data, .bss, .symtab, .strtab, .shstrtab, .rela.text, .rodata

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
    ehdr.e_shstrndx  = 7; // .shstrtab index


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

    // 7) write elf file
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

void codeGen::parseExpressionToReg(std::vector<uint8_t>& code, ASTNode* expression, std::string reg) {
    if (expression->type == NodeType::Constant) {
        Constant* constant = (Constant*)expression;
        if (constant->constantType == "uint64_t") {
            uint64_t value = std::stoll(constant->value);
            addCode(code,movabs(reg,value));
        }
        else if (constant->constantType == "string") {
            addConstantStringToRegToCode(code,constant,reg);
        }
        return;
    }

    if (expression->type == NodeType::Identifier) {
        Identifier* identifier = (Identifier*)expression;
        const Variable* var = variableNameToObject[identifier->name];
        if (var->isLocalArr) {
            addCode(code,leaRaxOffsetRbp(var->offset));
        }
        else {
            addCode(code,movRaxOffsetRbp(var->offset,var->getSize()));
        }
        if (reg != "rax") {
            addCode(code,movRegRax(reg));
        }
        return;
    }

    if (expression->type == NodeType::ArrayAccess) {
        ArrayAccess* arrAccess = (ArrayAccess*)expression;
        // only identifier for now
        if (arrAccess->array->type == NodeType::Identifier) {
            Identifier* identifier = (Identifier*)arrAccess->array;
            const Variable* var = variableNameToObject[identifier->name];
            parseExpressionToReg(code,arrAccess->index,"rax");
            addCode(code,movabs("rbx",var->getElementSize()));
            addCode(code,mulRbx(8));
            addCode(code,movRegRax("rbx"));
            parseExpressionToReg(code,identifier,"rax");
            addCode(code,addRaxRbx());
            addCode(code,movRaxQwordRax()); // dereference
            if (reg != "rax") {
                addCode(code,movRegRax(reg));
            }
        }
    }

    if (expression->type == NodeType::FunctionCall) {
        FunctionCall* functionCall = (FunctionCall*)expression;
        addFunctionCallToCode(code,functionCall); // returns in rax
        if (reg != "rax") {
            addCode(code,movRegRax(reg));
        }
        return;
    }

    if (expression->type == NodeType::UnaryExpression) {
        UnaryExpression* unaryExpr = (UnaryExpression*)expression;
        if (unaryExpr->op == "&") {
            if (unaryExpr->expression->type == NodeType::Identifier) {
                Identifier* identifier = (Identifier*)unaryExpr->expression;
                const Variable* var = variableNameToObject[identifier->name];
                addCode(code,leaRaxOffsetRbp(var->offset));
                if (reg != "rax") {
                    addCode(code,movRegRax(reg));
                }
            }
        }

        if (unaryExpr->op == "*") {
            parseExpressionToReg(code,unaryExpr->expression,"rax");
            addCode(code,movRaxQwordRax());
        }
        return;
    }
    if (expression->type == NodeType::BinaryExpression) {
        BinaryExpression* binExpr = (BinaryExpression*)expression;
        ASTNode* left = binExpr->left;
        ASTNode* right = binExpr->right;
        parseExpressionToReg(code,right,"rax");
        addCode(code,pushReg("rax"));
        parseExpressionToReg(code,left,"rax");
        addCode(code,popReg("rbx"));
        const std::string& op = binExpr->op;
        if (op == "+") {
            addCode(code,addRaxRbx());
        }
        if (op == "-") {
            addCode(code,subRaxRbx());
        }
        if (op == "*") {
            addCode(code,mulRbx(8));
        }
        if (op == "/") {
            addCode(code,movabs("rdx",0));
            addCode(code,divRbx(8));
        }
        if (op == "%") {
            addCode(code,movabs("rdx",0));
            addCode(code,divRbx(8));
            addCode(code,movRaxReg("rdx"));
        }
        if (reg != "rax") {
            addCode(code,movRegRax(reg));
        }
        return;
    }

}

void codeGen::addConstantStringToRegToCode(std::vector<uint8_t>& code,const Constant* constant, const std::string& reg) { 
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
    size_t stringAddressOffset = code.size() + currentFunctionOffset + register64BitMov[reg].size();
    
    // add relocation entry
    Elf64_Rela rel{};
    rel.r_offset = stringAddressOffset;  
    rel.r_addend = 0;
    // rel.r_info is added later
    stringRelaEntries.push_back(rel);
    addCode(code,movabsCodeStub);
} 

void codeGen::addReturnStatementToCode(std::vector<uint8_t>& code ,ReturnStatement* returnStatement) {
    parseExpressionToReg(code,returnStatement->expression,"rax");
    addCode(code,leaveFunction());
}

void codeGen::addFunctionCallToCode(std::vector<uint8_t>& code,FunctionCall* functionCall) {
    std::vector<ASTNode*>& args = functionCall->arguments;

    size_t size = std::min(args.size(),(size_t)6);
    for (size_t i = 0; i < size; ++i) {
        std::string reg = positionToRegister[i];
        parseExpressionToReg(code,args[i],"rax");
        addCode(code,pushReg("rax"));
    }

    for (size_t i = size; i > 0 ; --i) {
        std::string reg = positionToRegister[i-1];
        addCode(code,popReg(reg));
    }

    // add .rela.text entry
    Elf64_Rela rel{};
    rel.r_offset = currentFunctionOffset + code.size() + 1;
    rel.r_addend = -4; // constant
    // add reloc.info later (.symtab index + relocation type)
    relaTextEntries.push_back(rel);
    relaFuncStrings.push_back(functionCall->name);
    addCode(code,call());
}

void codeGen::addAssignmentToCode(std::vector<uint8_t>& code,Assignment* assignment) {
    // mov [rbp+offset], expression
    const ASTNode* identifierNode = assignment->identifier;
    if (identifierNode->type == NodeType::Identifier) {
        Identifier* identifier = (Identifier*)identifierNode;
        const Variable* var = variableNameToObject[identifier->name];
        parseExpressionToReg(code,assignment->expression,"rax");
        addCode(code,movOffsetRbpRax(var->offset,var->getSize()));
    }
    else if (identifierNode->type == NodeType::ArrayAccess) { 
        ArrayAccess* arrAccess = (ArrayAccess*)identifierNode;
        // only identifier for now
        if (arrAccess->array->type == NodeType::Identifier) {
            Identifier* identifier = (Identifier*)arrAccess->array;
            const Variable* var = variableNameToObject[identifier->name];
            parseExpressionToReg(code,arrAccess->index,"rax");
            uint8_t sizeOfElement = var->getElementSize();
            addCode(code,movabs("rbx",sizeOfElement)); 
            addCode(code,mulRbx(8));
            addCode(code,movRegRax("rbx"));
            parseExpressionToReg(code,identifier,"rax");
            addCode(code,addRaxRbx());
            addCode(code,pushReg("rax"));
            parseExpressionToReg(code,assignment->expression,"rbx");
            addCode(code,popReg("rax"));
            addCode(code,movPtrRaxRbx(sizeOfElement));
        }
    }
}


void codeGen::addCodeBlockToCode(std::vector<uint8_t>& code,CodeBlock* codeBlock) {
    for (const ASTNode* statement : codeBlock->statements) {
        if (statement->type == NodeType::ReturnStatement) {
            ReturnStatement* returnStatement = (ReturnStatement*)statement;
            addReturnStatementToCode(code,returnStatement);
        }

        else if (statement->type == NodeType::FunctionCall) {
            FunctionCall* functionCall = (FunctionCall*)statement;
            addFunctionCallToCode(code,functionCall);
        }

        else if (statement->type == NodeType::Assignment) {
            Assignment* assignment = (Assignment*)statement;
            addAssignmentToCode(code,assignment);
        }

        else if (statement->type == NodeType::IfStatement) {
            IfStatement* ifStatement = (IfStatement*)statement;
            addIfStatementToCode(code,ifStatement);
        }

        else if (statement->type == NodeType::WhileStatement) {
            WhileStatement* whileStatement = (WhileStatement*)statement;
            addWhileStatementToCode(code,whileStatement);
        }
    }
}

size_t codeGen::addDeclarations(const std::vector<ASTNode*>& parameters, size_t varSizes = 0) {
    for (const ASTNode* statement : parameters) {
        if (statement->type == NodeType::VariableDeclaration) {
            VariableDeclaration* d = (VariableDeclaration*)statement;
            if (d->pointerCount > 0) {
                varSizes += 8; // pointer size is always 8 bytes;
            }
            else if (d->isLocalArray) {
                varSizes += d->localArrSize;
            }
            else {
                varSizes += typeSizes[d->varType];
            }
            variableNameToObject[d->varName] = new Variable(varSizes,d->varType,d->pointerCount,d->isLocalArray,d->localArrSize);
        }
    }
    return varSizes;
}

void codeGen::addDeclarationsToCode(std::vector<uint8_t>& code, CodeBlock* codeBlock, std::vector<ASTNode*>& parameters) {
    size_t varSizes = addDeclarations(parameters);
    varSizes = addDeclarations(codeBlock->statements,varSizes);
    size_t pad = (16 - (varSizes % 16)) % 16; // pad to 16
    addCode(code,startFunction());
    if (varSizes > 0) {
        addCode(code,subRsp(varSizes + pad));
    }
    for (size_t i = 0; i < parameters.size(); ++i) {
        const std::string& varName = ((VariableDeclaration*)parameters[i])->varName;
        Variable* var = variableNameToObject[varName];
        const std::string& reg = positionToRegister[i];
        addCode(code,movRaxReg(reg));
        addCode(code,movOffsetRbpRax(var->offset,var->getSize()));
    }
}

void codeGen::addIfStatementToCode(std::vector<uint8_t>& code, IfStatement* ifStatement) {
    ASTNode* expression = ifStatement->expression;
    std::string op = "jmp";
    if (expression->type == NodeType::ComparisonExpression) {
        op = ((ComparisonExpression*)expression)->op;
    }
    parseComparsionExpressionCmp(code,expression);
    addCode(code,oppositeJump(op));
    size_t jmpLocation = code.size() - 4;
    size_t codeSizeBefore = code.size();
    addCodeBlockToCode(code,ifStatement->codeBlock);
    uint32_t jmpSize = code.size() - codeSizeBefore;
    changeJmpOffset(code,jmpLocation,jmpSize);
}

void codeGen::parseComparsionExpressionCmp(std::vector<uint8_t>& code, ASTNode* expression) {
    if (expression->type == NodeType::ComparisonExpression) {
        ComparisonExpression* compExpr = (ComparisonExpression*)expression;
        parseExpressionToReg(code,compExpr->left,"rax");
        addCode(code,pushReg("rax"));
        parseExpressionToReg(code,compExpr->right,"rbx");
        addCode(code,popReg("rax"));
        addCode(code,cmpRaxRbx());
    }
}

void codeGen::addWhileStatementToCode(std::vector<uint8_t>& code, WhileStatement* whileStatement) {
    ASTNode* expression = whileStatement->expression;
    std::string op = "jmp";
    if (expression->type == NodeType::ComparisonExpression) {
        op = ((ComparisonExpression*)expression)->op;
    }
    addCode(code,jump("jmp"));
    size_t firstJumpLocation = code.size() - 4;
    size_t codeBlockStart = code.size();
    addCodeBlockToCode(code,whileStatement->codeBlock);
    size_t firstJumpOffset = code.size() - codeBlockStart;
    parseComparsionExpressionCmp(code,expression);
    addCode(code,jump(op));
    size_t secondJumpLocation = code.size() - 4;
    size_t secondJumpOffset = codeBlockStart - code.size();
    
    changeJmpOffset(code,firstJumpLocation,firstJumpOffset);
    changeJmpOffset(code,secondJumpLocation,secondJumpOffset);
}

std::vector<uint8_t> codeGen::generateCodeFromFunction(Function* function) {
    std::vector<uint8_t> code;
    CodeBlock* codeBlock = function->codeBlock;
    std::vector<ASTNode*>& params = function->parameters;
    addDeclarationsToCode(code,codeBlock,params);

    addCodeBlockToCode(code,codeBlock);

    bool inMain = (function->name == entryFunctionName);
    if (inMain) {
        addCode(code,movabs("rax",0));
    }
    addCode(code,leaveFunction());

    // add symbol entry
    Symbol symbol{};

    //symbol.st_name  = index in .strtab, taken care of later
    symbol.st_info  = ELF64_ST_BIND(GLOBAL_SYMBOL) | ELF64_ST_TYPE(FUNCTION_SYMBOL_TYPE);
    symbol.st_shndx = 1;                // in .text
    symbol.st_value = currentFunctionOffset; // offset from start of .text
    symbol.st_size  = code.size();  // function size
    functionSymbols.push_back(symbol);
    functionSymbolNames.push_back(function->name);
    localFunctions[function->name] = true;

    currentFunctionOffset += code.size();
    variableNameToObject.clear();
    return code;
}

void codeGen::changeJmpOffset(std::vector<uint8_t>& code, size_t codeOffset, uint32_t jmpSize) {
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&jmpSize);
    for (size_t i = 0; i < 4; ++i) {
        code[codeOffset] = (*(bytePtr + i));
        ++codeOffset;
    }
}

void codeGen::addCode(std::vector<uint8_t>& code,const std::vector<uint8_t>& codeToAdd) {
    code.insert(code.end(),codeToAdd.begin(),codeToAdd.end());
}