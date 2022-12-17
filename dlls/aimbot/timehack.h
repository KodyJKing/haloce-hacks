#include "dllmain.h"
#include "hook.h"
#include "vec3.h"
#include "haloex.h"

namespace TimeHack {

    float walkingSpeed = 0.07f;
    float speedDeadzone = 0.005f;

    // Used to allow a selected entity to update despite freeze.
    DWORD allowUpdateHandle;
    DWORD allowUpdateUntil;

    float getTimescale(DWORD entityHandle) {
        if (entityHandle == pPlayerData->entityHandle)
            return 1.0f;

        auto pPlayer = getPlayerPointer();
        float playerSpeed = pPlayer->velocity.length();
        float s = playerSpeed / walkingSpeed;

        EntityRecord record = getRecord(entityHandle);
        // if (record.typeId == 0x02E4)
        //     return s * 0.1f;
        return s;
    }

    DWORD playerFrameCount = 0;
    DWORD updatingEntityHandle, oldParentEntityHandle;
    Vec3 oldPos, oldVel;
    float oldAge, oldFuse;
    ushort oldAnimFrame;
    void preUpdateEntity(DWORD entityHandle) {
        // GET_DWORD_REG(entityHandle, ebx);

        if (entityHandle == pPlayerData->entityHandle)
            playerFrameCount++;
        
        if (!Options::timeHack) return;

        updatingEntityHandle = entityHandle;

        float timescale = getTimescale(updatingEntityHandle);
        
        EntityRecord record = getRecord(entityHandle);
        Entity* pEntity = record.pEntity;
        
        oldPos = pEntity->pos;
        oldVel = pEntity->velocity;
        oldParentEntityHandle = pEntity->parentEntityHandle;
        oldAnimFrame = pEntity->animFrame;

        if (pEntity->entityCategory == EntityCategory_Projectile) {
            oldAge = pEntity->projectileAge;
            oldFuse = pEntity->fuse;

            pEntity->velocity = oldVel * timescale;
        }

        //record.pEntity->velocity = oldPos * timescale;
    }

    void postUpdateEntity() {
        if (!Options::timeHack) return;

        float timescale = getTimescale(updatingEntityHandle);

        EntityRecord record = getRecord(updatingEntityHandle);
        Entity* pEntity = record.pEntity;

        Vec3 newPos = pEntity->pos;
        Vec3 deltaPos = newPos - oldPos;
        Vec3 newVel = pEntity->velocity;
        Vec3 deltaVel;

        ushort newAnimFrame = pEntity->animFrame;
        ushort deltaAnimFrame = newAnimFrame - oldAnimFrame;
        if (deltaAnimFrame == 1) {
            ushort framesToAdd = (ushort) floorf(deltaAnimFrame * timescale);
            float lostFrames = deltaAnimFrame * timescale - framesToAdd;
            float u = (float) rand() / RAND_MAX;
            if (u < lostFrames)
                framesToAdd++;
            pEntity->animFrame = oldAnimFrame + framesToAdd;
        }

        if (pEntity->entityCategory == EntityCategory_Projectile) {
            float newAge = pEntity->projectileAge;
            float deltaAge = newAge - oldAge;
            pEntity->projectileAge = oldAge + deltaAge * timescale;

            float newFuse = pEntity->fuse;
            float deltaFuse = newFuse - oldFuse;
            pEntity->fuse = oldFuse + deltaFuse * timescale;
            
            deltaVel = newVel - oldVel * timescale;
        } else {
            if (pEntity->parentEntityHandle == oldParentEntityHandle)
                pEntity->pos = oldPos + deltaPos * timescale;

            deltaVel = newVel - oldVel;
        }
        
        pEntity->velocity = oldVel + deltaVel * timescale;

    }

    bool isPlayerControlled(DWORD entityHandle) {
        EntityRecord rec = getRecord(entityHandle);
        Entity* pPlayer = getPlayerPointer();
        return
            entityHandle == pPlayer->childEntityHandle ||
            entityHandle == pPlayer->parentEntityHandle ||
            entityHandle == pPlayer->vehicleEntityHandle ||
            rec.typeId == TypeID_Player;
    }

    bool doSingleStep = false;
    bool shouldEntityUpdate(DWORD entityHandle) {
        bool isSelected = entityHandle == allowUpdateHandle && GetTickCount() < allowUpdateUntil;

        Entity* pPlayer = getPlayerPointer();
        bool isPlayerDeadzoneSpeed = pPlayer->velocity.length() < speedDeadzone;
        bool shouldAnythingFreeze = Options::freezeTime || Options::timeHack && isPlayerDeadzoneSpeed;
        
        if (!shouldAnythingFreeze || doSingleStep || isSelected)
            return true;
            
        return isPlayerControlled(entityHandle);
    }

    DWORD updateEntityHook_trampolineReturn;
    DWORD updateEntityHook_entityHandle;
    Naked void updateEntityHook() {
        __asm mov [updateEntityHook_entityHandle], ebx;

        if (!shouldEntityUpdate(updateEntityHook_entityHandle))
            POPSTATE_AND_RETURN; // Forces hooked function to return early so entity doesn't update.

        preUpdateEntity(updateEntityHook_entityHandle);
        
        __asm jmp [updateEntityHook_trampolineReturn];
    }

    void init() {
        auto hook = addJumpHook("UpdateEntity", 0x004F4000U, 6, (DWORD) updateEntityHook, HK_JUMP | HK_PUSH_STATE);
        updateEntityHook_trampolineReturn = hook.trampolineReturn;

        addJumpHook("PostUpdateEntity", 0x004F406CU, 6, (DWORD) postUpdateEntity, HK_PUSH_STATE);
    }

    void update() {
        if ( GetAsyncKeyState('U') ) {
            RaycastResult result;
            raycastPlayerCrosshair(&result, traceProjectileRaycastFlags);
            allowUpdateHandle = result.entityHandle;
            allowUpdateUntil = GetTickCount() + 500;
        }
    }

}
