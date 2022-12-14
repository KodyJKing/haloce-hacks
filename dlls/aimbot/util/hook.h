#pragma once

#include "../includes.h"

// VTable Hooks

struct VTableHook {

    const char* description;
    void** vtable;
    int methodIndex;
    void* oldMethod;
    void* newMethod;
    DWORD dontRehookBeforeTick;

    void unhook();
    void rehook(void** newVtable, DWORD cooldown);

};

VTableHook addVTableHook(const char * description, void ** vtable, int methodIndex, void * newMethod);
void removeAllVTableHooks();


// Jump Hooks

// === Opcodes =======
#define CALL   '\xE8'
#define JMP    '\xE9'
#define NOP    '\x90'
#define PUSHFD '\x9C'
#define POPFD  '\x9D'
#define PUSHAD '\x60'
#define POPAD  '\x61'
// ===================

// === ASM Helpers ========
#define GET_DWORD_REG(name, reg) DWORD name; __asm { mov [name], reg }
#define POPSTATE_AND_RETURN __asm popad __asm popfd __asm ret
// ========================

// === Jump hook flags ===========
#define HK_JUMP         0b00000001
#define HK_PUSH_STATE   0b00000010
#define HK_STOLEN_AFTER 0b00000100
// ===============================

struct JumpHook {

    const char* description;
    DWORD address;
    size_t numStolenBytes;

    char* trampolineBytes;
    DWORD trampolineReturn;
    size_t trampolineSize;
    char* stolenBytes;

    void allocTrampoline(size_t size);
    void protectTrampoline();

    void hook();
    void unhook();

    void writeStolenBytes(char** head);
    void writeReturnJump(char** head);
    void writePushState(char** head);
    void writePopState(char** head);
    void writeJump(char** head, DWORD hookFunc);
    void writeCall(char** head, DWORD hookFunc);

};

JumpHook addJumpHook(const char * description, DWORD address, size_t numStolenBytes, DWORD hookFunc, DWORD flags);

void removeAllJumpHookRecords();
