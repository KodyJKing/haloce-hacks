#include "dllmain.h"
#include "hook.h"
#include "halo.h"
#include "haloex.h"

namespace BigHeadHack {

    void onAfterUpdate(DWORD entityHandle) {

        auto rec = getRecord(entityHandle);

        if (rec.typeId == TypeID_Marine) {

            Bone* bone;

            #define SCALE(i, s) \
                bone = getBonePointer(rec.pEntity, i);  \
                bone->x = bone->x * s; \
                bone->y = bone->y * s; \
                bone->z = bone->z * s;
            
            SCALE(12, 10.0f)
            SCALE(16, 15.0f)
            
            // float c = sinf( (GetTickCount() + entityHandle) * 5.0f / 1000.0f );
            // float h = (c + 1) * 0.5f;
            // SCALE(12, (1.0f + 4.0f * h))
            // SCALE(16, (1.0f + 6.5f * h))

            #undef SCALE

        }
        
    }

    DWORD returnAddress, entityHandle;
    Naked void afterUpdateBoneTransforms() {
        __asm {
            pushad
            pushfd
        }

        if (Options::bigHeads)
            onAfterUpdate(entityHandle);

        __asm {
            popfd
            popad
            jmp [returnAddress]
        }
    }

    DWORD trampolineReturn;
    Naked void rerouteHook() {
        // Reroute return through our hook.
        __asm {
            push eax

            mov eax, [esp+0x08]
            mov [entityHandle], eax

            mov eax, [esp+0x04]
            mov [returnAddress], eax

            mov eax, afterUpdateBoneTransforms
            mov [esp+0x04], eax

            pop eax

            jmp [trampolineReturn]
        }
    }

    void init() {

        addJumpHook("UpdateBoneTransforms", 0x004f8310U, 9, (DWORD) rerouteHook, HK_JUMP | HK_STOLEN_AFTER, &trampolineReturn );

    }

}