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
    DCQ type_info_rtti
    EXPORT type_info_vtable
type_info_vtable
    DCQ type_info_vector_dtor

    DCQ exception_rtti
    EXPORT exception_vtable
    EXPORT |??_7exception@@6B@|
exception_vtable
|??_7exception@@6B@|
    DCQ exception_vector_dtor
    DCQ exception_what

    DCQ bad_typeid_rtti
    EXPORT bad_typeid_vtable
    EXPORT |??_7bad_typeid@@6B@|
bad_typeid_vtable
|??_7bad_typeid@@6B@|
    DCQ bad_typeid_vector_dtor
    DCQ exception_what

    DCQ bad_cast_rtti
    EXPORT bad_cast_vtable
    EXPORT |??_7bad_cast@@6B@|
bad_cast_vtable
|??_7bad_cast@@6B@|
    DCQ bad_cast_vector_dtor
    DCQ exception_what

    DCQ __non_rtti_object_rtti
    EXPORT __non_rtti_object_vtable
    EXPORT |??_7__non_rtti_object@@6B@|
__non_rtti_object_vtable
|??_7__non_rtti_object@@6B@|
    DCQ __non_rtti_object_vector_dtor
    DCQ exception_what

    END
