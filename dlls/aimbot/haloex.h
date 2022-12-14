#pragma once

#include "includes.h"
#include "halo.h"

// === Raycasting ===

enum HitType {
    HitType_Map = 0x2,
    HitType_Entity = 0x3,
    HitType_Nothing = 0xFFFF,
};

struct RaycastResult {
    u_short hitType;
    char pad_0[0x12];
    float unknown_0;
    Vec3 hit, normal;
    char pad_1[0x8];
    DWORD entityHandle;
    u_short unknown_1;
    u_short boneIndex;
    char pad_2[0x10];
};

typedef uint (__cdecl *RaycastFunction)(DWORD flags, Vec3 *pRayOrigin, Vec3 *pRayDisplacement, DWORD sourceEntityHandle, RaycastResult *pResult);
const RaycastFunction raycast = (RaycastFunction) 0x00505880;

const DWORD traceProjectileRaycastFlags = 0x001000E9; // See 004C04B6.

void raycastPlayerCrosshair(RaycastResult* pRcResult, DWORD flags) {
    Vec3 origin = pCamData->pos;
    Vec3 displacement = pCamData->fwd * 100.0f;
    int status = raycast( flags, 
        &origin, &displacement,
        pPlayerData->entityHandle,
        pRcResult );
}

// === Entity Accessors ===

typedef std::vector<EntityRecord> Entities;

void getValidEntityRecords(Entities* pList) {
    EntityList* entityList = getpEntityList();
    for (int i = 0; i < entityList->capcity; i++) {
        EntityRecord record = entityList->pEntityRecords[i];
        if (!record.pEntity)
            continue;
        pList->push_back(record);
    }
}

Entity* getPlayerPointer() {
    auto record = getRecord( pPlayerData->entityHandle );
    return record.pEntity;
}

// === Skeletons ===

namespace Skeleton {

    const int PEN_UP = -1;
    const int PEN_END = -2;

    int marine[] = { 
        8, 4, 1, 0, 2, 5, 11, PEN_UP,              // Legs
        0, 3, 6, 9, 12, 16, PEN_UP,                // Spine
        19, 17, 14, 10, 9, 7, 13, 15, 18, PEN_END  // Arms
    };

    int grunt[] = {
        11, 5, 2, 0, 1, 4, 8, PEN_UP,      // Legs
        0, 3, 6, 9, 12, PEN_UP,            // Spine
        16, 14, 10, 9, 7, 13, 15, PEN_END  // Arms
    };

    int jackal[] = {
        16, 11, 7, 1, 0, 2, 8, 14, 18, PEN_UP,           // Legs
        0, 3, 9, 12, 21, 22, PEN_UP,                     // Spine
        24, 20, 17, 10, 12, 13, 19, 23, 26, 25, PEN_END  // Arms
    };

    int elite[] = {
        12, 8, 4, 1, 0, 2, 5, 11, 15, PEN_UP,     // Legs
        0, 3, 6, 9, 14, 17, PEN_UP,               // Spine
        20, 18, 13, 7, 9, 10, 16, 19, 21, PEN_UP, // Arms
        24, 22, 17, 23, 25, PEN_END               // Jaw
    };

    int hunter[] = {
        27, 22, 11, 5, 2, 0, 1, 4, 8, 20, 25, PEN_UP, // Legs
        0, 3, 6, 9, 19, 12, PEN_UP,                   // Spine
        28, 24, 21, 7, 9, 10, 23, 26, 29, PEN_END     // Arms
    };

    int* getEntitySkeleton(EntityRecord record) {
        switch (record.typeId) {
            case TypeID_Marine:  return marine;
            case TypeID_Grunt:   return grunt;
            case TypeID_Jackal:  return jackal;
            case TypeID_Elite:   return elite;
            case TypeID_Hunter:  return hunter;
            // case TypeID_Player:
            default:             return nullptr;
        }
    }

    const float SMALL_UNIT = 0.0125f;

    const int hunterTorsoIndex = 3;
    const int jackalShieldIndex = 26;

    struct BoneOffset {
        int boneIndex;
        Vec3 offset;
    };

    BoneOffset getHeadOffset(EntityRecord record) {
        const float u = SMALL_UNIT;
        switch (record.typeId) {
            case TypeID_Marine:  return { 12, Vec3{ 3, 1, 0 } * u };
            case TypeID_Grunt:   return { 12, Vec3{ 3, 2, 0 } * u };
            case TypeID_Jackal:  return { 15, Vec3{ 2, 2, 0 } * u };
            case TypeID_Elite:   return { 17, Vec3{ 0, 0, 0 } * u };
            case TypeID_Hunter:  return {  3, Vec3{ 1, 0, 0 } * u };
            // case TypeID_Player:
            default:             return {  3, Vec3{ 1, 0, 0 } * u };
        }
    }

    Vec3 boneOffsetToWorld(Entity* pEntity, BoneOffset boneOff) {
        Bone* bone = getBonePointer(pEntity, boneOff.boneIndex);
        return bone->pos
            + bone->x * boneOff.offset.x
            + bone->y * boneOff.offset.y
            + bone->z * boneOff.offset.z;
    }

    BoneOffset worldToBoneOffset(Entity* pEntity, int boneIndex, Vec3 worldPos) {
        Bone* bone = getBonePointer(pEntity, boneIndex);
        Vec3 diff = worldPos - bone->pos;
        return {
            boneIndex,
            {
                bone->x.dot(diff),
                bone->y.dot(diff),
                bone->z.dot(diff)
            }
        };
    }

    Vec3 getHeadPos(EntityRecord record) {
        Entity* pEntity = record.pEntity;
        BoneOffset head = getHeadOffset(record);
        return boneOffsetToWorld(pEntity, head);
    }

}
