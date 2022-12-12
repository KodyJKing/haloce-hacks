#include <iostream>

typedef unsigned int uint;
typedef unsigned short ushort;

#pragma once

static DWORD rehookCooldown = 500;

struct HookRecord {
    const char* description;
    void** vtable;
    int methodIndex;
    void* oldMethod;
    void* newMethod;
    DWORD dontRehookBeforeTick;
};

std::vector<HookRecord> hookRecords;

HookRecord addHook(const char* description, void** vtable, int methodIndex, void* newMethod);
void removeHook(HookRecord record);

HookRecord addHook(const char* description, void** vtable, int methodIndex, void* newMethod) {
    std::cout << "Adding hook: " << description << std::endl;
    HookRecord record{};
    record.description = description;
    record.vtable = vtable;
    record.methodIndex = methodIndex;
    record.newMethod = newMethod;
    record.oldMethod = vtable[methodIndex];
    vtable[methodIndex] = newMethod;
    hookRecords.push_back(record);
    return record;
}

void removeHook(HookRecord record) {
    std::cout << "Removing hook: " << record.description << std::endl;
    record.vtable[record.methodIndex] = record.oldMethod;
}

void removeAllHooks() {
    for(uint i = 0; i < hookRecords.size(); i++)
        removeHook(hookRecords[i]);
}

void rehook(HookRecord* pRecord, void** vtable) {
    DWORD now = GetTickCount();
    auto methodIndex = pRecord->methodIndex;
    auto newMethod = pRecord->newMethod;
    if (vtable[methodIndex] != newMethod) {
        if (pRecord->dontRehookBeforeTick > now)
            return;
        pRecord->dontRehookBeforeTick = now + rehookCooldown;
        std::cout << "Rehooking: " << pRecord->description << std::endl;
        pRecord->oldMethod = vtable[methodIndex];
        vtable[methodIndex] = newMethod;
    }
}

//===== Jump Hooks =====

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

struct JumpHookRecord {
    const char* description;
    DWORD address;
    size_t numStolenBytes;
    char* trampolineBytes;
    DWORD hookFunc;

    DWORD trampolineReturn;
};

std::vector<JumpHookRecord> jumpHookRecords;

JumpHookRecord addJumpHook(const char* description, DWORD address, size_t numStolenBytes, DWORD hookFunc, bool naked, DWORD* trampolineReturn) {
    const char CALL = '\xE8';
    const char JMP = '\xE9';
    const char NOP = '\x90';
    const char PUSHFD = '\x9C';
    const char POPFD = '\x9D';
    const char PUSHAD = '\x60';
    const char POPAD = '\x61';

    char* head; // write head

    std::cout << "Adding jump hook: " << description << std::endl;

    JumpHookRecord record = { description, address, numStolenBytes, nullptr, hookFunc };

    size_t trampolineSize = numStolenBytes + 14;

    record.trampolineBytes = (char*) VirtualAlloc(nullptr, trampolineSize, MEM_COMMIT, PAGE_READWRITE);

    // === Write trampoline code ===
    head = record.trampolineBytes;
    // Execute stolen bytes
    writeBytes(&head, (char*)address, numStolenBytes);

    // Call hook function with save/restore registers/flags
    write(&head, PUSHFD);
    write(&head, PUSHAD);
    write(&head, naked ? JMP : CALL);
    writeOffset(&head, hookFunc);
    if (trampolineReturn != nullptr)
        *trampolineReturn = (DWORD) head;
    write(&head, POPAD);
    write(&head, POPFD);

    // Jump back to hookee
    write(&head, JMP);
    writeOffset(&head, address + numStolenBytes);
    // =============================

    DWORD oldProtect;
    VirtualProtect(record.trampolineBytes, trampolineSize, PAGE_EXECUTE_READ, &oldProtect);

    // === Write jump to trampoline ===
    VirtualProtect((void*)address, numStolenBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

    // Make sure anything not overwritten by jump is a nop
    memset((void*)address, NOP, numStolenBytes);
    head = (char*)address;

    // Jump to trampoline
    write(&head, JMP);
    writeOffset(&head, (DWORD)record.trampolineBytes);

    VirtualProtect((void*)address, numStolenBytes, oldProtect, &oldProtect);
    // ================================

    std::cout << "Trampoline located at: " << std::hex << (DWORD)record.trampolineBytes << std::endl << std::endl;

    jumpHookRecords.push_back(record);
    return record;
}

JumpHookRecord addJumpCallHook(const char* description, DWORD address, size_t numStolenBytes, DWORD hookFunc) {
    return addJumpHook(description, address, numStolenBytes, hookFunc, false, nullptr);
}

void removeJumpHook(JumpHookRecord record) {
    std::cout << "Removing jump hook: " << record.description << std::endl;

    DWORD address = record.address;
    size_t numStolenBytes = record.numStolenBytes;
    char* trampolineBytes = record.trampolineBytes;

    // Write back stolen bytes
    DWORD oldProtect;
    VirtualProtect((void*)address, numStolenBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((void*)address, trampolineBytes, numStolenBytes);
    VirtualProtect((void*)address, numStolenBytes, oldProtect, &oldProtect);

    // Release trampoline
    VirtualFree(trampolineBytes, 0, MEM_RELEASE);
}

void removeAllJumpHookRecords() {
    for (uint i = 0; i < jumpHookRecords.size(); i++)
        removeJumpHook(jumpHookRecords[i]);
}

#define GET_DWORD_REG(name, reg) DWORD name; __asm { mov [name], reg }
#define MAKE_HOOKEE_RETURN __asm popad __asm popfd __asm ret