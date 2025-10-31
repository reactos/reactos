
#if defined(_M_IX86) || defined(_M_AMD64)
#include <asm.inc>
#elif defined(_M_ARM)
#include <kxarm.h>
#endif

#ifdef _M_IX86
.code32

EXTERN _NdrpClientCall2@16:PROC

PUBLIC _call_stubless_func
_call_stubless_func:

    mov ecx, [esp+4]                /* This Pointer */
    mov ecx, [ecx]                  /* This->lpVtbl */
    mov ecx, [ecx-8]                /* MIDL_STUBLESS_PROXY_INFO */
    mov edx, [ecx+8]                /* Info->FormatStringOffset */
    movzx edx, word ptr [edx+eax*2] /* FormatStringOffset[index] */
    add edx, [ecx+4]                /* info->ProcFormatString + offset */
    movzx eax, byte ptr [edx+1]     /* Oi_flags */
    and eax, 8                      /* Oi_HAS_RPCFLAGS */
    shr eax, 1
    movzx eax, word ptr [edx+eax+4] /* arguments size */
    push eax
    // .cfi_adjust_cfa_offset 4
    push 0                          /* fpu_stack */
    // .cfi_adjust_cfa_offset 4
    lea eax, [esp + 12]             /* &This */
    push eax
    // .cfi_adjust_cfa_offset 4
    push edx                        /* format string */
    // .cfi_adjust_cfa_offset 4
    push [ecx]                      /* info->pStubDesc */
    // .cfi_adjust_cfa_offset 4
    call _NdrpClientCall2@16
    // .cfi_adjust_cfa_offset -16
    pop edx                         /* arguments size */
    // .cfi_adjust_cfa_offset -4
    mov ecx, [esp]                  /* return address */
    add esp, edx
    jmp ecx

PUBLIC _NdrClientCall2
_NdrClientCall2:
    push ebp
    // .cfi_adjust_cfa_offset 4
    // .cfi_rel_offset %ebp,0
    mov ebp, esp
    // .cfi_def_cfa_register %ebp
    push 0                /* fpu_stack */
    push [ebp + 16]       /* stack */
    push [ebp + 12]       /* format */
    push [ebp + 8]        /* desc */
    call _NdrpClientCall2@16
    leave
    // .cfi_def_cfa %esp,4
    // .cfi_same_value %ebp
    ret

EXTERN _ndr_async_client_call:PROC
PUBLIC _NdrAsyncClientCall
_NdrAsyncClientCall:
    push ebp
    // .cfi_adjust_cfa_offset 4
    // .cfi_rel_offset %ebp,0
    mov ebp, esp
    // .cfi_def_cfa_register %ebp
    push 0             /* fpu_stack */
    push [ebp + 16]       /* stack */
    push [ebp + 12]       /* format */
    push [ebp + 8]        /* desc */
    call _ndr_async_client_call
    leave
    // .cfi_def_cfa %esp,4
    // .cfi_same_value %ebp
    ret

#elif _M_AMD64
.code64

EXTERN NdrpClientCall2:PROC
EXTERN ndr_stubless_client_call:PROC

PUBLIC call_stubless_func
FUNC call_stubless_func
    mov [rsp + 8], rcx
    .SAVEREG rcx, 8
    mov [rsp + 10h], rdx
    .SAVEREG rdx, 10h
    mov [rsp + 18h], r8
    .SAVEREG r8, 18h
    mov [rsp + 20h], r9
    .SAVEREG r9, 20h
    sub rsp, 38h
    .ALLOCSTACK 38h
    .ENDPROLOG

    lea rdx, [rsp + 40h]     /* args */
    movsd qword ptr [rsp + 20h], xmm1
    movsd qword ptr [rsp + 28h], xmm2
    movsd qword ptr [rsp + 30h], xmm3
    lea r8, [rsp + 18h]      /* fpu_regs */
    mov ecx, r10d            /* index */
    call ndr_stubless_client_call
    add rsp, 38h
    ret
ENDFUNC

PUBLIC call_server_func
FUNC call_server_func
    push rbp
    .PUSHREG rbp
    mov rbp, rsp
    push rsi
    .PUSHREG rsi
    push rdi
    .PUSHREG rdi
    .ENDPROLOG

    mov rax, rcx   /* function to call */
    mov rcx, 32    /* allocate max(32,stack_size) bytes of stack space */
    cmp r8, rcx
    cmovg rcx, r8
    sub rsp, rcx
    and rsp, NOT 15
    mov rcx, r8
    shr rcx, 3
    mov rdi, rsp
    mov rsi, rdx
    rep movsq       /* copy arguments */
    mov rcx, [rsp]
    mov rdx, [rsp + 8]
    mov r8, [rsp + 16]
    mov r9, [rsp + 24]

    /* Usually the 64 bit SSE2 version of movd is called movq, as in GCC code
       (see https://www.felixcloutier.com/x86/movd:movq). But there is another
       movq with different encoding, which does not accept an integer register
       as source (see https://www.felixcloutier.com/x86/movq). Older versions
       of ML64 get confused and do not accept movq with integer registers,
       but they translate movd to 64 bit, when 64 bit registers are used as
       source, so we use that here. */
    movd xmm0, rcx
    movd xmm1, rdx
    movd xmm2, r8
    movd xmm3, r9
    call rax

    lea rsp, [rbp - 16] /* restore stack */
    pop rdi
    pop rsi
    pop rbp
    ret
ENDFUNC


PUBLIC NdrClientCall2
FUNC NdrClientCall2
    mov [rsp + 18h], r8
    .SAVEREG r8, 18h
    mov [rsp + 20h], r9
    .SAVEREG r9, 20h
    sub rsp, 28h
    .ALLOCSTACK 28h
    .ENDPROLOG

    lea r8, [rsp + 28h + 18h]  /* stack */
    xor r9, r9                 /* fpu_stack */
    call NdrpClientCall2

    add rsp, 28h
    ret
ENDFUNC

EXTERN ndr_async_client_call:PROC
PUBLIC NdrAsyncClientCall
FUNC NdrAsyncClientCall
    mov [rsp + 18h], r8
    .SAVEREG r8, 18h
    mov [rsp + 20h], r9
    .SAVEREG r9, 20h
    sub rsp, 28h
    .ALLOCSTACK 28h
    .ENDPROLOG

    lea r8, [rsp + 28h + 18h]  /* stack */
    xor r9, r9                 /* fpu_stack */
    call ndr_async_client_call

    add rsp, 28h
    ret
ENDFUNC

EXTERN ndr64_client_call:PROC
PUBLIC NdrClientCall3
FUNC NdrClientCall3
    sub rsp, 28h
    .ALLOCSTACK 28h
    .ENDPROLOG

    mov [rsp + 48h], r9
    lea r9, [rsp + 48h] /* stack */
    call ndr64_client_call
    add rsp, 28h
    ret
ENDFUNC

EXTERN ndr64_async_client_call:PROC
PUBLIC Ndr64AsyncClientCall
FUNC Ndr64AsyncClientCall
    mov [rsp + 20h], r9
    lea r9, [rsp + 20h]
    push 0
    sub rsp, 20h
    .ALLOCSTACK 28h
    .ENDPROLOG

    call ndr64_async_client_call
    add rsp, 28h
    ret
ENDFUNC

#elif _M_ARM
TEXTAREA

    LEAF_ENTRY call_stubless_func

    /* Unimplemented */
    __assertfail
    bx lr

    LEAF_END call_stubless_func

    LEAF_ENTRY call_server_func

    /* Unimplemented */
    __assertfail
    bx lr

    LEAF_END call_server_func

#endif

END
