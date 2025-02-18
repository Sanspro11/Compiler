#include "codeGen.hpp"
#include <vector>
#include <unordered_map>

std::vector<uint8_t> codeGen::exitSyscall(uint8_t num) {
    std::vector<uint8_t> code = {
        0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00, // MOV RAX, 0x3C (EXIT SYSCALL)
        0x48, 0xC7, 0xC7, num, 0x00, 0x00, 0x00, // MOV RDI, NUM (0-255)
        0x0F, 0x05 // SYSCALL
    };
    return code;
}

std::vector<uint8_t> codeGen::call() {
    return {0xE8, 0x00, 0x00, 0x00, 0x00};
    // the address of the call is being relocated by .rela.text
} // call 0x00000000

std::vector<uint8_t> codeGen::movabs(const std::string& reg, uint64_t num) {
    auto movCode = register64BitMov[reg];
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&num);
    for (size_t i = 0; i < 8; ++i) {
        movCode.push_back(*(bytePtr + i));
    }
    return movCode;
} // movabs reg, num

std::vector<uint8_t> codeGen::leaStub(const std::string& reg) { 
    auto leaCode = register64BitLeaStub[reg];
    addCode(leaCode,{0x00,0x00,0x00,0x00});
    return leaCode;
} // lea reg, 0x00000000

std::vector<uint8_t> codeGen::pushReg(const std::string& reg) { 
    return pushRegCode[reg];
} // push reg

std::vector<uint8_t> codeGen::popReg(const std::string& reg) { 
    return popRegCode[reg];
} // pop reg

std::vector<uint8_t> codeGen::leave() { 
    return {0xC9};
} // leave, which is: 
// mov rsp, rbp
// pop rbp

std::vector<uint8_t> codeGen::leaveFunction() {
    return {0xC9,0xC3};
} // leave then ret

std::vector<uint8_t> codeGen::startFunction() {
    return {0x55,0x48,0x89,0xE5};
} // push rbp & mov rbp,rsp 

std::vector<uint8_t> codeGen::ret() { 
    return {0xC3};
} // ret

std::vector<uint8_t> codeGen::movRspRbp() { 
    return {0x48,0x89,0xE5};
} // mov rbp, rsp

std::vector<uint8_t> codeGen::movRaxQwordRbpOffset(uint32_t offset) { 
    std::vector<uint8_t> code = {0x48,0x8b,0x85};
    offset = ~offset + 1;
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&offset);
    for (size_t i = 0; i < 4; ++i) {
        code.push_back(*(bytePtr + i));
    }
    return code;
} // mov rax, [rbp-0xOFFSET]


std::vector<uint8_t> codeGen::movRbpQwordOffsetRax(uint32_t offset) { 
    std::vector<uint8_t> code = {0x48,0x89,0x85};
    offset = ~offset + 1;
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&offset);
    for (size_t i = 0; i < 4; ++i) {
        code.push_back(*(bytePtr + i));
    }
    return code;
} // mov [rbp-0xOFFSET], rax

std::vector<uint8_t> codeGen::leaRaxQwordRbpOffset(uint32_t offset) { 
    std::vector<uint8_t> code = {0x48,0x8d,0x85};
    offset = ~offset + 1;
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&offset);
    for (size_t i = 0; i < 4; ++i) {
        code.push_back(*(bytePtr + i));
    }
    return code;
} // lea rax, [rbp-0xOFFSET]

std::vector<uint8_t> codeGen::subRsp(uint32_t num) { 
    std::vector<uint8_t> code = {0x48,0x81,0xEC};
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&num);
    for (size_t i = 0; i < 4; ++i) {
        code.push_back(*(bytePtr + i));
    }
    return code;
} // sub rsp, num

std::vector<uint8_t> codeGen::movRegRax(const std::string& reg) { 
    return movRegRaxMap[reg];
} // mov reg, rax

std::vector<uint8_t> codeGen::movRaxReg(const std::string& reg) { 
    return movRaxRegMap[reg];
} // mov rax, reg

