#pragma once

#include "includes.h"
#include "vec3.h"

#define NULLHANDLE 0xFFFFFFFF
#define PROJECTILE_RAYCAST_FLAGS 0x001000E9

typedef unsigned int uint;
typedef unsigned short ushort;

struct Entity {
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
};

struct EntityRecord {
    ushort id;
    ushort unknown_1;
    ushort unknown_2;
    ushort typeId;
    Entity *pEntity;
};

struct EntityList {
    char pad_0[0x2E];
    ushort capcity;
    ushort count;
    char pad_1[0x2];
    EntityRecord *pEntityRecords;
};

struct CameraData {
    Vec3 pos, fwd, up;
    char pad_0[0x4];
    float fov;
    char pad_1[0x5C];
    ushort viewportHeight, viewportWidth;
};

struct PlayerData {
	uint entityHandle;
	char pad_0[8];
	float yaw;
	float pitch;
	char pad_1[12];
	uint weaponIndex;
	short zoomLevel;
	char pad_2[2];
	uint crosshairEntityHandle;
};

struct Matrix13 {
    float w;
    Vec3 x, y, z, pos;
};

struct EntityTraits {
    uint living;
    uint hostile;
    uint numBones;
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

enum HitType {
    HitType_Map = 0x2,
    HitType_Entity = 0x3,
    HitType_Nothing = 0xFFFF,
};

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

EntityTraits getEntityTraits(EntityRecord record);

typedef uint (__cdecl *RaycastFunction)(DWORD flags, Vec3 *pRayOrigin, Vec3 *pRayDisplacement, DWORD sourceEntityHandle, RaycastResult *pResult);
typedef std::vector<EntityRecord> Entities;
typedef Matrix13 Bone;

// Pointers
extern EntityList** const ppEntityList;
extern CameraData*  const pCamData;
extern PlayerData*  const pPlayerData;
extern const RaycastFunction raycast;

inline EntityList* getpEntityList();
int findEntityIndex(int address);
EntityRecord getRecord(int handle);
void getValidEntityRecords(Entities * pList);
void raycastPlayerCrosshair(RaycastResult* pRcResult, DWORD flags);
Entity* getPlayerPointer();

namespace Skeleton {

    #define HUNTER_TORSO_INDEX 3
    #define JACKAL_SHIELD_INDEX 26
    #define PEN_UP -1
    #define PEN_END -2
    #define SMALL_UNIT 0.0125f

    struct BoneOffset {
        int boneIndex;
        Vec3 offset;
    };

    int* getEntitySkeleton(EntityRecord record);
    BoneOffset getHeadOffset(EntityRecord record);
    Vec3 boneOffsetToWorld(Entity* pEntity, BoneOffset boneOff);
    BoneOffset worldToBoneOffset(Entity* pEntity, int boneIndex, Vec3 worldPos);
    Vec3 getHeadPos(EntityRecord record);
    Bone* getBonePointer(Entity* pEntity, uint index);

}
