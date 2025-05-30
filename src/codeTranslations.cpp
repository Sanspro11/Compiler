#include "CodeGen.hpp"
#include <vector>
#include <unordered_map>

std::vector<uint8_t> CodeGen::exitSyscall(uint8_t num) {
    std::vector<uint8_t> code = {
        0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00, // MOV RAX, 0x3C (EXIT SYSCALL)
        0x48, 0xC7, 0xC7, num, 0x00, 0x00, 0x00, // MOV RDI, NUM (0-255)
        0x0F, 0x05 // SYSCALL
    };
    return code;
}

std::vector<uint8_t> CodeGen::call() {
    return {0xE8, 0x00, 0x00, 0x00, 0x00};
    // the address of the call is being relocated by .rela.text
} // call 0x00000000

std::vector<uint8_t> CodeGen::movabs(const std::string& reg, uint64_t num) {
    std::vector<uint8_t> code = register64BitMov[reg];
    addNumToCode(code, num, 8);
    return code;
} // movabs reg, num

std::vector<uint8_t> CodeGen::leaStub(const std::string& reg) { 
    std::vector<uint8_t> leaCode = register64BitLeaStub[reg];
    addCode(leaCode,{0x00,0x00,0x00,0x00});
    return leaCode;
} // lea reg, 0x00000000

std::vector<uint8_t> CodeGen::pushReg(const std::string& reg) { 
    return pushRegCode[reg];
} // push reg

std::vector<uint8_t> CodeGen::popReg(const std::string& reg) { 
    return popRegCode[reg];
} // pop reg

std::vector<uint8_t> CodeGen::leave() { 
    return {0xC9};
} // leave, which is: 
// mov rsp, rbp
// pop rbp

std::vector<uint8_t> CodeGen::leaveFunction() {
    return {0xC9,0xC3};
} // leave then ret

std::vector<uint8_t> CodeGen::startFunction() {
    return {0x55,0x48,0x89,0xE5};
} // push rbp & mov rbp,rsp 

std::vector<uint8_t> CodeGen::ret() { 
    return {0xC3};
} // ret

std::vector<uint8_t> CodeGen::movRspRbp() { 
    return {0x48,0x89,0xE5};
} // mov rbp, rsp

std::vector<uint8_t> CodeGen::movRaxOffsetRbp(uint32_t offset, uint8_t size) { 
    std::vector<uint8_t> code = {0x48,0x8b,0x85}; // mov rax, qword ptr [rbp-offset]
    if (size == 4)
        code = {0x8b,0x85}; // mov eax, dword ptr [rbp-offset]
    if (size == 2)
        code = {0x66,0x8b,0x85}; // mov ax, word ptr [rbp-offset]
    if (size == 1)
        code = {0x8a,0x85}; // mov al, byte ptr [rbp-offset]
    offset = ~offset + 1;
    addNumToCode(code,offset,4);
    return code;
} // mov rax/eax/ax/al, qword/dword/word/byte ptr [rbp-0xOFFSET]

std::vector<uint8_t> CodeGen::movOffsetRbpRax(uint32_t offset, uint8_t size) { 
    std::vector<uint8_t> code = {0x48,0x89,0x85}; // mov qword ptr [rbp-offset], rax
    if (size == 4)
        code = {0x89,0x85}; // mov dword ptr [rbp-offset], eax
    if (size == 2)
        code = {0x66,0x89,0x85}; // mov word ptr [rbp-offset], ax
    if (size == 1)
        code = {0x88,0x85}; // mov byte ptr [rbp-offset], al
    offset = ~offset + 1;
    addNumToCode(code,offset,4);
    return code;
} // mov Qword/Dword/Word/Byte ptr [rbp-0xOFFSET], Rax/Eax/Ax/Al

std::vector<uint8_t> CodeGen::leaRaxOffsetRbp(uint32_t offset) { 
    std::vector<uint8_t> code = {0x48,0x8d,0x85};
    offset = ~offset + 1;
    addNumToCode(code,offset,4);
    return code;
} // lea rax, [rbp-0xOFFSET]
// lea rax, [rbp+rax*8+6]

std::vector<uint8_t> CodeGen::subRsp(uint32_t num) { 
    std::vector<uint8_t> code = {0x48,0x81,0xEC};
    addNumToCode(code,num,4);
    return code;
} // sub rsp, num

std::vector<uint8_t> CodeGen::movRegRax(const std::string& reg) { 
    return movRegRaxMap[reg];
} // mov reg, rax

std::vector<uint8_t> CodeGen::movRaxReg(const std::string& reg) { 
    return movRaxRegMap[reg];
} // mov rax, reg

std::vector<uint8_t> CodeGen::addRaxRbx() { 
    return {0x48,0x01,0xd8};
} // add rax, rbx

std::vector<uint8_t> CodeGen::subRaxRbx() { 
    return {0x48,0x29,0xd8};
} // sub rax, rbx

std::vector<uint8_t> CodeGen::mulRbx(uint8_t size) {
    if (size == 4)
        return {0xf7,0xe3};
    if (size == 2)
        return {0x66,0xf7,0xe3};
    if (size == 1)
        return {0xf6,0xe3};
    return {0x48,0xf7,0xe3};
} // mul rbx (result in rdx:rax)

std::vector<uint8_t> CodeGen::divRbx(uint8_t size) {
    if (size == 4)
        return {0xf7,0xf3};
    if (size == 2)
        return {0x66,0xf7,0xf3};
    if (size == 1)
        return {0xf6,0xf3};
    return {0x48,0xf7,0xf3};
} // div rbx (RAX quotient, RDX remainder)

