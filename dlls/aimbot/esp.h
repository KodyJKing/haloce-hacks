#pragma once

#include "dllmain.h"
#include "halo.h"
#include "haloex.h"
#include "drawing.h"
#include "debugdraw.h"
#include "keypressed.h"
#include "input.h"

namespace ESP {

    void renderBoneNumber(Bone* pBone, int i, DWORD color) {
        auto text = std::to_string(i);
        Drawing::drawText3D(pBone->pos, color, {}, {0,10}, text.c_str());
    }

    void renderBoneFrame(Bone* pBone) {
            const float scale = Skeleton::SMALL_UNIT;
            
            Vec3 pos, x, y, z;
            pos = pBone->pos;
            x = pBone->x * scale;
            y = pBone->y * scale;
            z = pBone->z * scale;

            Drawing::drawLine3D(pos, pos + x, 0x80FF0000);
            Drawing::drawLine3D(pos, pos + y, 0x8000FF00);
            Drawing::drawLine3D(pos, pos + z, 0x800000FF);
    }

    void renderBones(EntityRecord record, bool highlight, int highlightIndex) {
        if (record.typeId == TypeID_Player)
            return;
        
        EntityTraits traits = getEntityTraits(record);
        Entity* pEntity = record.pEntity;
        
        uint boneCount = getBoneCount(pEntity);
        for (uint i = 0; i < boneCount; i++) {
            Bone* pBone = getBonePointer(pEntity, i);
            bool highLightBone = highlight && i == highlightIndex;
            if (Options::showNumbers)
                renderBoneNumber(pBone, i, highLightBone ? 0xFFFFFF00 : 0xFFFFFFFF);
            if (Options::showFrames)
                renderBoneFrame(pBone);
        }
    }

    void renderSkeleton(EntityRecord record, bool highlight, int highlightIndex) {
        int* skeleton = Skeleton::getEntitySkeleton(record);
        if (!skeleton)
            return;

        Entity* pEntity = record.pEntity;
        
        for (int i = 1; i < 64; i++) {

            int index = skeleton[i];
            if (index == Skeleton::END) {
                break;
            } else if (index == Skeleton::UP) {
                i++; // Pick the "pen" up.
            } else {
                int previousI = i - 1;
                if (previousI < 0) 
                    continue; // Don't put "pen" back down until we have a pair of valid bone indices.
                    
                int index2 = skeleton[previousI];
                Vec3 pos = getBonePointer(pEntity, index)->pos;
                Vec3 pos2 = getBonePointer(pEntity, index2)->pos;

                D3DCOLOR defaultColor = 0x40888888;
                D3DCOLOR highlightColor = 0xFFFFFF00;
                D3DCOLOR color = highlight && index == highlightIndex ? highlightColor : defaultColor;
                D3DCOLOR color2 = highlight && index2 == highlightIndex ? highlightColor : defaultColor;

                Drawing::drawLine3DTwoColor(pos, pos2, color, color2);
            }

        }
    }

    void renderHead(EntityRecord record) {
        EntityTraits traits = getEntityTraits(record);
        if (!traits.living)
            return;
        Vec3 pos = Skeleton::getHeadPos(record);
        float w = 0.005f;
        Vec3 up = {0, 0, w};
        Drawing::drawThickLine3D(pos, pos + up, w, 0xFFFF0000);
    }

    void renderAxes() {
        Drawing::setup3DTransforms(1);
        Drawing::enableDepthTest(true);
        Drawing::enableZWrite(true);
        Drawing::clearDepthBuffer(1.0f);

        D3DVIEWPORT9 oldVP, newVP;
        Drawing::pDevice->GetViewport(&oldVP);
        
        DWORD w = oldVP.Width;
        DWORD h = oldVP.Height;

        Vec3 screenOrigin = { w / 2.0f, h / 2.0f, 0.5f };
        Vec3 pos = Drawing::screenToWorld( screenOrigin );

        DWORD size = 100;
        newVP = { w - size, h - size, size, size, 0.0f, 1.0f };
        Drawing::pDevice->SetViewport(&newVP);

        float scale = 0.05f;
        float thickness = scale * 0.1f;
        Drawing::drawThickLine3D(pos, pos + Vec3{1,0,0} * scale, thickness, 0x80FF0000);
        Drawing::drawThickLine3D(pos, pos + Vec3{0,1,0} * scale, thickness, 0x8000FF00);
        Drawing::drawThickLine3D(pos, pos + Vec3{0,0,1} * scale, thickness, 0x800000FF);

        Drawing::pDevice->SetViewport(&oldVP);
    }

