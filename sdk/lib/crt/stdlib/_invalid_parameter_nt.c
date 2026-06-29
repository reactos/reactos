/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _invalid_parameter
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <ndk/rtlfuncs.h>

void
__cdecl
_invalid_parameter(
    _In_opt_z_ wchar_t const* expression,
    _In_opt_z_ wchar_t const* function_name,
    _In_opt_z_ wchar_t const* file_name,
    _In_ unsigned int line_number,
    _In_ uintptr_t reserved)
{
    DbgPrint("%ws:%u: Invalid parameter ('%ws') passed to C runtime function %ws.\n",
             file_name,
             line_number,
             expression,
             function_name);
}
