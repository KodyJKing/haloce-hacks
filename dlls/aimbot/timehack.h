#include "dllmain.h"
#include "drawing.h"
#include "hook.h"
#include "vec3.h"
#include "haloex.h"
#include "util/math.h"

namespace TimeHack {

    const float walkingSpeed = 0.07f;
    const float timescaleDeadzone = 0.05f;
    const float rotationActivityCoefficient = 100.0f;
    const DWORD unpauseAfterFireMilis = 100;
    const DWORD unpauseReloadMilis = 500;
    const float activityDecayRate = 0.05f;
    const float maxTimescaleDueToTurning = 0.5f;
    const float maxSpeed = walkingSpeed * 20.0f;

    // The tick until which the player is considered to be acting.
    DWORD playerIsActingUntil = 0;
    float activityLevel = 0.0f;

    Vec3 previousLook;
    float lookSpeed, lookSpeedSmoothed;
    void updateLookActivity() {
        float sinRot = previousLook.cross(pCamData->fwd).length();
        lookSpeed = asinf(sinRot);
        lookSpeedSmoothed = Math::lerp(lookSpeedSmoothed, lookSpeed, 0.5f);
        previousLook = pCamData->fwd;
    }

    float previousPlasmaCharge;
    void updateActivityLevel() {
        auto pPlayer = getPlayerPointer();

        bool isThrowingGrenade = pPlayer->animId == 0xBC && pPlayer->animFrame < 0x12;

        DWORD now = GetTickCount();
        bool isActing = playerIsActingUntil > now || isThrowingGrenade;

        if (pPlayer->vehicleEntityHandle != NULL_ENTITY_HANDLE)
            isActing |= 
                GetAsyncKeyState( 'W' ) || GetAsyncKeyState( 'A' ) ||
                GetAsyncKeyState( 'S' ) || GetAsyncKeyState( 'D' ) ||
                GetAsyncKeyState( VK_LBUTTON ); // TODO: Replace with more robust checks for vehicle inputs.
        
        auto weaponRec = getRecord(pPlayer->childEntityHandle);
        if (weaponRec.pEntity) {
            float plasmaCharge = weaponRec.pEntity->plasmaCharge;
            if (plasmaCharge > previousPlasmaCharge)
                isActing = true;
            previousPlasmaCharge = plasmaCharge;
        }

        float targetActivityLevel = isActing ? 1.0f : 0.0f;

        activityLevel = Math::lerp(activityLevel, targetActivityLevel, activityDecayRate);
    }

    float getTimeScale() {
        auto pPlayer = getPlayerPointer();
        if (!pPlayer)
            return 1.0f;

        float playerSpeed = pPlayer->velocity.length();
        float speedLevel = playerSpeed / walkingSpeed;

        float lookSpeedLevel = Math::smoothstep(0.0f, 1.0f, lookSpeedSmoothed * rotationActivityCoefficient) * maxTimescaleDueToTurning;

        // float netLevel = speedLevel + lookSpeedLevel + activityLevel;
        float netLevel = lookSpeedLevel + activityLevel;

        if (pPlayer->vehicleEntityHandle == NULL_ENTITY_HANDLE)
            netLevel += speedLevel;

        return Math::smoothstep(0.0f, 1.0f, netLevel);
    }

    float getTimeScaleForEntity(DWORD entityHandle) {
        if (entityHandle == pPlayerData->entityHandle)
            return 1.0f;

        return getTimeScale();
    }

    namespace Rewind {

        ushort oldAnimFrame;
        DWORD updatingEntityHandle, oldParentEntityHandle;

        Vec3 old_pos, old_velocity;
        float old_projectileAge, old_projectileAge2, old_fuse;

        #define REWIND(field, type) \
            auto delta_##field = pEntity->field - old_##field; \
            pEntity->field = old_##field + (type)(delta_##field * timescale)
        #define SAVE(field) old_##field = pEntity->field

        void save(DWORD entityHandle) {

            updatingEntityHandle = entityHandle;

            EntityRecord record = getRecord(entityHandle);
            Entity* pEntity = record.pEntity;

            float speed = pEntity->velocity.length();
            if (speed > maxSpeed)
                pEntity->velocity = pEntity->velocity.unit() * maxSpeed;
            
            oldParentEntityHandle = pEntity->parentEntityHandle;
            oldAnimFrame = pEntity->animFrame;

            SAVE(pos);
            SAVE(velocity);

            if (pEntity->entityCategory == EntityCategory_Projectile) {
                SAVE(projectileAge);
                SAVE(projectileAge2);
                SAVE(fuse);
            }

        }

