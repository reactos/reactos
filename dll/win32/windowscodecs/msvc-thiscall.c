#include "wincodecs_private.h"

#undef DEFINE_THISCALL_WRAPPER
#define DEFINE_THISCALL_WRAPPER(func,args) \
    typedef struct {int x[args/4];} _tag_##func; \
    void __stdcall func(_tag_##func p1); \
    __declspec(naked) void __stdcall __thiscall_##func(_tag_##func p1) \
    { \
        __asm pop eax \
        __asm push ecx \
        __asm push eax \
        __asm jmp func \
    }

DEFINE_THISCALL_WRAPPER(IMILUnknown1Impl_unknown1, 8)
DEFINE_THISCALL_WRAPPER(IMILUnknown1Impl_unknown3, 8)
DEFINE_THISCALL_WRAPPER(IMILUnknown1Impl_unknown8, 4)
