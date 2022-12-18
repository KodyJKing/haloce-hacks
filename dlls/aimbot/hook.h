#include <iostream>

typedef unsigned int uint;
typedef unsigned short ushort;

#pragma once

//#region VTable Hooks

struct VTableHook {
    const char* description;
    void** vtable;
    int methodIndex;
    void* oldMethod;
    void* newMethod;
    DWORD dontRehookBeforeTick;

    void unhook() {
        std::cout << "Removing hook: " << description << std::endl;
        vtable[methodIndex] = oldMethod;
    }

    void rehook(void** newVtable, DWORD cooldown) {
        DWORD now = GetTickCount();
        if (newVtable[methodIndex] == newMethod || dontRehookBeforeTick > now)
            return;
        vtable = newVtable;
        dontRehookBeforeTick = now + cooldown;
        std::cout << "Rehooking: " << description << std::endl;
        oldMethod = vtable[methodIndex];
        vtable[methodIndex] = newMethod;
    }
};

std::vector<VTableHook> hookRecords;

VTableHook addVTableHook(const char* description, void** vtable, int methodIndex, void* newMethod);
void removeVTableHook(VTableHook record);

VTableHook addVTableHook(const char* description, void** vtable, int methodIndex, void* newMethod) {
    std::cout << "Adding hook: " << description << std::endl;
    VTableHook record{};
    record.description = description;
    record.vtable = vtable;
    record.methodIndex = methodIndex;
    record.newMethod = newMethod;
    record.oldMethod = vtable[methodIndex];
    vtable[methodIndex] = newMethod;
    hookRecords.push_back(record);
    return record;
}

void removeAllVTableHooks() {
    for(uint i = 0; i < hookRecords.size(); i++)
        hookRecords[i].unhook();
}

//#endregion VTable Hooks

//#region Jump Jooks

#define GET_DWORD_REG(name, reg) DWORD name; __asm { mov [name], reg }
#define GET_DWORD_REG_GLOBAL(name, reg) __asm { mov [name], reg }
#define POPSTATE_AND_RETURN __asm popad __asm popfd __asm ret
#define PUSHSTATE_BYTES 0x24
#define RETURN_ADDRESS_BYTES 0x04

// === Opcodes =======
#define CALL   '\xE8'
#define JMP    '\xE9'
#define NOP    '\x90'
#define PUSHFD '\x9C'
#define POPFD  '\x9D'
#define PUSHAD '\x60'
#define POPAD  '\x61'
// ===================


// === Buffer Writing ===
inline void writeBytes(char** pDest, char* src, size_t count) {
    memcpy(*pDest, src, count);
    *pDest += count;
}

template<typename T>
inline void write(char** pDest, T content) {
    writeBytes(pDest, (char*) &content, sizeof(T));
}

inline void writeOffset(char** pDest, DWORD address) {
    int offset = (int)address - (int)(*pDest) - sizeof(DWORD);
    write(pDest, offset);
}
// ======================

struct JumpHook {
    const char* description;
    DWORD address;
    size_t numStolenBytes; // Rather, the number of bytes overwritten. Not necessarily executed.

    char* trampolineBytes;
    DWORD trampolineReturn;
    size_t trampolineSize;
    char* stolenBytes;

    void allocTrampoline(size_t size) {
        trampolineSize = size;
        trampolineBytes = (char*) VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);
    }

    void protectTrampoline() {
        DWORD oldProtect;
        VirtualProtect(trampolineBytes, trampolineSize, PAGE_EXECUTE_READ, &oldProtect);
    }

    void hook() {
        std::cout << "Adding jump hook: " << description << std::endl;

        stolenBytes = (char*) malloc(numStolenBytes);
        memcpy(stolenBytes, (char*)address, numStolenBytes);

        DWORD oldProtect;
        VirtualProtect((void*)address, numStolenBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

        // Make sure anything not overwritten by jump is a nop
        memset((void*)address, NOP, numStolenBytes);
        char* head = (char*)address;

        // Jump to trampoline
        write(&head, JMP);
        writeOffset(&head, (DWORD) trampolineBytes);

        VirtualProtect((void*)address, numStolenBytes, oldProtect, &oldProtect);
    }

    void unhook() {
        std::cout << "Removing jump hook: " << description << std::endl;

        // Write back stolen bytes
        DWORD oldProtect;
        VirtualProtect((void*)address, numStolenBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((void*)address, stolenBytes, numStolenBytes);
        VirtualProtect((void*)address, numStolenBytes, oldProtect, &oldProtect);

        // Release trampoline
        VirtualFree(trampolineBytes, 0, MEM_RELEASE);
        free(stolenBytes);
    }

    // Trampoline code generation

    void writeStolenBytes(char** head) {
        writeBytes(head, (char*)address, numStolenBytes);
    }

    void writeReturnJump(char** head) {
        write(head, JMP);
        writeOffset(head, address + numStolenBytes);
    }

    void writePushState(char** head) {
        write(head, PUSHFD);
        write(head, PUSHAD);
    }

    void writePopState(char** head) {
        write(head, POPAD);
        write(head, POPFD);
    }

    void writeJump(char** head, DWORD hookFunc) {
        write(head, JMP);
        writeOffset(head, hookFunc);
        trampolineReturn = (DWORD) *head;
    }

    void writeCall(char** head, DWORD hookFunc) {
        write(head, CALL);
        writeOffset(head, hookFunc);
    }

};

std::vector<JumpHook> jumpHookRecords;

// === Jump hook flags ==========
#define HK_JUMP         0b00000001
#define HK_PUSH_STATE   0b00000010
#define HK_STOLEN_AFTER 0b00000100
// ===============================

JumpHook addJumpHook(
    const char* description, DWORD address, size_t numStolenBytes, DWORD hookFunc, DWORD flags, DWORD* returnAddress
) {
    JumpHook record = { description, address, numStolenBytes };

    record.allocTrampoline(numStolenBytes + 14);

    // === Write trampoline code ===
    char* head = record.trampolineBytes;

    bool execStolenAfter = flags & HK_STOLEN_AFTER;

    if (!execStolenAfter)
        record.writeStolenBytes(&head);

    if (flags & HK_PUSH_STATE)
        record.writePushState(&head);
    
    if (flags & HK_JUMP)
        record.writeJump(&head, hookFunc);
    else
        record.writeCall(&head, hookFunc);

    if (flags & HK_PUSH_STATE)
        record.writePopState(&head);

    if (execStolenAfter)
        record.writeStolenBytes(&head);

    record.writeReturnJump(&head);

    if (returnAddress != nullptr)
        *returnAddress = record.trampolineReturn;
    // =============================

    record.protectTrampoline();
    record.hook();

    std::cout << "Trampoline located at: " << std::hex << (DWORD)record.trampolineBytes << std::endl << std::endl;

    jumpHookRecords.push_back(record);
    return record;
}

JumpHook addJumpHook(
    const char* description, DWORD address, size_t numStolenBytes, DWORD hookFunc, DWORD flags
) {
    return addJumpHook(description, address, numStolenBytes, hookFunc, flags, nullptr);
}

void removeAllJumpHookRecords() {
    for (uint i = 0; i < jumpHookRecords.size(); i++)
        jumpHookRecords[i].unhook();
}

//#endregion Jump Jooks