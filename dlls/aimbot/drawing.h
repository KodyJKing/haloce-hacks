#pragma once

#include "includes.h"
#include "vec3.h"
#include "halo.h"

namespace Drawing {

    extern IDirect3DDevice9** ppDevice;
    extern IDirect3DDevice9* pDevice;
    extern bool drawTextBorders;

    struct ColoredVertex {
        float x, y, z;
        D3DCOLOR diffuse;
    };

    struct Align {
        char horizontal: 8;
        char vertical: 8;
    };

    void init();
    void release();
    void ** getDeviceVirtualTable();
    void setDefaultRenderState();
    void enableDepthTest(bool enable);
    void drawText(int x, int y, D3DCOLOR color, Align align, const char * text);
    void enableZWrite(bool enable);
    void clearDepthBuffer(float z);
    void drawText(int x, int y, D3DCOLOR color, Align align, const char * text);
    void drawText3D( Vec3 pos, D3DCOLOR color, Align align, Vec3 offset, const char * text );
    void drawLine3D( Vec3 a, Vec3 b, D3DCOLOR color );
    void drawLine3DTwoColor( Vec3 a, Vec3 b, D3DCOLOR colorA, D3DCOLOR colorB );
    void drawThickLine3D( Vec3 a, Vec3 b, float thickness, D3DCOLOR color);
    void getProjectionMatrix(D3DXMATRIX *pMat);
    void getViewMatrix(D3DXMATRIX *pMat);
    Vec3 worldToScreen(Vec3 v);
    Vec3 screenToWorld(Vec3 v);
    void setup3DTransforms(float fov);

}

