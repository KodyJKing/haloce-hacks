#pragma once

#include "dllmain.h"
#include "halo.h"
#include "haloex.h"
#include "drawing.h"
#include "debugdraw.h"
#include "keypressed.h"
#include "input.h"

namespace Aimbot {

    using Skeleton::BoneOffset;

    RaycastResult rcResult;

    struct Target {
        EntityRecord entityRec;
        BoneOffset boneOffset;

        inline Vec3 worldPos() {
            return Skeleton::boneOffsetToWorld(entityRec.pEntity, boneOffset);
        }

        inline bool exists() {
            return !!entityRec.pEntity;
        }
    };

    bool updateTargetIfOcluded(Target* pTarget) {
        Vec3 pos = pTarget->worldPos();
        Vec3 displacement = (pos - pCamData->pos) * 1.1f;

        RaycastResult result = {};
        DWORD flags = 0x001000E9;
        Vec3 origin = pCamData->pos;
        int status = raycast( flags, 
            &origin, &displacement,
            pPlayerData->entityHandle,
            &result );

        if (!result.entityHandle)
            return false;

        EntityRecord hitRecord = getRecord(result.entityHandle);
        BoneOffset newOffset = Skeleton::worldToBoneOffset(hitRecord.pEntity, result.boneIndex, result.hit);
        
        pTarget->entityRec = hitRecord;
        pTarget->boneOffset = newOffset;

        return true;
    }

    bool checkVisible(Target* pTarget, Vec3 displacement) {
        RaycastResult result = {};
        DWORD flags = 0x001000E9;
        Vec3 origin = pCamData->pos;
        int status = raycast( flags, 
            &origin, &displacement,
            pPlayerData->entityHandle,
            &result );

        if (!result.entityHandle)
            return false;

        if (result.boneIndex != pTarget->boneOffset.boneIndex)
            return false;

        EntityRecord hitRecord = getRecord(result.entityHandle);
        return hitRecord.pEntity == pTarget->entityRec.pEntity;
    }

    bool checkVisible(Target* pTarget) {
        Vec3 pos = pTarget->worldPos();
        Vec3 displacement = (pos - pCamData->pos) * 1.1f;
        return checkVisible(pTarget, displacement);
    }

    float getTargetScore(Target target) {
        if ( target.entityRec.typeId == TypeID_Jackal
            && target.boneOffset.boneIndex == Skeleton::jackalShieldIndex )
            return 0.0f;
        if ( target.entityRec.typeId == TypeID_Hunter
            && target.boneOffset.boneIndex != Skeleton::hunterTorsoIndex )
            return 0.0f;

        float shield = target.entityRec.pEntity->shield;
        Vec3 toEntity = target.worldPos() - pCamData->pos;
        Vec3 unit = toEntity.unit();

        float cosineScore   = unit.dot(pCamData->fwd) + 1.1f;
        float distanceScore = !Options::smartTargeting ? 1.0f : 1.0f / toEntity.length();
        float shieldScore   = !Options::smartTargeting ? 1.0f : 1.0f / (shield + 0.1f);

        return cosineScore * distanceScore * shieldScore;
    }

    void randomizeTarget(Target* pTarget, float scale) {
        float rx = (float) std::rand() / RAND_MAX;
        float ry = (float) std::rand() / RAND_MAX;
        float rz = (float) std::rand() / RAND_MAX;
        pTarget->boneOffset.offset.x += (rx - 0.5f) * scale;
        pTarget->boneOffset.offset.y += (ry - 0.5f) * scale;
        pTarget->boneOffset.offset.z += (rz - 0.5f) * scale;
    }

    bool isValidTarget(EntityRecord record) {
        EntityTraits traits = getEntityTraits(record);
        return traits.hostile && traits.living && record.pEntity->health > 0.0f;
    }

    // === Best Target ===
    bool bPreviousTarget;
    Target previousTarget;
    const float previousTargetBias = 4.0f;
    //
    Target bestTarget() {

        Target best = {};
        float bestScore = 0.0f;

        bool hadPreviousTarget = bPreviousTarget;
        bPreviousTarget = false;

        bool usingPreviousTarget = false;
        if (
            hadPreviousTarget &&
            previousTarget.exists() &&
            isValidTarget(previousTarget.entityRec) &&
            checkVisible(&previousTarget)
        ) {
            float score = getTargetScore(previousTarget) * previousTargetBias;
            if (score > bestScore) {
                bestScore = score;
                best = previousTarget;

                bPreviousTarget = true;
                usingPreviousTarget = true;
            }
        }

        Entities entities;
        getValidEntityRecords(&entities);
        for (auto record : entities) {

            if (!isValidTarget(record))
                continue;

            BoneOffset headBoneOffset = Skeleton::getHeadOffset(record);

            for (uint j = 0; j < getEntityTraits(record).numBones; j++ ) {

                bool isHead = j == headBoneOffset.boneIndex;
                BoneOffset boneOffset = isHead ? headBoneOffset : BoneOffset{ (int)j, {0, 0, 0} };
                Target target = { record, boneOffset };

                bool isPreviousTarget = usingPreviousTarget && j == previousTarget.boneOffset.boneIndex;
                int numRandomOffsets = isPreviousTarget ? 0 : (isHead ? 19 : 4);
                // int numRandomOffsets = isHead ? 5 : 1;
                // int numRandomOffsets = 0;

                for (int i = 0; i < 1 + numRandomOffsets; i++) {
                    Target targetPrime = target;
                    
                    if (i > 0) {
                        float drift = isPreviousTarget ? 1.0f : 15.0f;
                        randomizeTarget(&targetPrime, Skeleton::SMALL_UNIT * drift);
                    }
                    
                    if (!updateTargetIfOcluded(&targetPrime))
                        continue;
                    if (!isValidTarget(targetPrime.entityRec))
                        continue;
                    
                    float score = getTargetScore(targetPrime);
                    if (targetPrime.boneOffset.boneIndex == headBoneOffset.boneIndex)
                        score *= 5.0;

                    if (score > bestScore) {
                        bestScore = score;
                        best = targetPrime;

                        bPreviousTarget = true;
                        break;
                    }
                }

            }

        }
        
        previousTarget = best;

        return best;
    }

    void lookAt(Vec3 pos) {
        Vec3 dir = (pos - pCamData->pos).unit();
        if (Options::smoothTargeting)
            dir = pCamData->fwd.lerp(dir.unit(), 0.5f);
            // dir = pCamData->fwd.lerp(dir, 0.33f);
            // dir = pCamData->fwd.lerp(dir.unit(), 0.25f);
        Angles angles = dir.toAngles();
        pPlayerData->yaw = angles.yaw;
        pPlayerData->pitch = angles.pitch;
        pCamData->fwd = dir;
    }

    void update() {
        
        if (!Options::aimbot)
            return;

        if (GetAsyncKeyState(VK_SHIFT)) {
            Target target = bestTarget();
            if (target.entityRec.pEntity) {
                Vec3 pos = target.worldPos();
                lookAt( pos );
                if ( Options::triggerbot && checkVisible( &target, pCamData->fwd * 50.0f) )
                    Input::click();
            }
        }

        Input::update();
    }

}
