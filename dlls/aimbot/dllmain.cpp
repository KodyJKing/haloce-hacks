#include "dllmain.h"
#include "keypressed.h"
#include "hook.h"
#include "haloex.h"
#include "drawing.h"
#include "debugdraw.h"
#include "aimbot.h"

HMODULE myHModule;
HookRecord endSceneHookRecord;

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
    setupHooks();

    if (!err) {
        while (TRUE) {
            if (GetAsyncKeyState(VK_F9))
                break;
            rehook(&endSceneHookRecord, Drawing::getDeviceVirtualTable());
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

// === Hooks ======

// EndScene hook
typedef HRESULT(_stdcall* EndSceneFunc)(IDirect3DDevice9* pThisDevice);
HRESULT __stdcall endSceneHook( IDirect3DDevice9* pThisDevice ) {
    Aimbot::update();
    Aimbot::render();
    auto result = ((EndSceneFunc)endSceneHookRecord.oldMethod)(pThisDevice);
    return result;
 }

void setupHooks() {
    void** vTable = Drawing::getDeviceVirtualTable();
    endSceneHookRecord = addHook("EndScene", vTable, D3DVTABLE_INDEX::iEndScene, endSceneHook);
    // addJumpHook("CopyTransforms", 0x0051911dU, 6, (DWORD) copyTransforms);
}

void cleanupHooks() {
    removeAllHooks();
    removeAllJumpHookRecords();
}

