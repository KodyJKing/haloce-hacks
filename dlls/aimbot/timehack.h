#include "dllmain.h"
#include "hook.h"
#include "vec3.h"
#include "haloex.h"
#include "util/math.h"

namespace TimeHack {

    const float walkingSpeed = 0.07f;
    const float timescaleDeadzone = 0.05f;
    const float rotationActivityCoefficient = 100.0f;
    const DWORD unpauseAfterFireMilis = 100;
    const float activityDecayRate = 0.05f;
    const float maxTimescaleDueToTurning = 0.5f;
    const float maxSpeed = walkingSpeed * 20.0f;

    // The tick until which the player is considered to be acting.
    DWORD playerIsActingUntil = 0;
    float activityLevel = 0.0f;

    // Used to allow a selected entity to update despite freeze.
    DWORD allowUpdateHandle;
    DWORD allowUpdateUntil;

    Vec3 previousLook;
    float lookSpeed, lookSpeedSmoothed;
    void updateLookActivity() {
        float sinRot = previousLook.cross(pCamData->fwd).length();
        lookSpeed = asinf(sinRot);
        lookSpeedSmoothed = Math::lerp(lookSpeedSmoothed, lookSpeed, 0.5f);
        // activityLevel += rot * rotationActivityCoefficient;
        previousLook = pCamData->fwd;
    }

    float getTimeScale() {
        auto pPlayer = getPlayerPointer();
        if (!pPlayer)
            return 1.0f;

        float playerSpeed = pPlayer->velocity.length();
        float speedLevel = playerSpeed / walkingSpeed;

        float lookSpeedLevel = Math::smoothstep(0.0f, 1.0f, lookSpeedSmoothed * rotationActivityCoefficient) * maxTimescaleDueToTurning;

        float netLevel = speedLevel + lookSpeedLevel + activityLevel;

        return Math::smoothstep(0.0f, 1.0f, netLevel);
    }

    float getTimeScaleForEntity(DWORD entityHandle) {
        if (entityHandle == pPlayerData->entityHandle)
            return 1.0f;

        // EntityRecord record = getRecord(entityHandle);
        // if (record.typeId == 0x02E4)
        //     return s * 0.1f;

        return getTimeScale();
    }

    DWORD playerFrameCount = 0;
    DWORD updatingEntityHandle, oldParentEntityHandle;
    Vec3 oldPos, oldVel;
    float oldAge, oldFuse;
    ushort oldAnimFrame;
    void preUpdateEntity(DWORD entityHandle) {

        if (entityHandle == pPlayerData->entityHandle)
            playerFrameCount++;
        
        if (!Options::timeHack) return;

        updatingEntityHandle = entityHandle;

        float timescale = getTimeScaleForEntity(updatingEntityHandle);
        
        EntityRecord record = getRecord(entityHandle);
        Entity* pEntity = record.pEntity;

        float speed = pEntity->velocity.length();
        if (speed > maxSpeed) {
            pEntity->velocity = pEntity->velocity.unit() * maxSpeed;
        }
        
        oldPos = pEntity->pos;
        oldVel = pEntity->velocity;
        oldParentEntityHandle = pEntity->parentEntityHandle;
        oldAnimFrame = pEntity->animFrame;

        if (pEntity->entityCategory == EntityCategory_Projectile) {
            oldAge = pEntity->projectileAge;
            oldFuse = pEntity->fuse;

            // pEntity->velocity = oldVel * timescale;
        }

    }

    void postUpdateEntity() {
        if (!Options::timeHack) return;

        float timescale = getTimeScaleForEntity(updatingEntityHandle);

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
            
            // deltaVel = newVel - oldVel * timescale;
        }

        if (pEntity->parentEntityHandle == oldParentEntityHandle)
            pEntity->pos = oldPos + deltaPos * timescale;
        deltaVel = newVel - oldVel;
        
        pEntity->velocity = oldVel + deltaVel * timescale;

    }

    bool isPlayerControlled(DWORD entityHandle) {
        EntityRecord rec = getRecord(entityHandle);
        Entity* pPlayer = getPlayerPointer();
        if (!pPlayer)
            return false;
        return
            rec.pEntity->parentEntityHandle == pPlayerData->entityHandle ||
            entityHandle == pPlayer->childEntityHandle ||
            entityHandle == pPlayer->parentEntityHandle ||
            entityHandle == pPlayer->vehicleEntityHandle ||
            rec.typeId == TypeID_Player;
    }

    bool doSingleStep = false;
    bool shouldEntityUpdate(DWORD entityHandle) {
        bool isSelected = entityHandle == allowUpdateHandle && GetTickCount() < allowUpdateUntil;

        bool isTimescaleDeadzone = getTimeScale() < timescaleDeadzone;
        bool shouldAnythingFreeze = Options::freezeTime || Options::timeHack && isTimescaleDeadzone;
        
        if (!shouldAnythingFreeze || doSingleStep || isSelected)
            return true;

        EntityRecord rec = getRecord(entityHandle);
        if (rec.pEntity && rec.pEntity->entityCategory == EntityCategory_Projectile)
            return true; // Deadzoning does not apply to projectiles.
            
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

    // To check if the player has fired.
    namespace ConsumeClipHook {
        void onConsumeClip(DWORD dwWeapon) {
            if (!dwWeapon)
                return;
            auto pWeapon = (Entity*) dwWeapon;
            if (pWeapon->parentEntityHandle == pPlayerData->entityHandle)
                playerIsActingUntil = GetTickCount() + unpauseAfterFireMilis;
        }

        Naked void hook() {
            DWORD dwWeapon;
            _asm {
                push ebp
                mov ebp, esp
                sub esp, __LOCAL_SIZE

                mov [dwWeapon], edi
            }

            onConsumeClip(dwWeapon);

            _asm {
                add esp, __LOCAL_SIZE
                pop ebp
                ret
            }
        }
    }

    void init() {
        auto hook = addJumpHook("UpdateEntity", 0x004F4000U, 6, (DWORD) updateEntityHook, HK_JUMP | HK_PUSH_STATE);
        updateEntityHook_trampolineReturn = hook.trampolineReturn;
        addJumpHook("PostUpdateEntity", 0x004F406CU, 6, (DWORD) postUpdateEntity, HK_PUSH_STATE);
        addJumpHook("ConsumeClip", 0x004C408FU, 7, (DWORD) ConsumeClipHook::hook, HK_PUSH_STATE);
    }

    void update() {

        // if ( GetAsyncKeyState('U') ) {
        //     RaycastResult result;
        //     raycastPlayerCrosshair(&result, traceProjectileRaycastFlags);
        //     allowUpdateHandle = result.entityHandle;
        //     allowUpdateUntil = GetTickCount() + 500;
        // }

        if (Options::timeHack) {
            auto pPlayer = getPlayerPointer();

            if (!pPlayer)
                return;

            updateLookActivity();

            bool isThrowingGrenade = pPlayer->animId == 0xBC && pPlayer->animFrame < 0x12;

            DWORD now = GetTickCount();
            bool isActing = playerIsActingUntil > now || isThrowingGrenade;
            float targetActivityLevel = isActing ? 1.0f : 0.0f;

            activityLevel = Math::lerp(activityLevel, targetActivityLevel, activityDecayRate);

            //std::cout << pPlayer->animId << " " << pPlayer->animFrame << std::endl;

        }

    }

}
