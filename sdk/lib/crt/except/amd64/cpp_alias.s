

#include <asm.inc>

.code64
.align 4

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig:ABS
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ??3@YAXPEAX@Z, operator_delete
DEFINE_ALIAS ??_U@YAPEAX_K@Z, operator_new
DEFINE_ALIAS ??_U@YAPEAX_KHPEBDH@Z, operator_new_dbg
DEFINE_ALIAS ??_V@YAXPEAX@Z, operator_delete
DEFINE_ALIAS ??2@YAPEAX_K@Z, operator_new
DEFINE_ALIAS ??2@YAPEAX_KHPEBDH@Z, operator_new_dbg
DEFINE_ALIAS ?_query_new_handler@@YAP6AHI@ZXZ, _query_new_handler
DEFINE_ALIAS ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z, _set_new_handler
DEFINE_ALIAS ?set_new_handler@@YAP6AXXZP6AXXZ@Z, set_new_handler
DEFINE_ALIAS ?_query_new_mode@@YAHXZ, _query_new_mode
DEFINE_ALIAS ?_set_new_mode@@YAHH@Z, _set_new_mode
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, _set_se_translator
DEFINE_ALIAS ?set_terminate@@YAP6AXXZP6AXXZ@Z, set_terminate
DEFINE_ALIAS ?set_unexpected@@YAP6AXXZP6AXXZ@Z, set_unexpected
DEFINE_ALIAS ?terminate@@YAXXZ, terminate
DEFINE_ALIAS ?unexpected@@YAXXZ, unexpected
DEFINE_ALIAS ?what@exception@@UEBAPEBDXZ, exception_what
DEFINE_ALIAS ??0exception@@QEAA@AEBQEBDH@Z, exception_ctor_noalloc
DEFINE_ALIAS ??0exception@@QEAA@AEBV0@@Z, exception_copy_ctor
DEFINE_ALIAS ??0exception@@QEAA@XZ, exception_default_ctor
DEFINE_ALIAS ??1exception@@UEAA@XZ, exception_dtor
DEFINE_ALIAS ??4exception@@QEAAAEAV0@AEBV0@@Z, exception_opequals
DEFINE_ALIAS ??1type_info@@UEAA@XZ, type_info_dtor
DEFINE_ALIAS ??0__non_rtti_object@@QEAA@AEBV0@@Z, __non_rtti_object_copy_ctor
DEFINE_ALIAS ??0__non_rtti_object@@QEAA@PEBD@Z, __non_rtti_object_ctor
DEFINE_ALIAS ??0bad_cast@@AAE@PBQBD@Z, bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@AEAA@PEBQEBD@Z, bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QAE@ABQBD@Z, bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@AEBQEBD@Z, bad_cast_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@AEBV0@@Z, bad_cast_copy_ctor
DEFINE_ALIAS ??0bad_cast@@QEAA@PEBD@Z, bad_cast_ctor_charptr
DEFINE_ALIAS ??0bad_typeid@@QEAA@AEBV0@@Z, bad_typeid_copy_ctor
DEFINE_ALIAS ??0bad_typeid@@QEAA@PEBD@Z, bad_typeid_ctor
DEFINE_ALIAS ??0exception@@QEAA@AEBQEBD@Z, exception_ctor
DEFINE_ALIAS ??1__non_rtti_object@@UEAA@XZ, __non_rtti_object_dtor
DEFINE_ALIAS ??1bad_cast@@UEAA@XZ, bad_cast_dtor
DEFINE_ALIAS ??1bad_typeid@@UEAA@XZ, bad_typeid_dtor
DEFINE_ALIAS ??4bad_cast@@QEAAAEAV0@AEBV0@@Z, bad_cast_opequals
DEFINE_ALIAS ??4bad_typeid@@QEAAAEAV0@AEBV0@@Z, bad_typeid_opequals
DEFINE_ALIAS ??8type_info@@QEBAHAEBV0@@Z, type_info_opequals_equals
DEFINE_ALIAS ??9type_info@@QEBAHAEBV0@@Z, type_info_opnot_equals
DEFINE_ALIAS ??_Fbad_cast@@QEAAXXZ, bad_cast_default_ctor
DEFINE_ALIAS ??_Fbad_typeid@@QEAAXXZ, bad_typeid_default_ctor
DEFINE_ALIAS ?_query_new_handler@@YAP6AH_K@ZXZ, _query_new_handler
DEFINE_ALIAS ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z, _set_new_handler
DEFINE_ALIAS ?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z, _set_se_translator
DEFINE_ALIAS ?before@type_info@@QEBAHAEBV1@@Z, type_info_before
DEFINE_ALIAS ?name@type_info@@QEBAPEBDXZ, type_info_name
DEFINE_ALIAS ?raw_name@type_info@@QEBAPEBDXZ, type_info_raw_name
DEFINE_ALIAS ??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z, __non_rtti_object_opequals

END
