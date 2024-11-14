

#include <asm.inc>

.code64
.align 4

MACRO(START_VTABLE, shortname, cxxname)
EXTERN shortname&_rtti:PROC
EXTERN &shortname&_vector_dtor:PROC
    .quad shortname&_rtti
PUBLIC &shortname&_vtable
&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
    .quad &shortname&_vector_dtor
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN exception_what:ABS
    .quad exception_what
ENDM

START_VTABLE type_info, __dummyname_type_info
DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@
DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

END
