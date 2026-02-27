
#if defined(_M_IX86) || defined(_M_AMD64)
#include <asm.inc>
#elif defined(_M_ARM)
#include <kxarm.h>
#endif

#ifdef _M_IX86
.code32

THUNK_ENTRY MACRO num:REQ
    ALIGN 4
    PUBLIC _ObjectStublessClient&num
    _ObjectStublessClient&num:
    mov eax, num
    jmp _call_stubless_func
ENDM

THUNK_ENTRY_VTBL MACRO num:REQ
    ALIGN 4
    PUBLIC _NdrProxyForwardingFunction&num
    _NdrProxyForwardingFunction&num:
    mov eax, [esp + 4]
    mov eax, [eax + 16]
    mov [esp + 4], eax
    mov eax, [eax]
    DB 0ffh, 0a0h /* jmp *offset(%eax) */
    DD 4 * num
ENDM

#elif defined(_M_AMD64)
.code64

THUNK_ENTRY MACRO num:REQ
    ALIGN 4
    PUBLIC ObjectStublessClient&num
    ObjectStublessClient&num:
    mov r10d, num
    jmp call_stubless_func
ENDM

THUNK_ENTRY_VTBL MACRO num:REQ
    ALIGN 4
    PUBLIC NdrProxyForwardingFunction&num
    NdrProxyForwardingFunction&num:
    mov rcx, [rcx + 020h]
    mov rax, [rcx]
    DB 0ffh, 0a0h /* jmp *offset(%rax) */
    DD 8 * num
ENDM

#elif defined(_M_ARM)
.code32

// FIXME: this is probably broken
THUNK_ENTRY MACRO num:REQ
    ldr ip, .number
    b.w call_stubless_func
    .number DB num
ENDM

THUNK_ENTRY_VTBL MACRO num:REQ
    ALIGN 4
    ldr r0, [r0, 10h]
    ldr ip, [r0]
    ldr pc, [ip, 4 * num]
ENDM

#elif defined(_M_ARM64)
.code64

// FIXME: this is probably broken
THUNK_ENTRY MACRO num:REQ
    mov w16, num
    b call_stubless_func
ENDM

THUNK_ENTRY_VTBL MACRO num:REQ
    ldr x0, [x0, 20h]
    ldr x16, [x0]
    mov x17, num
    ldr x16, [x16, x17, lsl 3]
    br x16
ENDM

#endif

ALL_THUNK_ENTRIES MACRO entry
    blk = 3
    REPT 1021
        entry %blk
        blk = blk + 1
    ENDM
ENDM

#ifdef _M_IX86
EXTERN _call_stubless_func:PROC
PUBLIC _stubless_thunks
_stubless_thunks:
    ALL_THUNK_ENTRIES THUNK_ENTRY
PUBLIC _vtbl_thunks
_vtbl_thunks:
    ALL_THUNK_ENTRIES THUNK_ENTRY_VTBL
#else
EXTERN call_stubless_func:PROC
PUBLIC stubless_thunks
stubless_thunks:
    ALL_THUNK_ENTRIES THUNK_ENTRY
PUBLIC vtbl_thunks
vtbl_thunks:
    ALL_THUNK_ENTRIES THUNK_ENTRY_VTBL
#endif

END
