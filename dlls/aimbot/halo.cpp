
#include "halo.h"

EntityTraits getEntityTraits(EntityRecord record) {
    switch (record.typeId) {
        case TypeID_Player:  return { 1, 0, 30 };
        case TypeID_Marine:  return { 1, 0, 20 };
        case TypeID_Grunt:   return { 1, 1, 17 };
        case TypeID_Jackal:  return { 1, 1, 27 };
        case TypeID_Elite:   return { 1, 1, 25 };
        case TypeID_Hunter:  return { 1, 1, 30 };
        default:             return { 0, 1, 0  };
    }
}

// Pointers
EntityList** const ppEntityList = (EntityList**) 0x008603B0;
CameraData*  const pCamData     = (CameraData*)  0x00719BC8;
PlayerData*  const pPlayerData  = (PlayerData*)  0x402AD4AC;

// === Raycast ===

const RaycastFunction raycast = (RaycastFunction) 0x00505880;

void raycastPlayerCrosshair(RaycastResult* pRcResult, DWORD flags) {
    Vec3 origin = pCamData->pos;
    Vec3 displacement = pCamData->fwd * 100.0f;
    int status = raycast( flags, 
        &origin, &displacement,
        pPlayerData->entityHandle,
        pRcResult );
}

// === Entity Accessors ===

inline EntityList* getpEntityList() {
    return *ppEntityList;
}

int findEntityIndex(int address) {
    EntityList* entityList = getpEntityList();
    for (int i = 0; i < entityList->capcity; i++) {
        EntityRecord record = entityList->pEntityRecords[i];
        if ((int)record.pEntity == address)
            return i;
    }
    return -1;
}

EntityRecord getRecord(int handle) {
    int index = handle & 0xFFFF;
    EntityList* entityList = getpEntityList();
    return entityList->pEntityRecords[index];
}


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

    int marine[] = { 
        // Legs
        8, 4, 1, 0, 2, 5, 11, PEN_UP,
        // Spine
        0, 3, 6, 9, 12, 16, PEN_UP,
        // Arms
        19, 17, 14, 10, 9, 7, 13, 15, 18, PEN_END
    };

    int grunt[] = {
        // Legs
        11, 5, 2, 0, 1, 4, 8, PEN_UP,
        // Spine
        0, 3, 6, 9, 12, PEN_UP,
        // Arms
        16, 14, 10, 9, 7, 13, 15, PEN_END
    };

    int jackal[] = {
        // Legs
        16, 11, 7, 1, 0, 2, 8, 14, 18, PEN_UP,
        // Spine
        0, 3, 9, 12, 21, 22, PEN_UP,
        // Arms
        24, 20, 17, 10, 12, 13, 19, 23, 26, 25, PEN_END
    };

    int elite[] = {
        // Legs
        12, 8, 4, 1, 0, 2, 5, 11, 15, PEN_UP,
        // Spine
        0, 3, 6, 9, 14, 17, PEN_UP,
        // Arms
        20, 18, 13, 7, 9, 10, 16, 19, 21, PEN_UP,
        // Jaw
        24, 22, 17, 23, 25, PEN_END
    };

    int hunter[] = {
        // Legs
        27, 22, 11, 5, 2, 0, 1, 4, 8, 20, 25, PEN_UP,
        // Spine
        0, 3, 6, 9, 19, 12, PEN_UP,
        // Arms
        28, 24, 21, 7, 9, 10, 23, 26, 29, PEN_END
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

    Bone* getBonePointer(Entity* pEntity, uint index) {
        const uint entityBoneListOffset = 0x550;
        uint address = ((uint)pEntity) + entityBoneListOffset + index * sizeof(Bone);
        return (Bone*)address;
    }

}
