/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x64 C++ V-tables for cpp.c
 * COPYRIGHT:   Copyright 2014-2024 Wine team
 *              Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

// See msvcrt/cpp.c

.const

MACRO(START_VTABLE, shortname, cxxname)
EXTERN shortname&_rtti:PROC
    .quad shortname&_rtti
PUBLIC &shortname&_vtable
&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
ENDM

MACRO(DEFINE_EXCEPTION_VTABLE, shortname, cxxname)
    START_VTABLE shortname, cxxname
    EXTERN &shortname&_vector_dtor:PROC
    .quad &shortname&_vector_dtor
    EXTERN exception_what:ABS
    .quad exception_what
ENDM

START_VTABLE type_info, __dummyname_type_info
    EXTERN type_info_vector_dtor:PROC
    .quad type_info_vector_dtor

DEFINE_EXCEPTION_VTABLE exception, ??_7exception@@6B@

DEFINE_EXCEPTION_VTABLE bad_typeid, ??_7bad_typeid@@6B@
DEFINE_EXCEPTION_VTABLE bad_cast, ??_7bad_cast@@6B@
DEFINE_EXCEPTION_VTABLE __non_rtti_object, ??_7__non_rtti_object@@6B@

#if 0
START_VTABLE exception_old, __dummyname_exception_old
    .quad exception_vector_dtor
    .quad exception_what

START_VTABLE bad_alloc, ??_7bad_alloc@@6B@
    .quad exception_vector_dtor
    .quad exception_what
#endif

END
