#pragma once

#include "includes.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

DWORD __stdcall myThread(LPVOID lpParameter);
void setupHooks();
void cleanupHooks();
