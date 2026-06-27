/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * mbsrtowcs compatibility shim for MinGW-w64 support libraries on pre-NT6 CRTs.
 */

#include <errno.h>
#include <stdlib.h>
#include <wchar.h>

size_t __cdecl
mbsrtowcs(wchar_t *Destination, const char **Source, size_t Count, mbstate_t *State)
{
    const char *Current;
    size_t Converted = 0;

    if (Source == NULL)
    {
        errno = EINVAL;
        return (size_t)-1;
    }

    Current = *Source;
    if (Current == NULL)
        return 0;

    if (State != NULL)
        *State = 0;

    while (Destination == NULL || Converted < Count)
    {
        wchar_t WideCharacter = 0;
        int CharacterLength;

        CharacterLength = mbtowc(&WideCharacter, Current, MB_CUR_MAX);
        if (CharacterLength < 0)
        {
            errno = EILSEQ;
            return (size_t)-1;
        }

        if (CharacterLength == 0)
        {
            if (Destination != NULL)
                *Source = NULL;

            return Converted;
        }

        if (Destination != NULL)
            Destination[Converted] = WideCharacter;

        Current += CharacterLength;
        Converted++;
    }

    if (Destination != NULL)
        *Source = Current;

    return Converted;
}

#ifdef _M_IX86
void* _imp__mbsrtowcs = mbsrtowcs;
#else
void* __imp_mbsrtowcs = mbsrtowcs;
#endif
