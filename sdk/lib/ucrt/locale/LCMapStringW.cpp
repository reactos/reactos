/***
*w_map.c - W version of LCMapString.
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Wrapper for LCMapStringW.
*
*******************************************************************************/
#include <corecrt_internal.h>

/***
*int __cdecl __acrt_LCMapStringW - Get type information about a wide string.
*
*Purpose:
*       Internal support function. Assumes info in wide string format.
*
*Entry:
*       LPCWSTR  LocaleName  - locale context for the comparison.
*       DWORD    dwMapFlags  - see NT\Chicago docs
*       LPCWSTR  lpSrcStr    - pointer to string to be mapped
*       int      cchSrc      - wide char (word) count of input string
*                              (including nullptr if any)
*                              (-1 if nullptr terminated)
*       LPWSTR   lpDestStr   - pointer to memory to store mapping
*       int      cchDest     - wide char (word) count of buffer (including nullptr)
*
*       NOTE:    if LCMAP_SORTKEY is specified, then cchDest refers to number
*                of BYTES, not number of wide chars. The return string will be
*                a series of bytes with a nullptr byte terminator.
*
*Exit:
*       Success: if LCMAP_SORKEY:
*                   number of bytes written to lpDestStr (including nullptr byte
*                   terminator)
*               else
*                   number of wide characters written to lpDestStr (including
*                   nullptr)
*       Failure: 0
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl __acrt_LCMapStringW(
    LPCWSTR const locale_name,
    DWORD   const map_flags,
    LPCWSTR const source,
    int           source_count,
    LPWSTR  const destination,
    int     const destination_count
    )
{
    // LCMapString will map past the null terminator.  We must find the null
    // terminator if it occurs in the string before source_count characters
    // and cap the number of characters to be considered.
    if (source_count > 0)
    {
        int const source_length = static_cast<int>(wcsnlen(source, source_count));

        // Include the null terminator if the source string is terminated within
        // the buffer.
        if (source_length < source_count)
        {
            source_count = source_length + 1;
        }
        else
        {
            source_count = source_length;
        }
    }

    return __acrt_LCMapStringEx(locale_name, map_flags, source, source_count, destination, destination_count, nullptr, nullptr, 0);
}
