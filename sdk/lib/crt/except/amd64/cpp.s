

#include <asm.inc>

.code64
.align 4

MACRO(START_VTABLE, shortname, cxxname)
EXTERN shortname&_rtti:PROC
EXTERN MSVCRT_&shortname&_vector_dtor:PROC
    .quad shortname&_rtti
PUBLIC MSVCRT_&shortname&_vtable
MSVCRT_&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
    .quad MSVCRT_&shortname&_vector_dtor
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN MSVCRT_what_exception:ABS
    .quad MSVCRT_what_exception
ENDM

START_VTABLE type_info, __dummyname_type_info
DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@


MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig:ABS
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ??3@YAXPEAX@Z, MSVCRT_operator_delete
DEFINE_ALIAS ??_U@YAPEAX_K@Z, MSVCRT_operator_new
DEFINE_ALIAS ??_U@YAPEAX_KHPEBDH@Z, MSVCRT_operator_new_dbg
DEFINE_ALIAS ??_V@YAXPEAX@Z, MSVCRT_operator_delete
DEFINE_ALIAS ??2@YAPEAX_K@Z, MSVCRT_operator_new
DEFINE_ALIAS ??2@YAPEAX_KHPEBDH@Z, MSVCRT_operator_new_dbg
DEFINE_ALIAS ?_query_new_handler@@YAP6AHI@ZXZ, MSVCRT__query_new_handler
DEFINE_ALIAS ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z, MSVCRT__set_new_handler
DEFINE_ALIAS ?set_new_handler@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_new_handler
DEFINE_ALIAS ?_query_new_mode@@YAHXZ, MSVCRT__query_new_mode
DEFINE_ALIAS ?_set_new_mode@@YAHH@Z, MSVCRT__set_new_mode
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, MSVCRT__set_se_translator
DEFINE_ALIAS ?set_terminate@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_terminate
DEFINE_ALIAS ?set_unexpected@@YAP6AXXZP6AXXZ@Z, MSVCRT_set_unexpected
DEFINE_ALIAS ?terminate@@YAXXZ, MSVCRT_terminate
DEFINE_ALIAS ?unexpected@@YAXXZ, MSVCRT_unexpected
DEFINE_ALIAS ?what@exception@@UEBAPEBDXZ, MSVCRT_what_exception
DEFINE_ALIAS ??0exception@@QEAA@AEBQEBDH@Z, MSVCRT_exception_ctor_noalloc
DEFINE_ALIAS ??0exception@@QEAA@AEBV0@@Z, MSVCRT_exception_copy_ctor
DEFINE_ALIAS ??0exception@@QEAA@XZ, MSVCRT_exception_default_ctor
DEFINE_ALIAS ??1exception@@UEAA@XZ, MSVCRT_exception_dtor
DEFINE_ALIAS ??4exception@@QEAAAEAV0@AEBV0@@Z, MSVCRT_exception_opequals
DEFINE_ALIAS ??1type_info@@UEAA@XZ, MSVCRT_type_info_dtor
DEFINE_ALIAS ??0__non_rtti_object@@QEAA@AEBV0@@Z, MSVCRT___non_rtti_object_copy_ctor
DEFINE_ALIAS ??0__non_rtti_object@@QEAA@PEBD@Z, MSVCRT___non_rtti_object_ctor
DEFINE_ALIAS ??0bad_cast@@AAE@PBQBD@Z, MSVCRT_bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@AEAA@PEBQEBD@Z, MSVCRT_bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QAE@ABQBD@Z, MSVCRT_bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@AEBQEBD@Z, MSVCRT_bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@AEBV0@@Z, MSVCRT_bad_cast_copy_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@PEBD@Z, MSVCRT_bad_cast_ctor_charptr
DEFINE_ALIAS ??0bad_typeid@@QEAA@AEBV0@@Z, MSVCRT_bad_typeid_copy_ctor
DEFINE_ALIAS ??0bad_typeid@@QEAA@PEBD@Z, MSVCRT_bad_typeid_ctor
DEFINE_ALIAS ??0exception@@QEAA@AEBQEBD@Z, MSVCRT_exception_ctor
DEFINE_ALIAS ??1__non_rtti_object@@UEAA@XZ, MSVCRT___non_rtti_object_dtor
DEFINE_ALIAS ??1bad_cast@@UEAA@XZ, MSVCRT_bad_cast_dtor
DEFINE_ALIAS ??1bad_typeid@@UEAA@XZ, MSVCRT_bad_typeid_dtor
DEFINE_ALIAS ??4bad_cast@@QEAAAEAV0@AEBV0@@Z, MSVCRT_bad_cast_opequals
DEFINE_ALIAS ??4bad_typeid@@QEAAAEAV0@AEBV0@@Z, MSVCRT_bad_typeid_opequals
DEFINE_ALIAS ??8type_info@@QEBAHAEBV0@@Z, MSVCRT_type_info_opequals_equals
DEFINE_ALIAS ??9type_info@@QEBAHAEBV0@@Z, MSVCRT_type_info_opnot_equals
DEFINE_ALIAS ??_Fbad_cast@@QEAAXXZ, MSVCRT_bad_cast_default_ctor
DEFINE_ALIAS ??_Fbad_typeid@@QEAAXXZ, MSVCRT_bad_typeid_default_ctor
DEFINE_ALIAS ?_query_new_handler@@YAP6AH_K@ZXZ, MSVCRT__query_new_handler
DEFINE_ALIAS ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z, MSVCRT__set_new_handler
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, MSVCRT__set_se_translator
DEFINE_ALIAS ?before@type_info@@QEBAHAEBV1@@Z, MSVCRT_type_info_before
DEFINE_ALIAS ?name@type_info@@QEBAPEBDXZ, MSVCRT_type_info_name
DEFINE_ALIAS ?raw_name@type_info@@QEBAPEBDXZ, MSVCRT_type_info_raw_name
DEFINE_ALIAS ??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z, MSVCRT___non_rtti_object_opequals

END

