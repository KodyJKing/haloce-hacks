#pragma once

#include "vec3.h"

#define NULL_ENTITY_HANDLE 0xFFFFFFFF

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;

typedef struct {
    // Created with ReClass.NET 1.2 by KN4CK3R
	uint tagHandle; //0x0000
	char pad_0004[16]; //0x0004
	uint ageMilis; //0x0014
	char pad_0018[68]; //0x0018
	Vec3 pos; //0x005C
	Vec3 velocity; //0x0068
	char pad_0074[44]; //0x0074
	Vec3 eyePos; //0x00A0
	char pad_00AC[8]; //0x00AC
	ushort entityCategory; //0x00B4
	char pad_00B6[26]; //0x00B6
	ushort animId; //0x00D0
	ushort animFrame; //0x00D2
	char pad_00D4[12]; //0x00D4
	float health; //0x00E0
	float shield; //0x00E4
	char pad_00E8[44]; //0x00E8
	uint vehicleEntityHandle; //0x0114
	uint childEntityHandle; //0x0118
	uint parentEntityHandle; //0x011C
	char pad_0120[208]; //0x0120
	ushort numBoneBytes; //0x01F0
	ushort bonesOffset; //0x01F2
	char pad_01F4[60]; //0x01F4
	Vec3 lookDir; //0x0230
	char pad_023C[4]; //0x023C
	float fuse; //0x0240
	char pad_0244[4]; //0x0244
	float projectileAge; //0x0248
	float projectileAgeRate; //0x024C
	float projectileAge2; //0x0250
	char pad_0254[200]; //0x0254
	uchar N0000010E; //0x031C
	uchar N00000130; //0x031D
	uchar frags; //0x031E
	uchar plasmas; //0x031F
} Entity; //Size: 0x0320

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
} EntityTraits;

EntityTraits getEntityTraits(EntityRecord record) {
    switch (record.typeId) {
        case TypeID_Player:  return { 1, 0 };
        case TypeID_Marine:  return { 1, 0 };
        case TypeID_Grunt:   return { 1, 1 };
        case TypeID_Jackal:  return { 1, 1 };
        case TypeID_Elite:   return { 1, 1 };
        case TypeID_Hunter:  return { 1, 1 };
        default:             return { 0, 1 };
    }
}

// Pointers
EntityList **ppEntityList = (EntityList**) 0x008603B0;
CameraData *pCamData      = (CameraData*)  0x00719BC8;
PlayerData *pPlayerData   = (PlayerData*)  0x402AD4AC;

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
    if (!handle || handle == NULL_ENTITY_HANDLE)
        return {};
    int index = handle & 0xFFFF;
    EntityList* entityList = getpEntityList();
    return entityList->pEntityRecords[index];
}

uint getBoneCount(Entity* pEntity) {
    return pEntity->numBoneBytes / (13 * 4);
}

Bone* getBonePointer(Entity* pEntity, uint index) {
    uint address = ((uint)pEntity) + pEntity->bonesOffset + index * sizeof(Bone);
    return (Bone*)address;
}
