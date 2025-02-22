/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Instantiation of CRT stdio inline functions
 * COPYRIGHT:   Copyright 2023-2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _NO_CRT_STDIO_INLINE
#include <stdio.h>

// These 2 functions are inlined in UCRT headers, but they are referenced by
// external code in the legacy CRT, so we need to provide them as non-inline
// functions here.

_Success_(return >= 0)
_Check_return_opt_
int __CRTDECL sprintf(
    _Pre_notnull_ _Always_(_Post_z_) char*       const _Buffer,
    _In_z_ _Printf_format_string_    char const* const _Format,
    ...)
{
    int _Result;
    va_list _ArgList;

    __crt_va_start(_ArgList, _Format);

    _Result = __stdio_common_vsprintf(
        _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
        _Buffer, (size_t)-1, _Format, NULL, _ArgList);

    __crt_va_end(_ArgList);
    return _Result < 0 ? -1 : _Result;
}

_Success_(return >= 0)
_Check_return_opt_
int __CRTDECL _vsnprintf(
    _Out_writes_opt_(_BufferCount) _Post_maybez_ char*       const _Buffer,
    _In_                                        size_t      const _BufferCount,
    _In_z_ _Printf_format_string_               char const* const _Format,
                                                va_list           _ArgList
    )
{
    int _Result = __stdio_common_vsprintf(
        _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
        _Buffer, _BufferCount, _Format, NULL, _ArgList);

    return _Result < 0 ? -1 : _Result;
}