        void rewind() {

            float timescale = getTimeScaleForEntity(updatingEntityHandle);

            EntityRecord record = getRecord(updatingEntityHandle);
            Entity* pEntity = record.pEntity;

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
                REWIND(projectileAge, float);
                REWIND(projectileAge2, float);
                REWIND(fuse, float);
            }

            // Do not rewind positions if an entity changes mounts.
            if (pEntity->parentEntityHandle == oldParentEntityHandle) {
                REWIND(pos, Vec3);
            }

            REWIND(velocity, Vec3);

        }

        #undef REWIND
        #undef SAVE

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
        bool isTimescaleDeadzone = Options::timeHack && getTimeScale() < timescaleDeadzone;
        bool isTimeFrozen = Options::freezeTime && !doSingleStep;
        bool shouldAnythingFreeze = isTimeFrozen || isTimescaleDeadzone;
        
        if (!shouldAnythingFreeze)
            return true;

        EntityRecord rec = getRecord(entityHandle);
        if (rec.pEntity && rec.pEntity->entityCategory == EntityCategory_Projectile)
            return !isTimeFrozen; // Deadzoning does not apply to projectiles.
            
        return isPlayerControlled(entityHandle);
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

        void setup() {
            addJumpHook("ConsumeClip", 0x004C408FU, 7, (DWORD) hook, HK_PUSH_STATE);
        }
    }

    namespace UpdateEntityHook {
        void preUpdateEntity(DWORD entityHandle) {
            if (!Options::timeHack)
                return;

            Rewind::save(entityHandle);
        }

        void postUpdateEntity() {
            if (!Options::timeHack)
                return;

            Rewind::rewind();
        }

        DWORD trampolineReturn;
        DWORD entityHandle;
        Naked void updateEntityHook() {
            __asm mov [entityHandle], ebx;

            if (!shouldEntityUpdate(entityHandle))
                POPSTATE_AND_RETURN; // Forces hooked function to return early so entity doesn't update.

            preUpdateEntity(entityHandle);
            
            __asm jmp [trampolineReturn];
        }

        void setup() {
            addJumpHook("UpdateEntity", 0x004F4000U, 6, (DWORD) updateEntityHook, HK_JUMP | HK_PUSH_STATE, &trampolineReturn);
            addJumpHook("PostUpdateEntity", 0x004F406CU, 6, (DWORD) postUpdateEntity, HK_PUSH_STATE);
        }

    }

    // Hook for instruction found to run on melee, weapon switch and reload.
    void onMiscPlayerAction() {
        playerIsActingUntil = GetTickCount() + unpauseReloadMilis;
    }

    void init() {
        UpdateEntityHook::setup();
        ConsumeClipHook::setup();
        
        auto hook = addJumpHook("MiscPlayerAction", 0x0049327CU, 7, (DWORD) onMiscPlayerAction, HK_PUSH_STATE);
        hook.unprotectTrampoline();
        hook.fixStolenOffset(3);
        hook.protectTrampoline();
    }

    void renderTracers();

    void update() {

        if (Options::timeHack) {
            auto pPlayer = getPlayerPointer();

            if (!pPlayer)
                return;

            updateLookActivity();
            updateActivityLevel();

            //std::cout << pPlayer->animId << " " << pPlayer->animFrame << std::endl;

            // int midx = (int) pCamData->viewportWidth / 2;
            // Drawing::drawText(midx, 0, 0xFFFF0000, {0, 1}, "SUPERHOT Mod");

            renderTracers();

        }

    }

    void renderTracers() {

        Drawing::setDefaultRenderState();
        Drawing::setup3DTransforms(pCamData->fov);
        Drawing::enableDepthTest(true);

        Entities records;
        getValidEntityRecords(&records);
        for (auto rec : records) {

            Entity* pEntity = rec.pEntity;
            
            if (pEntity->entityCategory != EntityCategory_Projectile)
                continue;

            float epsilon = 0.001f;
            Vec3 UP = {0, 0, epsilon};

            Vec3 pos = pEntity->pos - UP;
            Vec3 oldPos = pEntity->pos - pEntity->velocity * 0.5f - UP;
            Drawing::drawThickLine3D(
                pos, oldPos, 0.01f,
                0xFFFF0000, 0x00FF0000
            );

        }

    }

}
