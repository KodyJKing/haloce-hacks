#include "dllmain.h"
#include "keypressed.h"
#include "hook.h"
#include "haloex.h"
#include "drawing.h"
#include "debugdraw.h"
#include "aimbot.h"
#include "esp.h"
#include "timehack.h"

HMODULE myHModule;
VTableHook endSceneHookRecord;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            myHModule = hModule;
            CreateThread(0, 0, myThread, 0, 0, 0);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

DWORD __stdcall myThread(LPVOID lpParameter) {

    const bool useConsole = true;
    const bool useStdin = false;
    const bool pressKeyToExit = false;

    errno_t err = 0;
    FILE *pFile_stdout, *pFile_stderr, *pFile_stdin;
    if (useConsole) {
        AllocConsole();
        err = freopen_s(&pFile_stdout, "CONOUT$", "w", stdout);
        err |= freopen_s(&pFile_stderr, "CONOUT$", "w", stderr);
        if (useStdin)
            err |= freopen_s(&pFile_stdin, "CONIN$", "r", stdin);
    }

    Drawing::init();
    TimeHack::init();
    setupHooks();

    if (!err) {
        while (TRUE) {
            if (GetAsyncKeyState(VK_F9))
                break;
            endSceneHookRecord.rehook(Drawing::getDeviceVirtualTable(), 500);
            // rehookVTable(&endSceneHookRecord, Drawing::getDeviceVirtualTable());
            Sleep(16);
        }
    }

    cleanupHooks();
    Sleep(500); // Give our hooks a while to exit.
                // A cleaner solution using a mutex might be possible.
    Drawing::release();

    if (useConsole) {
        if (useStdin && pressKeyToExit) {
            std::cout << std::endl << "Press anything to continue" << std::endl;
            getchar();
        }

        if (pFile_stdout) fclose(pFile_stdout);
        if (pFile_stderr) fclose(pFile_stderr);
        if (useStdin && pFile_stdin) fclose(pFile_stdin);
        FreeConsole();
    }

    FreeLibraryAndExitThread(myHModule, 0);
}

void checkOptionToggles() {
    
    #define BIND(vk, name) \
        if ( keypressed( vk ) ) { \
            Options::name = !Options::name; \
            printf("%s %s\n", #name, Options::name ? "enabled" : "disabled"); \
        }
        
    BIND(VK_F1, aimbot)
    BIND(VK_F2, esp)
    BIND(VK_F3, timeHack)
    BIND(VK_F4, freezeTime)
    BIND(VK_NUMPAD1, showLines)
    BIND(VK_NUMPAD2, showNumbers)
    BIND(VK_NUMPAD3, showFrames)
    BIND(VK_NUMPAD4, showLabels)
    BIND(VK_NUMPAD5, smoothTargeting)
    BIND(VK_NUMPAD6, showAxes)
    BIND(VK_NUMPAD7, showLookingAt)
    BIND(VK_NUMPAD8, triggerbot)
    BIND(VK_NUMPAD9, smartTargeting)
    
    #undef BIND

}

void teleportToCrosshair() {
    RaycastResult rcResult = {};
    raycastPlayerCrosshair(&rcResult, traceProjectileRaycastFlags);
    if (rcResult.hitType != HitType_Nothing) {
        Entity* pPlayer = getPlayerPointer();
        pPlayer->pos = rcResult.hit - pCamData->fwd * 1.0f;
        pPlayer->velocity = pPlayer->velocity + pCamData->fwd * 0.1f;
    }
}

void onSceneEnd() {

    TimeHack::doSingleStep = false;

    checkOptionToggles();

    ESP::render();
    Aimbot::update();

    // if( keypressed('C') )
    //     teleportToCrosshair();

    if ( keypressed(VK_F5) )
        TimeHack::doSingleStep = true;
    
}

// === Hooks ======

// EndScene hook
typedef HRESULT(_stdcall* EndSceneFunc)(IDirect3DDevice9* pThisDevice);
HRESULT __stdcall endSceneHook( IDirect3DDevice9* pThisDevice ) {
    onSceneEnd();
    auto result = ((EndSceneFunc)endSceneHookRecord.oldMethod)(pThisDevice);
    return result;
}

namespace FullAutoHook {
    DWORD hookAddress = 0x004C4914;
    DWORD numStolenBytes = 8;
    DWORD hookReturn = hookAddress + numStolenBytes;
    DWORD tranpolineReturn, entityHandle;

    bool shouldGetFullAuto(DWORD entityHandle) {
        return getRecord(entityHandle).typeId == TypeID_Player;
    }

    Naked void hookFunc() {
        __asm {
            pushad
            pushfd
            mov eax, [esp+0x38+PUSHSTATE_BYTES]
            mov [entityHandle], eax
        }
        if ( Options::fullAuto && shouldGetFullAuto(entityHandle) ) {
            __asm {
                popfd
                popad
                jmp [hookReturn]
            }
        }
        __asm {
            popfd
            popad
            jmp [tranpolineReturn];
        }
    }

    void setup() {
        addJumpHook(
            "FullAuto",
            FullAutoHook::hookAddress,
            FullAutoHook::numStolenBytes,
            (DWORD) FullAutoHook::hookFunc,
            HK_JUMP | HK_STOLEN_AFTER,
            &FullAutoHook::tranpolineReturn
        );
    }
}

void setupHooks() {
    void** vTable = Drawing::getDeviceVirtualTable();
    endSceneHookRecord = addVTableHook("EndScene", vTable, D3DVTABLE_INDEX::iEndScene, endSceneHook);
    FullAutoHook::setup();
}

void cleanupHooks() {
    removeAllVTableHooks();
    removeAllJumpHookRecords();
}

