#pragma once

#include "drawing.h"

DllExport void __cdecl debugLine(Vec3 *a, Vec3* b , D3DCOLOR color, DWORD durationTicks);
DllExport void __cdecl debugRay(Vec3 *origin, Vec3* displacement, D3DCOLOR color, DWORD durationTicks);

namespace DebugDraw {
    
    void renderLines();
    
}
