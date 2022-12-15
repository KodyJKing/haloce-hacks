#pragma once

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

#define DllExport __declspec( dllexport )
#define Naked __declspec( naked )

DWORD __stdcall myThread(LPVOID lpParameter);
void setupHooks();
void cleanupHooks();

// === Options ===
namespace Options {
    DllExport bool aimbot          = false;
    DllExport bool esp             = true;
    DllExport bool timeHack        = true;
    DllExport bool freezeTime      = false;
    DllExport bool showLines       = true;
    DllExport bool showNumbers     = false;
    DllExport bool showFrames      = true;
    DllExport bool showHead        = false;
    DllExport bool smoothTargeting = true;
    DllExport bool showAxes        = true;
    DllExport bool showLookingAt   = true;
    DllExport bool triggerbot      = false;
    DllExport bool smartTargeting  = false;
    DllExport bool showLabels      = false;
    DllExport bool fullAuto        = false;
}
// extern std::unordered_map<std::string, bool> options;
// #define DECLARE_OPTION(name, value) options[#name] = value;
// #define OPTION(name) options[#name]
// ===============