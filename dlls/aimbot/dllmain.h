#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <mutex>
#include <set>
#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#pragma once

#define DllExport   __declspec( dllexport )

DWORD __stdcall myThread(LPVOID lpParameter);
void setupHooks();
void cleanupHooks();


// === Options ===
namespace Options {
    bool aimbot          = false;
    bool esp             = false;
    bool freezeEntities  = false;
    bool showLines       = true;
    bool showNumbers     = false;
    bool showFrames      = true;
    bool showHead        = true;
    bool smoothTargeting = true;
    bool showAxes        = true;
    bool showLookingAt   = true;
    bool triggerbot      = false;
    bool smartTargeting  = false;
}
// ===============