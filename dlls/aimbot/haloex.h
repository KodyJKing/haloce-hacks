#pragma once

#include "dllmain.h"
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

    const int UP = -1;
    const int END = -2;

    int marine[] = { 
        8, 4, 1, 0, 2, 5, 11, UP,              // Legs
        0, 3, 6, 9, 12, 16, UP,                // Spine
        19, 17, 14, 10, 9, 7, 13, 15, 18, END  // Arms
    };

    int grunt[] = {
        11, 5, 2, 0, 1, 4, 8, UP,      // Legs
        0, 3, 6, 9, 12, UP,            // Spine
        16, 14, 10, 9, 7, 13, 15, END  // Arms
    };

    int jackal[] = {
        16, 11, 7, 1, 0, 2, 8, 14, 18, UP,           // Legs
        0, 3, 9, 12, 21, 22, UP,                     // Spine
        24, 20, 17, 10, 12, 13, 19, 23, 26, 25, END  // Arms
    };

    int elite[] = {
        12, 8, 4, 1, 0, 2, 5, 11, 15, UP,     // Legs
        0, 3, 6, 9, 14, 17, UP,               // Spine
        20, 18, 13, 7, 9, 10, 16, 19, 21, UP, // Arms
        24, 22, 17, 23, 25, END               // Jaw
    };

    int hunter[] = {
        27, 22, 11, 5, 2, 0, 1, 4, 8, 20, 25, UP, // Legs
        0, 3, 6, 9, 19, 12, UP,                   // Spine
        28, 24, 21, 7, 9, 10, 23, 26, 29, END     // Arms
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

// === Debug Functions ===

#define PAD_DWORD std::setfill('0') << std::setw(8)
#define UPPER_HEX std::uppercase << std::hex

void printRecord(EntityRecord record) {
    std::cout
        << PAD_DWORD << UPPER_HEX << record.unknown_1 << " "
        << PAD_DWORD << UPPER_HEX << record.unknown_2 << " "
        << PAD_DWORD << record.pEntity;
}

void printCameraData() {
    std::cout << "{\n";

    #define printVec(v) std::printf("{ x: %f.4, y: %f.4, z: %f.4 }", (v).x, (v).y, (v).z)
    #define printVecLine(name) std::cout << "\n    "#name": "; printVec(pCamData->name)
    printVecLine(pos);
    printVecLine(fwd);
    printVecLine(up);
    #undef printVecLine
    #undef printVec

    std::cout << "\n    fov: " << pCamData->fov;
    std::cout << "\n    viewportHeight: " << std::dec
        << pCamData->viewportHeight;
    std::cout << "\n    viewportWidth: " << std::dec
        << pCamData->viewportWidth;

    std::cout << "\n}\n";
}

void printEntityRecord(int address) {
    EntityList* entityList = getpEntityList();
    int index = findEntityIndex(address);
    if (index) {
        EntityRecord record = entityList->pEntityRecords[index];
        printRecord(record);
        std::cout << std::endl;
    } else {
        std::cout << "Address not found in entity list." << std::endl;
    }
}

void printEntityRecords() {
    EntityList* entityList = getpEntityList();
    const int entriesPerLine = 3;
    for (int i = 0; i < entityList->capcity; i++) {
        EntityRecord record = entityList->pEntityRecords[i];
        if ( i % entriesPerLine == 0 )
            std::cout << std::endl;
        else
            std::cout << " | ";
        printRecord(record);
    }
}
