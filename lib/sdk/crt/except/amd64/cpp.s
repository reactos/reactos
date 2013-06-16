

#include <asm.inc>

.code64
.align 4

MACRO(START_VTABLE, shortname, cxxname)
EXTERN shortname&_rtti:PROC
EXTERN MSVCRT_&shortname&_vector_dtor:PROC
    .double shortname&_rtti
PUBLIC MSVCRT_&shortname&_vtable
MSVCRT_&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
    .double MSVCRT_&shortname&_vector_dtor
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN MSVCRT_what_exception:PROC
    .double MSVCRT_what_exception
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
DEFINE_ALIAS ??_V@YAXPEAX@Z, MSVCRT_operator_delete
DEFINE_ALIAS ??2@YAPEAX_K@Z, MSVCRT_operator_new
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

END