std::vector<uint8_t> codeGen::addRaxRbx() { 
    return {0x48,0x01,0xd8};
} // add rax, rbx

std::vector<uint8_t> codeGen::subRaxRbx() { 
    return {0x48,0x29,0xd8};
} // sub rax, rbx

std::vector<uint8_t> codeGen::mulRbx() {
    return {0x48,0xf7,0xe3};
} // mul rbx (result in rdx:rax)

std::vector<uint8_t> codeGen::divRbx() {
    return {0x48,0xf7,0xf3};
} // div rbx (RAX quotient, RDX remainder)

std::vector<uint8_t> codeGen::cmpRaxRbx() { 
    return {0x48,0x39,0xd8};
} // cmp rax, rbx

std::vector<uint8_t> codeGen::oppositeJump(const std::string& type) { 
    std::string oppositeOp = oppositeJumpType[type];
    std::vector<uint8_t> jmp = jumpType[oppositeOp];
    addCode(jmp,{0x00,0x00,0x00,0x00});
    return jmp;
} // je/jne/ja/jb/jae/jbe 0x00000000

std::vector<uint8_t> codeGen::jump(const std::string& type) { 
    std::vector<uint8_t> jmp = jumpType[type];
    addCode(jmp,{0x00,0x00,0x00,0x00});
    return jmp;
} // je/jne/ja/jb/jae/jbe 0x00000000


std::unordered_map<uint8_t,std::string> codeGen::positionToRegister = {
    {0,"rdi"},
    {1,"rsi"},
    {2,"rdx"},
    {3,"rcx"},
    {4,"r8"},
    {5,"r9"}
};
std::unordered_map<std::string,std::vector<uint8_t>> codeGen::register64BitMov {
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

std::unordered_map<std::string,std::vector<uint8_t>> codeGen::register64BitLeaStub {
    {"rdi",{0x48,0x8d,0x3c,0x25}},
    {"rsi",{0x48,0x8d,0x34,0x25}},
    {"rdx",{0x48,0x8d,0x14,0x25}},
    {"rcx",{0x48,0x8d,0x0c,0x25}},
    {"r9",{0x4c,0x8d,0x0c,0x25}},
    {"r8",{0x4c,0x8d,0x04,0x25}}
};

std::unordered_map<std::string,std::vector<uint8_t>> codeGen::movRegRaxMap {
    {"rbx",{0x48,0x89,0xc3}},
    {"rdi",{0x48,0x89,0xc7}},
    {"rsi",{0x48,0x89,0xc6}},
    {"rdx",{0x48,0x89,0xc2}},
    {"rcx",{0x48,0x89,0xc1}},
    {"r9",{0x49,0x89,0xc1}},
    {"r8",{0x49,0x89,0xc0}}
};

std::unordered_map<std::string,std::vector<uint8_t>> codeGen::movRaxRegMap {
    {"rdx",{0x48,0x89,0xd0}},
};

std::unordered_map<std::string,uint8_t> codeGen::typeSizes {
    {"int",4},
    {"uint8_t",1},
    {"uint16_t",2},
    {"uint32_t",4},
    {"uint64_t",8}
};
std::unordered_map<std::string,std::vector<uint8_t>> codeGen::pushRegCode {
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

std::unordered_map<std::string,std::vector<uint8_t>> codeGen::popRegCode {
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

std::unordered_map<std::string,std::vector<uint8_t>> codeGen::jumpType {
    {"jmp",{0xe9}},     // jmp
    {"==",{0x0f,0x84}}, // je
    {"!=",{0x0f,0x85}}, // jne
    {">",{0x0f,0x87}},  // ja
    {"<",{0x0f,0x82}},  // jb
    {">=",{0x0f,0x83}}, // jae
    {"<=",{0x0f,0x86}}, // jbe
};

std::unordered_map<std::string,std::string> codeGen::oppositeJumpType {
    // opposite jump type for easier code logic
    {"!=","=="}, 
    {"==","!="}, 
    {"<=",">"}, 
    {">=","<"}, 
    {"<",">="},  
    {">","<="},  
};