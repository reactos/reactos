
#define DEFINE_THISCALL_WRAPPER(func,args) \
    typedef struct {int x[args/4];} _tag_##func; \
    void __stdcall func(_tag_##func p1); \
    __declspec(naked) void __stdcall func(_tag_##func p1) \
    { \
        __asm pop eax \
        __asm push ecx \
        __asm push eax \
        __asm jmp func \
    }

DEFINE_THISCALL_WRAPPER(Allocate_PMemoryAllocator,8)
