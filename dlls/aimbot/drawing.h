#include "dllmain.h"
// #include "util/D3DVTABLE_INDEX.h"
#include "vec3.h"
#include "halo.h"

#pragma once

namespace Drawing {

    // === Definitions ===

    // Found device location by backtracing a device method call.
    // Found device method using virtual table from dummy device.
    // https://gist.github.com/KodyJKing/d7b374b29998dfdd7d631430164f3e50
    IDirect3DDevice9** ppDevice = (IDirect3DDevice9**)0x0071D174;

    struct ColoredVertex {
        float x, y, z;
        D3DCOLOR diffuse;
    };

    // === Vars ===

    IDirect3DDevice9* pDevice;
    LPD3DXFONT pFont;
    D3DXMATRIX identity;
    
    // === Predeclarations ===

    void getProjectionMatrix(D3DXMATRIX *pMat);
    void getViewMatrix(D3DXMATRIX *pMat);
    Vec3 worldToScreen(Vec3 v);

    // === Boilerplate ===

    void init() {
        pDevice = *ppDevice;
        D3DXCreateFontA(
            pDevice, 14, 0, FW_NORMAL, 1, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            "Arial", &pFont );
        D3DXMatrixIdentity(&identity);
    }

    void release() {
        pFont->Release();
    }

    void** getDeviceVirtualTable() {
        IDirect3DDevice9* pDevice = *ppDevice;
        void** vTable = *(void***)(pDevice);
        // DWORD methodAddr = (DWORD) vTable[D3DVTABLE_INDEX::iSetTransform];
        // std::cout << "Method location: " << std::uppercase << std::hex << methodAddr << std::endl;
        return vTable;
    }

    void setDefaultRenderState() {
        // https://www.unknowncheats.me/forum/counterstrike-global-offensive/201018-d3d9-rendering.html
        pDevice->SetTexture( 0, nullptr );
        pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
        pDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
        pDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
        pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        pDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
        pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
        pDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
    }

    void enableDepthTest(bool enable) {
        pDevice->SetRenderState( D3DRS_ZENABLE, enable ? D3DZB_TRUE : D3DZB_FALSE );
    }

    void enableZWrite(bool enable) {
        pDevice->SetRenderState( D3DRS_ZWRITEENABLE, enable );
    }

