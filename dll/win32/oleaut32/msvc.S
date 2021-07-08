
#ifdef _M_ARM

#include <kxarm.h>

    LEAF_ENTRY call_method
        __assertfail
    LEAF_END call_method

#elif defined(_M_AMD64)

#include <asm.inc>
.code64

PUBLIC call_method
FUNC call_method
    push rbp
    .PUSHREG rbp
    mov rbp, rsp
    push rsi
    .PUSHREG rsi
    push rdi
    .PUSHREG rdi
    .ENDPROLOG

    mov rax, rcx
    mov rcx, 4
    cmp rdx, rcx
    cmovg rcx, rdx
    lea rdx, [rcx * 8]
    sub rsp, rdx
    and rsp, HEX(0FFFFFFFFFFFFFFF0)
    mov rdi, rsp
    mov rsi, r8
    rep movsq
    mov rcx, qword ptr [rsp + 0]
    mov rdx, qword ptr [rsp + 8]
    mov r8, qword ptr [rsp + 16]
    mov r9, qword ptr [rsp + 24]
    movq xmm0, qword ptr [rsp + 0]
    movq xmm1, qword ptr [rsp + 8]
    movq xmm2, qword ptr [rsp + 16]
    movq xmm3, qword ptr [rsp + 24]
    call qword ptr [rax]

    lea rsp, [rbp - 16]
    pop rdi
    pop rsi
    pop rbp
    ret
ENDFUNC

PUBLIC call_double_method
call_double_method:
    jmp call_method

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

PUBLIC _call_double_method
_call_double_method:
    jmp _call_method

#endif

    END
