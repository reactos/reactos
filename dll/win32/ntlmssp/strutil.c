/*
 * PROJECT:     ReactOS strutil (ntlmssp)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     provides string-functions similar to RtlString-functions
 *              however it provies a variable alloc/free function.
 *              so its useful in sspi where it can be user or lsa mode.
 *              (if useful make a lib or something ...)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */

#include <stdlib.h>
#include <ntlmssp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

_strutil_alloc_proc _strutil_alloc = malloc;
_strutil_free_proc _strutil_free = free;
//TODO: realloc (to improve speed for concat)
BOOL _strutil_is_custom = FALSE;

#define ALLOC_BLOCK 16

BOOL
init_strutil(
    IN _strutil_alloc_proc ap,
    IN _strutil_free_proc fp)
{
    /* set memmory manager once and dont change
     * it while running ... */
    if (_strutil_is_custom)
    {
        WARN("init_strutil was called more then once!\n");
        return FALSE;
    }
    _strutil_alloc = ap;
    _strutil_free = fp;
    _strutil_is_custom = TRUE;
    return TRUE;
}

void ExtWStrInit(
    IN PEXT_STRING dst,
    IN WCHAR* initstr)
{
    if (initstr == NULL)
    {
        dst->bUsed = 0;
        dst->bAllocated = 0;
        dst->Buffer = NULL;
        dst->typ = stUnicodeStr;
        return;
    }
}

void ExtAStrInit(
    IN PEXT_STRING dst,
    IN char* initstr)
{
    if (initstr == NULL)
    {
        dst->bUsed = 0;
        dst->bAllocated = 0;
        dst->Buffer = NULL;
        dst->typ = stAnsiStr;
        return;
    }
}

void
ExtStrFree(
    IN PEXT_STRING s)
{
    if (s->bAllocated == 0)
        return;
    s->bUsed = 0;
    _strutil_free(s->Buffer);
    s->bAllocated = 0;
    s->Buffer = NULL;
}

/* internal - does not shrink buffer */
BOOL
_ExtStrRealloc(
    IN PEXT_STRING dest,
    size_t bNeeded,
    BOOL copyBuffer)
{
    ULONG bNewAlloc;
    PBYTE newBuffer;

    if (bNeeded <= dest->bAllocated)
        return TRUE;

    /* align tO ALLOC_BLOCK byte */
    bNewAlloc = (((bNeeded - 1) / ALLOC_BLOCK) + 1) * ALLOC_BLOCK;
    ASSERT(bNewAlloc >= bNeeded);

    //FIXME: realloc ...
    newBuffer = _strutil_alloc(bNewAlloc);
    if (newBuffer == NULL)
        return FALSE;

    if ((copyBuffer) &&
        (dest->bUsed > 0))
        memcpy(newBuffer, dest->Buffer, dest->bUsed);

    /* free old buffer */
    if (dest->Buffer)
        _strutil_free(dest->Buffer);

    dest->Buffer = newBuffer;
    dest->bAllocated = bNewAlloc;

    return TRUE;
}

BOOL
ExtWStrSetN(
    IN PEXT_STRING dest,
    IN WCHAR* newstr,
    IN size_t chLen)
{
    ULONG bNeeded;

    if (chLen == (size_t)-1)
        chLen = wcslen(newstr);

    bNeeded = (chLen + 1) * sizeof(WCHAR);

    if (!_ExtStrRealloc(dest, bNeeded, FALSE))
        return FALSE;

    dest->bUsed = chLen * sizeof(WCHAR);
    if (chLen > 0)
        memcpy(dest->Buffer, newstr, dest->bUsed);
    ((WCHAR*)dest->Buffer)[chLen] = 0;

    return TRUE;
}

