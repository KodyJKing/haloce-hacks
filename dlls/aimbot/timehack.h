#include "dllmain.h"
#include "hook.h"
#include "vec3.h"
#include "haloex.h"

namespace TimeHack {

    float getTimescale(DWORD entityHandle) {
        if (entityHandle == pPlayerData->entityHandle)
            return 1.0f;
        EntityRecord record = getRecord(entityHandle);
        if (record.typeId == 0x02E4)
            return 0.01f;
        return 0.1f;
    }

    DWORD updatingEntityHandle, oldParentEntityHandle;
    Vec3 oldPos, oldVel;
    void preUpdateEntity() {
        GET_DWORD_REG(entityHandle, ebx);
        
        if (!Options::timeHack) return;

        updatingEntityHandle = entityHandle;
        EntityRecord record = getRecord(entityHandle);
        oldPos = record.pEntity->pos;
        oldVel = record.pEntity->velocity;
        oldParentEntityHandle = record.pEntity->parentEntityHandle;

        //record.pEntity->velocity = oldPos * timescale;
    }

    void postUpdateEntity() {
        if (!Options::timeHack) return;

        float timescale = getTimescale(updatingEntityHandle);

        EntityRecord record = getRecord(updatingEntityHandle);

        Vec3 newPos = record.pEntity->pos;
        Vec3 deltaPos = newPos - oldPos;
        Vec3 newVel = record.pEntity->velocity;
        Vec3 deltaVel = newVel - oldVel;

        if (record.pEntity->parentEntityHandle == oldParentEntityHandle)
            record.pEntity->pos = oldPos + deltaPos * timescale;
        record.pEntity->velocity = oldVel + deltaVel * timescale;
        
    }

    bool doSingleStep = false;
    bool shouldEntityUpdate(DWORD entityHandle) {
        if (!Options::freezeTime || doSingleStep)
            return true;
            
        EntityRecord rec = getRecord(entityHandle);
        if (!rec.pEntity)
            return true;

        Entity* pPlayer = getPlayerPointer();
        if (
            entityHandle == pPlayer->childEntityHandle ||
            entityHandle == pPlayer->parentEntityHandle ||
            entityHandle == pPlayer->vehicleEntityHandle
        )
            return true;
        
        // EntityTraits traits = getEntityTraits(rec);
        // if (!traits.living)
        //     return true;

        return rec.typeId == TypeID_Player;
    }

    DWORD updateEntityHook_trampolineReturn;
    DWORD updateEntityHook_entityHandle;
    Naked void updateEntityHook() {
        __asm mov [updateEntityHook_entityHandle], ebx;

        if (!shouldEntityUpdate(updateEntityHook_entityHandle))
            MAKE_HOOKEE_RETURN;

        preUpdateEntity();
        
        __asm jmp [updateEntityHook_trampolineReturn];
    }

    void init() {
        addJumpHook("UpdateEntity", 0x004F4000U, 6, (DWORD) updateEntityHook, true, &updateEntityHook_trampolineReturn);
        // addJumpCallHook("PreUpdateEntity", 0x004F4000U, 6, (DWORD) preUpdateEntity);
        addJumpCallHook("PostUpdateEntity", 0x004F406CU, 6, (DWORD) postUpdateEntity);
    }

}
