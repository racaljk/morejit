#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <iostream>

typedef int (*puts_ptr)(const char*);
typedef void (*myfunc_ptr)(puts_ptr);

#if 0
offset | bytes (in hex) | mnemonics
00     | 55             | push EBP
01     | 8BEC           | mov  EBP, ESP
03     | 68 ????????    | push ???????? ; address of string not known untilrun-time 
08     | FF55 08        | call dword ptr [EBP + 8] 
11     | 81C4 0400      | add  ESP, 4 
15     | 5D             | pop  EBP 
16     | C3             | ret
#endif
#if 0
   printf("%d%d%d", 5325, 235235, k);
003D50B1 8B 45 EC             mov         eax,dword ptr [k]  
003D50B4 50                   push        eax  
003D50B5 68 E3 96 03 00       push        396E3h  
003D50BA 68 CD 14 00 00       push        14CDh  
003D50BF 68 30 7B 3D 00       push        offset string "%d%d%d" (03D7B30h)  
003D50C4 E8 E3 C2 FF FF       call        _printf (03D13ACh)  
003D50C9 83 C4 10             add         esp,10h
#endif

/// All instructions are destination operand first. For example, give a
/// mov_reg32_reg32(ESP,EBP), it will generate mov esp<-ebp

using i8 = std::int8_t;
using u8 = std::uint32_t;
enum Register { EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI };

#define _modrm(MOD, REG, RM) \
    (u8)((RM & 0x7) | ((REG & 0x7) << 3) | (0b##MOD << 6))

#define _imm32(CODE, I)                   \
    *CODE++ = (i8)(I & 0XFF);             \
    *CODE++ = (i8)((I & 0xFF00) >> 8);    \
    *CODE++ = (i8)((I & 0xFF0000) >> 16); \
    *CODE++ = (i8)((I & 0xFF000000) >> 24);

// call [eax+dispatch8]
#define call_8(CODE, R, DISP)   \
    *CODE++ = 0xff;             \
    *CODE++ = _modrm(01, 2, R); \
    *CODE++ = DISP;

#define mov_reg32_reg32(CODE, R1, R2) \
    *CODE++ = 0x8b;                   \
    *CODE++ = _modrm(11, R1, R2);

#define mov_reg32_imm32(CODE, R, I) \
    *CODE++ = (0xb8 + R);           \
    _imm32(CODE, I);

#define add_reg32_imm32(CODE, R, I) \
    *CODE++ = 0x81;                 \
    *CODE++ = _modrm(11, 000, R);   \
    _imm32(CODE, I)

#define push_reg(CODE, R) *CODE++ = (0x50 + R);
#define pop_reg(CODE, R) *CODE++ = (0x58 + R);
#define push_imm32(I) \
    *CODE++ = 0x68;   \
    _imm32(CODE, I)

#define ret(CODE) *CODE++ = (0xC3)

void* allocateArean(size_t size) {
    return VirtualAlloc(NULL, 50, MEM_COMMIT | MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE);
}

void deallocateArean(void* addr) { VirtualFree(addr, 0, MEM_RELEASE); }

int main() {
    __asm {
         mov ebx,esp
    }
    char* p = (char*)malloc(100);
    int k = _modrm(11, 0, ESP);
    mov_reg32_imm32(p, EDX, 42);
    printf("%d%d%d", 5325, 235235, k);
    // Startup
    char* code = (char*)allocateArean(500);

    char* gen = code; /* address to generate code to */
    const char* pStr = "generating on the fly";

    /* function prologue */
    push_reg(gen, EBP);
    mov_reg32_reg32(gen, EBP, ESP);

    /* function body */
    *gen++ = 0x68;                 /* push                     */
    *((int*)gen) = (int)code + 19; /* (address of string)      */
    gen += 4;
    mov_reg32_reg32(gen, EBX, EBP);
    call_8(gen, EBX, 8);  // call [esp+8]
    add_reg32_imm32(gen, ESP, 0x4);
    pop_reg(gen, EBP);
    ret(gen);
    strcpy(gen, pStr);
    /* end of code generation */

    myfunc_ptr pMyfunc = (myfunc_ptr)code;
    pMyfunc(&puts);
    deallocateArean(code);
    return 0;
}
