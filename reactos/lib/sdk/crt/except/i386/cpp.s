

#include <asm.inc>

.code
.align 4

MACRO(DEFINE_THISCALL_WRAPPER, cxxname, stdcallname)
EXTERN &stdcallname:PROC
PUBLIC &cxxname
&cxxname:
    pop eax
    push ecx
    push eax
    jmp &stdcallname
ENDM

DEFINE_THISCALL_WRAPPER ??0exception@@QAE@ABQBD@Z, _MSVCRT_exception_ctor@8
DEFINE_THISCALL_WRAPPER ??0exception@@QAE@ABQBDH@Z, _MSVCRT_exception_ctor_noalloc@12
DEFINE_THISCALL_WRAPPER ??0exception@@QAE@ABV0@@Z, _MSVCRT_exception_copy_ctor@8
DEFINE_THISCALL_WRAPPER ??0exception@@QAE@XZ, _MSVCRT_exception_default_ctor@4
DEFINE_THISCALL_WRAPPER ??1exception@@UAE@XZ, _MSVCRT_exception_dtor@4
DEFINE_THISCALL_WRAPPER ??4exception@@QAEAAV0@ABV0@@Z, _MSVCRT_exception_opequals@8
DEFINE_THISCALL_WRAPPER ??_Eexception@@UAEPAXI@Z, _MSVCRT_exception_vector_dtor@8
DEFINE_THISCALL_WRAPPER ??_Gexception@@UAEPAXI@Z, _MSVCRT_exception_scalar_dtor@8
DEFINE_THISCALL_WRAPPER ?what@exception@@UBEPBDXZ, _MSVCRT_what_exception@4
DEFINE_THISCALL_WRAPPER ??0bad_typeid@@QAE@ABV0@@Z, _MSVCRT_bad_typeid_copy_ctor@8
DEFINE_THISCALL_WRAPPER ??0bad_typeid@@QAE@PBD@Z, _MSVCRT_bad_typeid_ctor@8
DEFINE_THISCALL_WRAPPER ??_Fbad_typeid@@QAEXXZ, _MSVCRT_bad_typeid_default_ctor@4
DEFINE_THISCALL_WRAPPER ??1bad_typeid@@UAE@XZ, _MSVCRT_bad_typeid_dtor@4
DEFINE_THISCALL_WRAPPER ??4bad_typeid@@QAEAAV0@ABV0@@Z, _MSVCRT_bad_typeid_opequals@8
DEFINE_THISCALL_WRAPPER ??_Ebad_typeid@@UAEPAXI@Z, _MSVCRT_bad_typeid_vector_dtor@8
DEFINE_THISCALL_WRAPPER ??_Gbad_typeid@@UAEPAXI@Z, _MSVCRT_bad_typeid_scalar_dtor@8
DEFINE_THISCALL_WRAPPER ??0__non_rtti_object@@QAE@ABV0@@Z, _MSVCRT___non_rtti_object_copy_ctor@8
DEFINE_THISCALL_WRAPPER ??0__non_rtti_object@@QAE@PBD@Z, _MSVCRT___non_rtti_object_ctor@8
DEFINE_THISCALL_WRAPPER ??1__non_rtti_object@@UAE@XZ, _MSVCRT___non_rtti_object_dtor@4
DEFINE_THISCALL_WRAPPER ??4__non_rtti_object@@QAEAAV0@ABV0@@Z, _MSVCRT___non_rtti_object_opequals@8
DEFINE_THISCALL_WRAPPER ??_E__non_rtti_object@@UAEPAXI@Z, _MSVCRT___non_rtti_object_vector_dtor@8
DEFINE_THISCALL_WRAPPER ??_G__non_rtti_object@@UAEPAXI@Z, _MSVCRT___non_rtti_object_scalar_dtor@8
DEFINE_THISCALL_WRAPPER ??0bad_cast@@AAE@PBQBD@Z, _MSVCRT_bad_cast_ctor@8
DEFINE_THISCALL_WRAPPER ??0bad_cast@@QAE@ABQBD@Z, _MSVCRT_bad_cast_ctor@8
DEFINE_THISCALL_WRAPPER ??0bad_cast@@QAE@ABV0@@Z, _MSVCRT_bad_cast_copy_ctor@8
DEFINE_THISCALL_WRAPPER ??0bad_cast@@QAE@PBD@Z, _MSVCRT_bad_cast_ctor_charptr@8
DEFINE_THISCALL_WRAPPER ??_Fbad_cast@@QAEXXZ, _MSVCRT_bad_cast_default_ctor@4
DEFINE_THISCALL_WRAPPER ??1bad_cast@@UAE@XZ, _MSVCRT_bad_cast_dtor@4
DEFINE_THISCALL_WRAPPER ??4bad_cast@@QAEAAV0@ABV0@@Z, _MSVCRT_bad_cast_opequals@8
DEFINE_THISCALL_WRAPPER ??_Ebad_cast@@UAEPAXI@Z, _MSVCRT_bad_cast_vector_dtor@8
DEFINE_THISCALL_WRAPPER ??_Gbad_cast@@UAEPAXI@Z, _MSVCRT_bad_cast_scalar_dtor@8
DEFINE_THISCALL_WRAPPER ??8type_info@@QBEHABV0@@Z, _MSVCRT_type_info_opequals_equals@8
DEFINE_THISCALL_WRAPPER ??9type_info@@QBEHABV0@@Z, _MSVCRT_type_info_opnot_equals@8
DEFINE_THISCALL_WRAPPER ?before@type_info@@QBEHABV1@@Z, _MSVCRT_type_info_before@8
DEFINE_THISCALL_WRAPPER ??1type_info@@UAE@XZ, _MSVCRT_type_info_dtor@4
DEFINE_THISCALL_WRAPPER ?name@type_info@@QBEPBDXZ, _MSVCRT_type_info_name@4
DEFINE_THISCALL_WRAPPER ?raw_name@type_info@@QBEPBDXZ, _MSVCRT_type_info_raw_name@4


