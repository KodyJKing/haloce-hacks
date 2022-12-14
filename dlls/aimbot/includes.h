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

#include "options.h"

#define DllExport __declspec( dllexport )
#define Naked __declspec( naked )