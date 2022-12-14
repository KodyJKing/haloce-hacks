#include "drawing.h"
#include "util/math.h"
// #include <algorithm>

#pragma once

typedef unsigned char uchar;

namespace {

    struct DebugLine {
        Vec3 a, b;
        D3DCOLOR color;
        DWORD durationTicks;
        DWORD expirationTick;
    };

    const int bufferSize = 4096;
    int writeHead = 0;
    DebugLine lineBuffer[bufferSize] = {};
    
    void addDebugLine(DebugLine line) {
        lineBuffer[writeHead++] = line;
        writeHead %= bufferSize;
    }

}

DllExport void __cdecl debugLine(Vec3 *a, Vec3* b , D3DCOLOR color, DWORD durationTicks) {
    addDebugLine({ 
        *a, *b, 
        color, 
        durationTicks,
        GetTickCount() + durationTicks
    });
}
DllExport void __cdecl debugRay(Vec3 *origin, Vec3* displacement, D3DCOLOR color, DWORD durationTicks) {
    addDebugLine({
        *origin, *origin + *displacement, 
        color, 
        durationTicks,
        GetTickCount() + durationTicks
    });
}

namespace DebugDraw {
    
    void renderLines() {
        Drawing::enableDepthTest(true);
        auto now = GetTickCount();
        for (int i = 0; i < bufferSize; i++) {
            auto l = lineBuffer[i];
            float t = (float)(l.expirationTick - now) / (float)l.durationTicks;
            float s = Math::smoothstep(.0f, .1f, t);
            D3DCOLOR finalColor = l.color;
            uchar* pAlpha = ((uchar*)(&finalColor)) + 3;
            *pAlpha = (uchar)(*pAlpha * s);
            if (l.expirationTick > now)
                Drawing::drawThickLine3D(l.a, l.b, 0.005f, finalColor);
                // Drawing::drawLine3D(l.a, l.b, finalColor);
        }
    }
    
}
