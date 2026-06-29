/***
*getpath.c - extract a pathname from an environment variable
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Extract pathnames from a string of semicolon delimited pathnames
*       (generally the value of an environment variable such as PATH).
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <stddef.h>

/***
*_getpath() - extract a pathname from a semicolon-delimited list of pathnames
*
*Purpose:
*       To extract the next pathname from a semicolon-delimited list of
*       pathnames (usually the value on an environment variable) and copy
*       it to a caller-specified buffer. No check is done to see if the path
*       is valid. The maximum number of characters copied to the buffer is
*       maxlen - 1 (and then a '\0' is appended).
*
*ifdef _HPFS_
*       If we hit a quoted string, then allow any characters inside.
*       For example, to put a semi-colon in a path, the user could have
*       an environment variable that looks like:
*
*               PATH=C:\BIN;"D:\CRT\TOOLS;B1";C:\BINP
*endif
*
*       NOTE: Semi-colons in sequence are skipped over; pointers to 0-length
*       pathnames are NOT returned (this includes leading semi-colons).
*
*       NOTE: If this routine is made user-callable, the near attribute
*       must be replaced by _LOAD_DS and the prototype moved from INTERNAL.H
*       to STDLIB.H. The source files MISC\SEARCHEN.C and EXEC\SPAWNVPE.C
*       will need to be recompiled, but should not require any changes.
*
*Entry:
*       src    - Pointer to a string of 0 or more path specificiations,
*                delimited by semicolons (';'), and terminated by a null
*                character
*       dst    - Pointer to the buffer where the next path specification is to
*                be copied
*       maxlen - Maximum number of characters to be copied, counting the
*                terminating null character. Note that a value of 0 is treated
*                as UINT_MAX + 1.
*
*Exit:
*       If a pathname is successfully extracted and copied, a pointer to the
*       first character of next pathname is returned (intervening semicolons
*       are skipped over). If the pathname is too long, as much as possible
*       is copied to the user-specified buffer and nullptr is returned.
*
*       Note that the no check is made of the validity of the copied pathname.
*
*Exceptions:
*
*******************************************************************************/
template <typename Character>
_Success_(return != 0)
static Character* __cdecl common_getpath(
    _In_z_                          Character const*    const   delimited_paths,
    _Out_writes_z_(result_count)    Character*          const   result,
                                    size_t              const   result_count
    ) throw()
{
    _VALIDATE_RETURN_NOEXC(result != nullptr, EINVAL, nullptr);

    if (result_count > 0)
    {
        result[0] = '\0';
    }

    _VALIDATE_RETURN_NOEXC(result_count > 1,  EINVAL, nullptr);

    Character const* source_it = delimited_paths;

    // Skip past any leading semicolons:
    while (*source_it == ';')
    {
        ++source_it;
    }

    Character const* const source_first = source_it;

    Character*       result_it   = result;
    Character* const result_last = result + result_count - 1; // Leave room for \0

#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 Prefast is confused.
    while (*source_it != '\0' && *source_it != ';')
    {
        if (*source_it == '"')
        {
            // We found a quote; copy all characters until we reach the matching
            // end quote or the end of the string:
            ++source_it; // Advance past opening quote

            while (*source_it != '\0' && *source_it != '"')
            {
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015 Prefast is confused.
                *result_it++ = *source_it++;
                if (result_it == result_last)
                {
                    *result_it = '\0';
                    errno = ERANGE;
                    return nullptr;
                }
            }

            if (*source_it != '\0')
            {
                ++source_it; // Advance past closing quote
            }
        }
        else
        {
            *result_it++ = *source_it++;
            if (result_it == result_last)
            {
                *result_it = '\0';
                errno = ERANGE;
                return nullptr;
            }
        }
    }

    // If we copied something and stopped because we reached a semicolon, skip
    // any semicolons before returning:
    while (*source_it == ';')
    {
        ++source_it;
    }

    *result_it = '\0';
    return source_it == source_first
        ? nullptr
        : const_cast<Character*>(source_it);
}

extern "C" char* __cdecl __acrt_getpath(
    char const* const delimited_paths,
    char*       const result,
    size_t      const result_count
    )
{
    return common_getpath(delimited_paths, result, result_count);
}

extern "C" wchar_t* __cdecl __acrt_wgetpath(
    wchar_t const* const delimited_paths,
    wchar_t*       const result,
    size_t         const result_count
    )
{
    return common_getpath(delimited_paths, result, result_count);
}
