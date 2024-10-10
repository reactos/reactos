
#ifdef _M_ARM

#include <kxarm.h>

    LEAF_ENTRY call_method
        __assertfail
    LEAF_END call_method

    LEAF_ENTRY call_float_method
        __assertfail
    LEAF_END call_float_method

    LEAF_ENTRY call_double_method
        __assertfail
    LEAF_END call_double_method

#elif defined(_M_AMD64)

#include <asm.inc>
.code64

EXTERN __chkstk:PROC

// DWORD_PTR
// call_method(
//     void *func, // rcx
//     int nb_args, // edx
//     const DWORD_PTR *args );
PUBLIC call_method
FUNC call_method
    push rbp
    .PUSHREG rbp
    push rsi
    .PUSHREG rsi
    push rdi
    .PUSHREG rdi
    mov rbp, rsp
    .SETFRAME rbp, 0
    .ENDPROLOG

    // We need at least space for 4 parameters (home space)
    mov rax, 4
    cmp rdx, rax
    cmovg rax, rdx

    // Allocate dynamic stack space (aligned to 16 bytes)
    lea rax, [rax * 8 + 15]
    and rax, HEX(0FFFFFFFFFFFFFFF0)
    call __chkstk
    sub rsp, rax

    // Save  function pointer in rax
    mov rax, rcx

    // Copy parameters to the stack
    mov rcx, rdx
    mov rdi, rsp
    mov rsi, r8
    rep movsq

    // Set up parameter registers for the call
    mov rcx, qword ptr [rsp + 0]
    mov rdx, qword ptr [rsp + 8]
    mov r8, qword ptr [rsp + 16]
    mov r9, qword ptr [rsp + 24]
    movq xmm0, qword ptr [rsp + 0]
    movq xmm1, qword ptr [rsp + 8]
    movq xmm2, qword ptr [rsp + 16]
    movq xmm3, qword ptr [rsp + 24]
    call rax

    lea rsp, [rbp]
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