#undef _MSVCRT_
MACRO(START_VTABLE, shortname, cxxname)
EXTERN _&shortname&_rtti:PROC
EXTERN ___thiscall_MSVCRT_&shortname&_vector_dtor:PROC
    .long _&shortname&_rtti
PUBLIC _MSVCRT_&shortname&_vtable
_MSVCRT_&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
    .long ___thiscall_MSVCRT_&shortname&_vector_dtor
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN ___thiscall_MSVCRT_what_exception:PROC
    .long ___thiscall_MSVCRT_what_exception
ENDM

START_VTABLE type_info, __dummyname_type_info
DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

EXTERN _MSVCRT_operator_delete:PROC
PUBLIC ??3@YAXPAX@Z
??3@YAXPAX@Z:
    jmp _MSVCRT_operator_delete

EXTERN _MSVCRT_operator_new:PROC
PUBLIC ??_U@YAPAXI@Z
??_U@YAPAXI@Z:
    jmp _MSVCRT_operator_new


MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ??_V@YAXPAX@Z, _MSVCRT_operator_delete, PROC
DEFINE_ALIAS ??2@YAPAXI@Z, _MSVCRT_operator_new, PROC
DEFINE_ALIAS ?_query_new_handler@@YAP6AHI@ZXZ, _MSVCRT__query_new_handler, PROC
DEFINE_ALIAS ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z, _MSVCRT__set_new_handler, PROC
DEFINE_ALIAS ?set_new_handler@@YAP6AXXZP6AXXZ@Z, _MSVCRT_set_new_handler, PROC
DEFINE_ALIAS ?_query_new_mode@@YAHXZ, _MSVCRT__query_new_mode, PROC
DEFINE_ALIAS ?_set_new_mode@@YAHH@Z, _MSVCRT__set_new_mode, PROC
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, _MSVCRT__set_se_translator, PROC
DEFINE_ALIAS ?set_terminate@@YAP6AXXZP6AXXZ@Z, _MSVCRT_set_terminate, PROC
DEFINE_ALIAS ?set_unexpected@@YAP6AXXZP6AXXZ@Z, _MSVCRT_set_unexpected, PROC
DEFINE_ALIAS ?terminate@@YAXXZ, _MSVCRT_terminate, PROC
DEFINE_ALIAS ?unexpected@@YAXXZ, _MSVCRT_unexpected, PROC


END

