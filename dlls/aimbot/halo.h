#pragma once

#ifdef __cplusplus
    #define IF_C(x)
#else
    #define IF_C(x) x
#endif

#include "vec3.h"

#define NULL_ENTITY_HANDLE 0xFFFFFFFF

typedef unsigned int uint;
typedef unsigned short ushort;

typedef struct {
	// Created with ReClass.NET 1.2 by KN4CK3R
    uint tag;
	char pad_0000[88];
	Vec3 pos, velocity; 
	char pad_0068[44];  
	Vec3 eyePos;
	char pad_00AC[8];
	ushort entityCategory;
	char pad_00B6[42];
	float health;
	float shield;
	char pad_00E8[44];
	uint vehicleEntityHandle;
	uint childEntityHandle;
	uint parentEntityHandle;
	char pad_0120[272];
	Vec3 lookDir;       
	char pad_023C[4];
	float fuse;
	char pad_0244[4];
	float projectileAge;
	float projectileAgeRate;
	// char pad_0250[214];
	// char frags;         
	// char plasmas;       
} Entity;

typedef struct {
    ushort id;
    ushort unknown_1;
    ushort unknown_2;
    ushort typeId;
    Entity *pEntity;
} EntityRecord;

typedef struct {
    char pad_0[0x2E];
    ushort capcity;
    ushort count;
    char pad_1[0x2];
    EntityRecord *pEntityRecords;
} EntityList;

typedef struct {
    Vec3 pos, fwd, up;
    char pad_0[0x4];
    float fov;
    char pad_1[0x5C];
    ushort viewportHeight, viewportWidth;
} CameraData;

typedef struct {
	uint entityHandle;
	char pad_0[8];
	float yaw;
	float pitch;
	char pad_1[12];
	uint weaponIndex;
	short zoomLevel;
	char pad_2[2];
	uint crosshairEntityHandle;
} PlayerData;

typedef struct {
    float w;
    Vec3 x, y, z, pos;
} Matrix13;

typedef Matrix13 Bone;

enum EntityCategory {
    EntityCategory_Biped,
    EntityCategory_Vehicle,
    EntityCategory_Weapon,
    EntityCategory_Equipment,
    EntityCategory_Garbage,
    EntityCategory_Projectile,
    EntityCategory_Scenery,
    EntityCategory_Machine,
    EntityCategory_Control,
    EntityCategory_LightFixture,
    EntityCategory_Placeholder,
    EntityCategory_SoundScenery,
};

enum TypeID {
    TypeID_Player = 0x0DEC,
    TypeID_Marine = 0x0E60,
    TypeID_Grunt  = 0x0D04,
    TypeID_Jackal = 0x118C,
    TypeID_Elite  = 0x1118,
    TypeID_Hunter = 0x12E8,
};

typedef struct {
    uint living;
    uint hostile;
    uint numBones;
} EntityTraits;

EntityTraits getEntityTraits(EntityRecord record) {
    switch (record.typeId) {
        #define CAST IF_C((EntityTraits))
        case TypeID_Player:  return CAST{ 1, 0, 30 };
        case TypeID_Marine:  return CAST{ 1, 0, 20 };
        case TypeID_Grunt:   return CAST{ 1, 1, 17 };
        case TypeID_Jackal:  return CAST{ 1, 1, 27 };
        case TypeID_Elite:   return CAST{ 1, 1, 25 };
        case TypeID_Hunter:  return CAST{ 1, 1, 30 };
        default:             return CAST{ 0, 1, 0  };
        #undef CAST
    }
}

// Pointers
EntityList** const ppEntityList = (EntityList**) 0x008603B0;
CameraData*  const pCamData     = (CameraData*)  0x00719BC8;
PlayerData*  const pPlayerData  = (PlayerData*)  0x402AD4AC;

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
    return 0;
}

EntityRecord getRecord(int handle) {
    int index = handle & 0xFFFF;
    EntityList* entityList = getpEntityList();
    return entityList->pEntityRecords[index];
}

// EntityRecord* getRecordPointer(int handle) {
//     if (!handle || handle == NULL_ENTITY_HANDLE)
//         return nullptr;
//     int index = handle & 0xFFFF;
//     EntityList* entityList = getpEntityList();
//     EntityRecord* result = &entityList->pEntityRecords[index];
//     if (!result->pEntity)
//         return nullptr;
//     return result;
// }

Bone* getBonePointer(Entity* pEntity, uint index) {
    static const uint entityBoneListOffset = 0x550;
    uint address = ((uint)pEntity) + entityBoneListOffset + index * sizeof(Bone);
    return (Bone*)address;
}
