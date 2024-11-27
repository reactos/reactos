

#include <asm.inc>

.code
.align 4

EXTERN _cxx_frame_handler:PROC

MACRO(DEFINE_THISCALL_ALIAS, cxxname, target)
#ifdef _USE_ML
    EXTERN ___thiscall&target:PROC
    ALIAS <&cxxname> = <___thiscall&target>
#else
    PUBLIC cxxname
    .weakref &cxxname, &target
#endif
ENDM

DEFINE_THISCALL_ALIAS ??0exception@@QAE@ABQBD@Z, _exception_ctor
DEFINE_THISCALL_ALIAS ??0exception@@QAE@ABQBDH@Z, _exception_ctor_noalloc
DEFINE_THISCALL_ALIAS ??0exception@@QAE@ABV0@@Z, _exception_copy_ctor
DEFINE_THISCALL_ALIAS ??0exception@@QAE@XZ, _exception_default_ctor
DEFINE_THISCALL_ALIAS ??1exception@@UAE@XZ, _exception_dtor
DEFINE_THISCALL_ALIAS ??4exception@@QAEAAV0@ABV0@@Z, _exception_opequals
DEFINE_THISCALL_ALIAS ??_Eexception@@UAEPAXI@Z, _exception_vector_dtor
DEFINE_THISCALL_ALIAS ??_Gexception@@UAEPAXI@Z, _exception_scalar_dtor
DEFINE_THISCALL_ALIAS ?what@exception@@UBEPBDXZ, _exception_what
DEFINE_THISCALL_ALIAS ??0bad_typeid@@QAE@ABV0@@Z, _bad_typeid_copy_ctor
DEFINE_THISCALL_ALIAS ??0bad_typeid@@QAE@PBD@Z, _bad_typeid_ctor
DEFINE_THISCALL_ALIAS ??_Fbad_typeid@@QAEXXZ, _bad_typeid_default_ctor
DEFINE_THISCALL_ALIAS ??1bad_typeid@@UAE@XZ, _bad_typeid_dtor
DEFINE_THISCALL_ALIAS ??4bad_typeid@@QAEAAV0@ABV0@@Z, _bad_typeid_opequals
DEFINE_THISCALL_ALIAS ??_Ebad_typeid@@UAEPAXI@Z, _bad_typeid_vector_dtor
DEFINE_THISCALL_ALIAS ??_Gbad_typeid@@UAEPAXI@Z, _bad_typeid_scalar_dtor
DEFINE_THISCALL_ALIAS ??0__non_rtti_object@@QAE@ABV0@@Z, ___non_rtti_object_copy_ctor
DEFINE_THISCALL_ALIAS ??0__non_rtti_object@@QAE@PBD@Z, ___non_rtti_object_ctor
DEFINE_THISCALL_ALIAS ??1__non_rtti_object@@UAE@XZ, ___non_rtti_object_dtor
DEFINE_THISCALL_ALIAS ??4__non_rtti_object@@QAEAAV0@ABV0@@Z, ___non_rtti_object_opequals
DEFINE_THISCALL_ALIAS ??_E__non_rtti_object@@UAEPAXI@Z, ___non_rtti_object_vector_dtor
DEFINE_THISCALL_ALIAS ??_G__non_rtti_object@@UAEPAXI@Z, ___non_rtti_object_scalar_dtor
DEFINE_THISCALL_ALIAS ??0bad_cast@@AAE@PBQBD@Z, _bad_cast_ctor
DEFINE_THISCALL_ALIAS ??0bad_cast@@QAE@ABQBD@Z, _bad_cast_ctor
DEFINE_THISCALL_ALIAS ??0bad_cast@@QAE@ABV0@@Z, _bad_cast_copy_ctor
DEFINE_THISCALL_ALIAS ??0bad_cast@@QAE@PBD@Z, _bad_cast_ctor_charptr
DEFINE_THISCALL_ALIAS ??_Fbad_cast@@QAEXXZ, _bad_cast_default_ctor
DEFINE_THISCALL_ALIAS ??1bad_cast@@UAE@XZ, _bad_cast_dtor
DEFINE_THISCALL_ALIAS ??4bad_cast@@QAEAAV0@ABV0@@Z, _bad_cast_opequals
DEFINE_THISCALL_ALIAS ??_Ebad_cast@@UAEPAXI@Z, _bad_cast_vector_dtor
DEFINE_THISCALL_ALIAS ??_Gbad_cast@@UAEPAXI@Z, _bad_cast_scalar_dtor
DEFINE_THISCALL_ALIAS ??8type_info@@QBEHABV0@@Z, _type_info_opequals_equals
DEFINE_THISCALL_ALIAS ??9type_info@@QBEHABV0@@Z, _type_info_opnot_equals
DEFINE_THISCALL_ALIAS ?before@type_info@@QBEHABV1@@Z, _type_info_before
DEFINE_THISCALL_ALIAS ??1type_info@@UAE@XZ, _type_info_dtor
DEFINE_THISCALL_ALIAS ?name@type_info@@QBEPBDXZ, _type_info_name
DEFINE_THISCALL_ALIAS ?raw_name@type_info@@QBEPBDXZ, _type_info_raw_name


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

EXTERN _operator_delete:PROC
PUBLIC ??3@YAXPAX@Z
??3@YAXPAX@Z:
    jmp _operator_delete

EXTERN _operator_new:PROC
PUBLIC ??_U@YAPAXI@Z
??_U@YAPAXI@Z:
    jmp _operator_new


MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ??_V@YAXPAX@Z, _operator_delete, PROC
DEFINE_ALIAS ??2@YAPAXI@Z, _operator_new, PROC
DEFINE_ALIAS ?_query_new_handler@@YAP6AHI@ZXZ, __query_new_handler, PROC
DEFINE_ALIAS ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z, __set_new_handler, PROC
DEFINE_ALIAS ?set_new_handler@@YAP6AXXZP6AXXZ@Z, _set_new_handler, PROC
DEFINE_ALIAS ?_query_new_mode@@YAHXZ, __query_new_mode, PROC
DEFINE_ALIAS ?_set_new_mode@@YAHH@Z, __set_new_mode, PROC
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, __set_se_translator, PROC
DEFINE_ALIAS ?set_terminate@@YAP6AXXZP6AXXZ@Z, _set_terminate, PROC
DEFINE_ALIAS ?set_unexpected@@YAP6AXXZP6AXXZ@Z, _set_unexpected, PROC
DEFINE_ALIAS ?terminate@@YAXXZ, _terminate, PROC
DEFINE_ALIAS ?unexpected@@YAXXZ, _unexpected, PROC


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

