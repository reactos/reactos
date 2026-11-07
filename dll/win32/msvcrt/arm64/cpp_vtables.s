/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ARM64 C++ V-tables for cpp.c
 * COPYRIGHT:   Copyright 2014-2024 Wine team
 *              Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

// See msvcrt/cpp.c, each vtable is preceded by its RTTI pointer

    IMPORT type_info_rtti
    IMPORT type_info_vector_dtor
    IMPORT exception_rtti
    IMPORT exception_vector_dtor
    IMPORT bad_typeid_rtti
    IMPORT bad_typeid_vector_dtor
    IMPORT bad_cast_rtti
    IMPORT bad_cast_vector_dtor
    IMPORT __non_rtti_object_rtti
    IMPORT __non_rtti_object_vector_dtor
    IMPORT exception_what

    RODATAAREA

    ALIGN 8
    dcq type_info_rtti
    EXPORT type_info_vtable
LABEL(type_info_vtable)
    dcq type_info_vector_dtor

    dcq exception_rtti
    EXPORT exception_vtable
LABEL(exception_vtable)
    EXPORT MANGLED(??_7exception@@6B@)
LABEL(MANGLED(??_7exception@@6B@))
    dcq exception_vector_dtor
    dcq exception_what

    dcq bad_typeid_rtti
    EXPORT bad_typeid_vtable
LABEL(bad_typeid_vtable)
    EXPORT MANGLED(??_7bad_typeid@@6B@)
LABEL(MANGLED(??_7bad_typeid@@6B@))
    dcq bad_typeid_vector_dtor
    dcq exception_what

    dcq bad_cast_rtti
    EXPORT bad_cast_vtable
LABEL(bad_cast_vtable)
    EXPORT MANGLED(??_7bad_cast@@6B@)
LABEL(MANGLED(??_7bad_cast@@6B@))
    dcq bad_cast_vector_dtor
    dcq exception_what

    dcq __non_rtti_object_rtti
    EXPORT __non_rtti_object_vtable
LABEL(__non_rtti_object_vtable)
    EXPORT MANGLED(??_7__non_rtti_object@@6B@)
LABEL(MANGLED(??_7__non_rtti_object@@6B@))
    dcq __non_rtti_object_vector_dtor
    dcq exception_what

    END
