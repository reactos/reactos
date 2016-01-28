
#ifdef _M_ARM

#include <kxarm.h>

    LEAF_ENTRY call_method
        __assertfail
    LEAF_END call_method

#elif defined(_M_AMD64)

#include <asm.inc>
.code64

PUBLIC call_method
call_method:
    int 2ch
    ret

#else

#include <asm.inc>
.code32

PUBLIC _call_method
_call_method:
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov edx, dword ptr ds:[ebp + 12]
    mov edi, esp
    shl edx, 2
    jz cm1
    sub edi, edx
    and edi, HEX(0FFFFFFF0)
    mov esp, edi
    mov ecx, dword ptr ds:[ebp + 12]
    mov esi, dword ptr ds:[ebp + 16]
    cld
    rep movsd
cm1:
    call dword ptr ds:[ebp + 8]
    sub edi, esp
    mov ecx,[ebp + 20]
    mov [ecx], edi
    lea esp, [ebp - 8]
    pop edi
    pop esi
    pop ebp
    ret
#endif

    END
