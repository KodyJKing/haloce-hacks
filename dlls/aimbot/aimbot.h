#pragma once

#include "dllmain.h"
#include "halo.h"
#include "haloex.h"
#include "drawing.h"
#include "debugdraw.h"
#include "keypressed.h"

namespace Aimbot {

    // === Toggles ===
    bool showLines       = true;
    bool showNumbers     = false;
    bool showFrames      = false;
    bool showHead        = true;
    bool smoothTargeting = true;
    bool showAxes        = false;
    // ===============

    EntityRecord targetEntity;

    // #region Aim

    float getTargetScore(EntityRecord record) {
        Vec3 toEntity = Skeleton::getHeadPos(record) - pCamData->pos;
        Vec3 unit = toEntity.unit();
        return unit.dot(pCamData->fwd);
    }

    EntityRecord bestTarget() {

        EntityRecord best = {};
        float bestScore = -1e+10;

        EntityList* entityList = getpEntityList();

        Entities entities;
        getValidEntityRecords(&entities);
        for (auto record : entities) {

            EntityTraits traits = getEntityTraits(record);
            if (!traits.hostile || !traits.living || record.pEntity->health <= 0.0f)
                continue;
            
            float score = getTargetScore(record);

            if (score > bestScore) {
                bestScore = score;
                best = record;
            }

        }

        return best;
    }

    void lookAt(Vec3 pos) {
        Vec3 dir = pos - pCamData->pos;
        if (smoothTargeting)
            dir = pCamData->fwd.lerp(dir.unit(), 0.5f);
        Angles angles = dir.toAngles();
        pPlayerData->yaw = angles.yaw;
        pPlayerData->pitch = angles.pitch;
    }

    // #endregion

    void update() {

        if (keypressed(VK_NUMPAD1)) showLines       = !showLines;
        if (keypressed(VK_NUMPAD2)) showNumbers     = !showNumbers;
        if (keypressed(VK_NUMPAD3)) showFrames      = !showFrames;
        if (keypressed(VK_NUMPAD4)) showHead        = !showHead;
        if (keypressed(VK_NUMPAD5)) smoothTargeting = !smoothTargeting;
        if (keypressed(VK_NUMPAD6)) showAxes        = !showAxes;

        if (!GetAsyncKeyState(VK_SHIFT)) {
            targetEntity = {};
        } else {
            targetEntity = bestTarget();
            if (targetEntity.pEntity)
                lookAt( Skeleton::getHeadPos(targetEntity) );
        }

        if( keypressed('C') ) {
            
            RaycastResult result{};
            DWORD flags = 0x001000E9; // Raycast flags are from traceProjectile function. See 004C04B6.
            Vec3 origin = pCamData->pos;
            Vec3 displacement = pCamData->fwd * 100.0f;
            int status = raycast( flags, 
                &origin, &displacement,
                pPlayerData->entityHandle,
                &result );
            
            debugLine(&origin, &result.hit, 0xFFF98000, 5*1000);

            if (result.hitType != HitType_Nothing) {
                Entity* pPlayer = getPlayerPointer();
                pPlayer->pos = result.hit - pCamData->fwd * 1.0f;
                pPlayer->velocity = pPlayer->velocity + pCamData->fwd * 0.1f;
            }

        }

    }

    // #region Rendering

    void renderBoneNumber(Bone* pBone, int i) {
        auto text = std::to_string(i);
        Drawing::drawText3D(pBone->pos, 0x80FFFFFF, {}, {0,10}, text.c_str());
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

    void renderBones(EntityRecord record) {
        if (record.typeId == TypeID_Player)
            return;
        
        EntityTraits traits = getEntityTraits(record);
        Entity* pEntity = record.pEntity;
        
        for (uint i = 0; i < traits.numBones; i++) {
            Bone* pBone = getBonePointer(pEntity, i);
            if (showNumbers)
                renderBoneNumber(pBone, i);
            if (showFrames)
                renderBoneFrame(pBone);
        }
    }

    void renderSkeleton(EntityRecord record) {
        int* skeleton = Skeleton::getEntitySkeleton(record);
        if (!skeleton)
            return;

        Entity* pEntity = record.pEntity;
        
        for (int i = 1; i < 64; i++) {

            int index = skeleton[i];
            if (index == Skeleton::END) {
                break;
            } else if (index == Skeleton::UP) {
                i++;
            } else {
                int j = i - 1;
                if (j < 0) 
                    continue;
                    
                int index2 = skeleton[j];
                Vec3 pos = getBonePointer(pEntity, index)->pos;
                Vec3 pos2 = getBonePointer(pEntity, index2)->pos;

                D3DCOLOR color = 0x40888888; //0x800080FF;
                Drawing::drawLine3D(pos, pos2, color);
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

    void render() {
        Drawing::setup3DTransforms(pCamData->fov);
        Drawing::setDefaultRenderState();

        DebugDraw::renderLines();

        Entities entities;
        getValidEntityRecords(&entities);

        Drawing::enableDepthTest(false);
        for (auto record : entities) {
            bool alive = record.pEntity->health > 0.0f;
            if (!alive)
                continue;
            if (showLines)
                renderSkeleton(record);
            if (showNumbers || showFrames)
                renderBones(record);
            if (showHead)
                renderHead(record);
        }

        if (showAxes)
            renderAxes();
    }

    // #endregion

}