    void renderLookingAtCaption(RaycastResult* pResult) {
        if ( pResult->hitType != HitType_Entity )
            return;
        EntityRecord record = getRecord(pResult->entityHandle);

        bool print = keypressed('P');

        char buf[100];
        int lineNum = 0;

        #define LINE(format, ...) \
            sprintf_s(buf, format, __VA_ARGS__); \
            Drawing::drawText(pCamData->viewportWidth / 2, (lineNum++) * 16, 0xFFFFFFFF, {0, 1}, buf); \
            if (print) std::cout << buf << std::endl;

        if (print) std::cout << std::endl;
        LINE("Handle %X", pResult->entityHandle);
        LINE("Address %X", (DWORD) record.pEntity);
        LINE("Type %X", record.typeId);
        LINE("Bone index %u", pResult->boneIndex);

        #undef LINE
        
    }

    void renderEntityInfo(EntityRecord* pRecord) {
        Entity* pEntity = pRecord->pEntity;
        Vec3 pos = pEntity->pos;
        Vec3 camPos = pCamData->pos;

        float distance = (pos - camPos).length();
        if (
            pEntity->entityCategory == EntityCategory_SoundScenery ||
            pEntity->entityCategory == EntityCategory_Scenery ||
            distance > 25.0f
        )
            return;

        RaycastResult result;
        Vec3 displacement = pos - camPos;
        raycast(
            traceProjectileRaycastFlags,
            &camPos, &displacement,
            pPlayerData->entityHandle,
            &result
        );

        float distanceToHit = (result.hit - camPos).length();
        if (distanceToHit + 1.0f < distance)
            return;
        
        char buf[100];
        int lineNum = 0;

        #define LINE(format, ...) \
            sprintf_s( buf, format, __VA_ARGS__ ); \
            Drawing::drawText3D( pos, 0xFFFFFFFF, {0, 1}, {0, (lineNum++) * 16.0f}, buf );
        
        LINE("%04X %04X", pRecord->typeId, pRecord->unknown_2);
        LINE("%04X %04X", pRecord->id,  pRecord->unknown_1);
        LINE("@%08X", (DWORD) pEntity);
        LINE("#%08X", pEntity->tagHandle);

        #undef LINE

    }

    void render() {
        if (!Options::esp)
            return;
        
        Drawing::setup3DTransforms(pCamData->fov);
        Drawing::setDefaultRenderState();

        DebugDraw::renderLines();

        RaycastResult rcResult;
        raycastPlayerCrosshair(&rcResult, traceProjectileRaycastFlags);

        EntityRecord entityUnderCrosshair = {};
        if ( rcResult.hitType == HitType_Entity )
            entityUnderCrosshair = getRecord(rcResult.entityHandle);

        Drawing::enableDepthTest(false);
        
        Entities entities;
        getValidEntityRecords(&entities);
        for (auto record : entities) {
            auto pEntity = record.pEntity;

            auto cat = pEntity->entityCategory;
            if (
                cat != EntityCategory_Biped &&
                cat != EntityCategory_Equipment &&
                cat != EntityCategory_Projectile &&
                cat != EntityCategory_Vehicle &&
                cat != EntityCategory_Weapon
            )
                continue;

            if (Options::showLabels)
                renderEntityInfo(&record);
            
            auto traits = getEntityTraits(record);
            bool alive = pEntity->health > 0.0f;
            bool dead = traits.living && !alive;

            if (dead)
                continue;

            if (pEntity->parentEntityHandle == pPlayerData->entityHandle)
                continue;
                
            bool highlight = entityUnderCrosshair.pEntity == pEntity;

            if (Options::showLines)
                renderSkeleton(record, highlight, rcResult.boneIndex);
            if (Options::showNumbers || Options::showFrames)
                renderBones(record, highlight, rcResult.boneIndex);
            if (Options::showHead)
                renderHead(record);
        }

        if (Options::showAxes)
            renderAxes();

        if (Options::showLookingAt)
            renderLookingAtCaption(&rcResult);
    }

}