std::vector<uint8_t> CodeGen::cmpRaxRbx() { 
    return {0x48,0x39,0xd8};
} // cmp rax, rbx

std::vector<uint8_t> CodeGen::oppositeJump(const std::string& type) { 
    std::string oppositeOp = oppositeJumpType[type];
    std::vector<uint8_t> jmp = jumpType[oppositeOp];
    addCode(jmp,{0x00,0x00,0x00,0x00});
    return jmp;
} // je/jne/ja/jb/jae/jbe 0x00000000

std::vector<uint8_t> CodeGen::jump(const std::string& type) { 
    std::vector<uint8_t> jmp = jumpType[type];
    addCode(jmp,{0x00,0x00,0x00,0x00});
    return jmp;
} // je/jne/ja/jb/jae/jbe 0x00000000

std::vector<uint8_t> CodeGen::movRaxQwordRax() {
    return {0x48,0x8b,0x00};
} // mov rax, [rax]

std::vector<uint8_t> CodeGen::movPtrRaxRbx(uint8_t size) {
    if (size == 8)
        return {0x48,0x89,0x18}; // mov [rax], Rbx
    if (size == 4)
        return {0x89,0x18}; // mov [rax], Ebx
    if (size == 2)
        return {0x66,0x89,0x18}; // mov [rax], Bx
    return {0x88,0x18}; // mov [rax], bl
} // mov Qword/Dword/Word/Byte ptr [rax], Rbx/Ebx/Bx/Bl

void CodeGen::addNumToCode(std::vector<uint8_t>& code, uint64_t num, uint8_t size) {
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&num);
    for (size_t i = 0; i < size; ++i) {
        code.push_back(*(bytePtr + i));
    }
}

std::unordered_map<uint8_t,std::string> CodeGen::positionToRegister = {
    {0,"rdi"},
    {1,"rsi"},
    {2,"rdx"},
    {3,"rcx"},
    {4,"r8"},
    {5,"r9"}
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::register64BitMov {
    {"rax",{0x48,0xb8}},
    {"rbx",{0x48,0xbb}},
    {"rcx",{0x48,0xb9}},
    {"rdx",{0x48,0xba}},
    {"rdi",{0x48,0xbf}},
    {"rsi",{0x48,0xbe}},
    {"rdx",{0x48,0xba}},
    {"rcx",{0x48,0xb9}},
    {"r9",{0x49,0xb9}},
    {"r8",{0x49,0xb8}}
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::register64BitLeaStub {
    {"rdi",{0x48,0x8d,0x3c,0x25}},
    {"rsi",{0x48,0x8d,0x34,0x25}},
    {"rdx",{0x48,0x8d,0x14,0x25}},
    {"rcx",{0x48,0x8d,0x0c,0x25}},
    {"r9",{0x4c,0x8d,0x0c,0x25}},
    {"r8",{0x4c,0x8d,0x04,0x25}}
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::movRegRaxMap {
    {"rbx",{0x48,0x89,0xc3}},
    {"rdi",{0x48,0x89,0xc7}},
    {"rsi",{0x48,0x89,0xc6}},
    {"rdx",{0x48,0x89,0xc2}},
    {"rcx",{0x48,0x89,0xc1}},
    {"r9",{0x49,0x89,0xc1}},
    {"r8",{0x49,0x89,0xc0}}
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::movRaxRegMap {
    {"rdi", {0x48,0x89,0xf8}},  
    {"rsi", {0x48,0x89,0xf0}},  
    {"rdx", {0x48,0x89,0xd0}},  
    {"rcx", {0x48,0x89,0xc8}},  
    {"r8", {0x4c,0x89,0xc0}},  
    {"r9", {0x4c,0x89,0xc8}},  
    {"rdx",{0x48,0x89,0xd0}},
};

std::unordered_map<std::string,uint8_t> CodeGen::typeSizes {
    {"uint8_t",1},
    {"uint16_t",2},
    {"uint32_t",4},
    {"uint64_t",8},
    {"char",1},
    {"short",2},
    {"int",4},
    {"long long",8},
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::pushRegCode {
    {"rax",{0x50}},
    {"rbx",{0x53}},
    {"rcx",{0x51}},
    {"rdx",{0x52}},
    {"rbp",{0x55}},
    {"rsp",{0x54}},
    {"rdi",{0x57}},
    {"rsi",{0x56}},
    {"r8",{0x41,0x50}},
    {"r9",{0x41,0x51}},
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::popRegCode {
    {"rax",{0x58}},
    {"rbx",{0x5b}},
    {"rcx",{0x59}},
    {"rdx",{0x5a}},
    {"rbp",{0x5d}},
    {"rsp",{0x5c}},
    {"rdi",{0x5f}},
    {"rsi",{0x5e}},
    {"r8",{0x41,0x58}},
    {"r9",{0x41,0x59}},
};

std::unordered_map<std::string,std::vector<uint8_t>> CodeGen::jumpType {
    {"jmp",{0xe9}},     // jmp
    {"==",{0x0f,0x84}}, // je
    {"!=",{0x0f,0x85}}, // jne
    {">",{0x0f,0x87}},  // ja
    {"<",{0x0f,0x82}},  // jb
    {">=",{0x0f,0x83}}, // jae
    {"<=",{0x0f,0x86}}, // jbe
};

std::unordered_map<std::string,std::string> CodeGen::oppositeJumpType {
    {"!=","=="}, 
    {"==","!="}, 
    {"<=",">"}, 
    {">=","<"}, 
    {"<",">="},  
    {">","<="},  
};