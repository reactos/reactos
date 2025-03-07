

#include <asm.inc>

.code
.align 4

EXTERN _cxx_frame_handler:PROC

#undef _MSVCRT_
MACRO(START_VTABLE, shortname, cxxname)
EXTERN _&shortname&_rtti:PROC
EXTERN ___thiscall_&shortname&_vector_dtor:PROC
    .long _&shortname&_rtti
PUBLIC _&shortname&_vtable
_&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
    .long ___thiscall_&shortname&_vector_dtor
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN ___thiscall_exception_what:PROC
    .long ___thiscall_exception_what
ENDM

START_VTABLE type_info, __dummyname_type_info
DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

// void call_copy_ctor( void *func, void *this, void *src, int has_vbase );
PUBLIC _call_copy_ctor
_call_copy_ctor:
    push ebp
    CFI_ADJUST_CFA_OFFSET 4
    CFI_REL_OFFSET ebp, 0
    mov ebp, esp
    CFI_DEF_CFA_REGISTER ebp
    push 1
    mov ecx, [ebp + 12]
    push dword ptr [ebp + 16]
    call dword ptr [ebp + 8]
    leave
    CFI_DEF_CFA esp, 4
    CFI_SAME_VALUE ebp
    ret

// void DECLSPEC_NORETURN continue_after_catch( cxx_exception_frame* frame, void *addr );
PUBLIC _continue_after_catch
_continue_after_catch:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    mov esp, [edx - 4]
    lea ebp, [edx + 12]
    jmp eax

// void DECLSPEC_NORETURN call_finally_block( void *code_block, void *base_ptr );
PUBLIC _call_finally_block
_call_finally_block:
    mov ebp, [esp +8]
    jmp dword ptr [esp + 4]

// int call_filter( int (*func)(PEXCEPTION_POINTERS), void *arg, void *ebp );
PUBLIC _call_filter
_call_filter:
    push ebp
    push [esp + 12]
    mov ebp, [esp + 20]
    call dword ptr [esp + 12]
    pop ebp
    pop ebp
    ret

// void *call_handler( void * (*func)(void), void *ebp );
PUBLIC _call_handler
_call_handler:
    push ebp
    push ebx
    push esi
    push edi
    mov ebp, [esp + 24]
    call dword ptr [esp + 20]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret


PUBLIC ___CxxFrameHandler
___CxxFrameHandler:
    push 0
    push 0
    push eax
    push[esp + 28]
    push[esp + 28]
    push[esp + 28]
    push[esp + 28]
    call _cxx_frame_handler
    add esp, 28
    ret

END

