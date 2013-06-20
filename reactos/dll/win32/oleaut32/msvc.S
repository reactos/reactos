
#include <asm.inc>

#ifdef _M_AMD64
.code64

PUBLIC call_method
call_method:
    int 2ch
    ret

#else
.code32

PUBLIC _call_method
_call_method:
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov edx, dword ptr ds:[ebp + 12]
    shl edx, 2
    jz cm1
    sub esp, edx
    and esp, HEX(0FFFFFFF0)
    mov ecx, dword ptr ds:[ebp + 12]
    mov esi, dword ptr ds:[ebp + 16]
    mov edi, esp
    cld
    rep movsd
cm1:
    call dword ptr ds:[ebp + 8]
    lea esp, [ebp - 8]
    pop edi
    pop esi
    pop ebp
    ret
#endif

END