    void clearDepthBuffer(float z) {
        pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, z, 0);
    }

    // === Drawing Functions ===

    struct Align {
        char horizontal: 8;
        char vertical: 8;
    };

    void realign(long dir, long* lbound, long* ubound) {
        long delta = (long)( (1 - dir) / 2.0f * (*ubound - *lbound) );
        *ubound -= delta;
        *lbound -= delta;
    }

    void _drawText( int x, int y, D3DCOLOR color, Align align, const char * text ) {
        RECT rect = { x, y, x, y };

        #define ARGS(fmt) NULL, text, -1, &rect, fmt, color
        pFont->DrawTextA( ARGS(DT_CALCRECT) );
        
        realign(align.horizontal, &rect.left, &rect.right);
        realign(align.vertical, &rect.top, &rect.bottom);

        D3DVIEWPORT9 viewport;
        pDevice->GetViewport(&viewport);
        if (rect.right < 0 || rect.left > (LONG)viewport.Width) 
            return;
        if (rect.bottom < 0 || rect.top > (LONG)viewport.Height)
            return;

        pFont->DrawTextA( ARGS(DT_LEFT) );
        #undef ARGS
    }

    bool drawTextBorders = true;
    void drawText( int x, int y, D3DCOLOR color, Align align, const char * text ) {
        if (drawTextBorders) {
            D3DCOLOR black = 0xFF000000;
            _drawText(x - 1, y + 0, black, align, text);
            _drawText(x + 1, y + 0, black, align, text);
            _drawText(x + 0, y - 1, black, align, text);
            _drawText(x + 0, y + 1, black, align, text);
        }
        _drawText(x, y, color, align, text);
    }

    void drawText3D( Vec3 pos, D3DCOLOR color, Align align, Vec3 offset, const char * text ) {
        Vec3 screenPos = worldToScreen(pos) + offset;
        Vec3 toPos = pos - pCamData->pos;
        float depth = pCamData->fwd.dot(toPos);
        if (depth > 0)
            drawText(
                (int)roundf(screenPos.x),
                (int)roundf(screenPos.y),
                color, align, text );
    }
    
    void drawLine3D(
        Vec3 a, Vec3 b,
        D3DCOLOR color
    ) {
        ColoredVertex vertices[] = {
            { a.x, a.y, a.z, color },
            { b.x, b.y, b.z, color },
        };
        pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 1, vertices, sizeof(ColoredVertex));
    }

    void drawLine3DTwoColor(
        Vec3 a, Vec3 b,
        D3DCOLOR colorA,
        D3DCOLOR colorB
    ) {
        ColoredVertex vertices[] = {
            { a.x, a.y, a.z, colorA },
            { b.x, b.y, b.z, colorB },
        };
        pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 1, vertices, sizeof(ColoredVertex));
    }

    void drawThickLine3D(
        Vec3 a, Vec3 b,
        float thickness,
        D3DCOLOR color
    ) {
        Vec3 mid = (a + b) / 2;
        Vec3 toEye = pCamData->pos - mid;
        Vec3 aToB = b - a;
        Vec3 tangent = aToB.cross(toEye).unit();

        float r = thickness * .5f;

        Vec3 v0 = a + tangent * -r;
        Vec3 v1 = b + tangent * -r;
        Vec3 v2 = a + tangent * r;
        Vec3 v3 = b + tangent * r;

        #define vert(v) { v.x, v.y, v.z, color }
        ColoredVertex vertices[] = { vert(v0), vert(v1), vert(v2), vert(v3) };
        #undef vert

        pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(ColoredVertex));
    }

    // === Transform functions ===

    // Note: Found clippingNear by manually tweaking until a vertical line no longer rendered under the floor.
    float clippingNear = 0.063f;
    float clippingFar = 200.0f;
    void getProjectionMatrix(D3DXMATRIX *pMat, float fov) {
        float aspect = (float)pCamData->viewportWidth / pCamData->viewportHeight;
        D3DXMatrixPerspectiveFovRH(pMat, fov, aspect, clippingNear, clippingFar);
    }

    void getViewMatrix(D3DXMATRIX *pMat) {
        Vec3 eye = pCamData->pos;
        Vec3 at = eye + pCamData->fwd;
        D3DXVECTOR3 d3dUp(0, 0, 1);
        D3DXMatrixLookAtRH(pMat, (D3DXVECTOR3*)&eye, (D3DXVECTOR3*)&at, &d3dUp);
    }

    void setup3DTransforms(float fov) {
        D3DXMATRIX projMat, viewMat;
        getProjectionMatrix(&projMat, fov);
        pDevice->SetTransform(D3DTS_PROJECTION, &projMat);
        getViewMatrix(&viewMat);
        pDevice->SetTransform(D3DTS_VIEW, &viewMat);
    }

    Vec3 worldToScreen(Vec3 v) {
        D3DXMATRIX projMat, viewMat;
        pDevice->GetTransform(D3DTS_PROJECTION, &projMat);
        pDevice->GetTransform(D3DTS_VIEW, &viewMat);

        D3DVIEWPORT9 viewport;
        pDevice->GetViewport(&viewport);

        D3DXVECTOR3 result;
        D3DXVec3Project(&result, (D3DXVECTOR3*)&v, &viewport, &projMat, &viewMat, &identity);

        Vec3* pResult = (Vec3*) &result;
        return *pResult;
    }

    Vec3 screenToWorld(Vec3 v) {
        D3DXMATRIX projMat, viewMat;
        pDevice->GetTransform(D3DTS_PROJECTION, &projMat);
        pDevice->GetTransform(D3DTS_VIEW, &viewMat);

        D3DVIEWPORT9 viewport;
        pDevice->GetViewport(&viewport);

        D3DXVECTOR3 result;
        D3DXVec3Unproject(&result, (D3DXVECTOR3*)&v, &viewport, &projMat, &viewMat, &identity);

        Vec3* pResult = (Vec3*) &result;
        return *pResult;
    }

}