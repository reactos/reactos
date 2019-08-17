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

void
_ExtStrWSetTerm0(
    PEXT_STRING_W dest,
    ULONG chLen)
{
    ((WCHAR*)dest->Buffer)[chLen] = 0;
}

void
_ExtStrASetTerm0(
    PEXT_STRING_A dest,
    ULONG chLen)
{
    ((char*)dest->Buffer)[chLen] = 0;
}

/* init */
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

BOOL
ExtWStrInit(
    IN PEXT_STRING dst,
    IN WCHAR* initstr)
{
    ULONG bNeeded, chLen;
    if (initstr == NULL)
    {
        dst->bUsed = 0;
        dst->bAllocated = 0;
        dst->Buffer = NULL;
        dst->typ = stUnicodeStr;
        return TRUE;
    }

    dst->bUsed = 0;
    dst->bAllocated = 0;
    dst->Buffer = NULL;

    chLen = wcslen(initstr);
    bNeeded = (chLen + 1) * sizeof(WCHAR);
    if (!_ExtStrRealloc(dst, bNeeded, FALSE))
        return FALSE;
    dst->bUsed = chLen * sizeof(WCHAR);
    memcpy(dst->Buffer, initstr, dst->bUsed);
    _ExtStrWSetTerm0(dst, chLen);
    return TRUE;
}

BOOL
ExtAStrInit(
    IN PEXT_STRING dst,
    IN char* initstr)
{
    if (initstr == NULL)
    {
        dst->bUsed = 0;
        dst->bAllocated = 0;
        dst->Buffer = NULL;
        dst->typ = stAnsiStr;
        return TRUE;
    }
    return TRUE;
}

BOOL
ExtWStrCat(
    IN OUT PEXT_STRING dst,
    IN WCHAR* s)
{
    ULONG bNewStr = wcslen(s) * sizeof(WCHAR);
    ULONG bNeeded = dst->bUsed + bNewStr + sizeof(WCHAR);

    if (!_ExtStrRealloc(dst, bNeeded, TRUE))
        return FALSE;

    memcpy(dst->Buffer + dst->bUsed, s, bNewStr);
    dst->bUsed += bNewStr;
    _ExtStrWSetTerm0(dst, dst->bUsed / sizeof(WCHAR));
    return TRUE;
}

void
ExtWStrUpper(
    IN OUT PEXT_STRING s)
{
    int i1;
    PWCHAR tmp;
    tmp = (PWCHAR)s->Buffer;
    for (i1 = 0; i1 < s->bUsed / sizeof(WCHAR); i1++)
        tmp[i1] = toupper(tmp[i1]);
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


BOOL
ExtWStrSetN(
    IN PEXT_STRING_W dst,
    IN WCHAR* newstr,
    IN size_t chLen)
{
    ULONG bNeeded;

    if (chLen == (size_t)-1)
        chLen = wcslen(newstr);

    bNeeded = (chLen + 1) * sizeof(WCHAR);

    if (!_ExtStrRealloc(dst, bNeeded, FALSE))
        return FALSE;

    dst->bUsed = chLen * sizeof(WCHAR);
    if (chLen > 0)
        memcpy(dst->Buffer, newstr, dst->bUsed);
    _ExtStrWSetTerm0(dst, chLen);

    return TRUE;
}

BOOL
ExtAStrSetN(
    IN PEXT_STRING_A dst,
    IN char* newstr,
    IN size_t chLen)
{
    ULONG bNeeded;

    if (chLen == (size_t)-1)
        chLen = strlen(newstr);

    bNeeded = (chLen + 1) * sizeof(char);

    if (!_ExtStrRealloc(dst, bNeeded, FALSE))
        return FALSE;

    dst->bUsed = chLen * sizeof(char);
    if (chLen > 0)
        memcpy(dst->Buffer, newstr, dst->bUsed);
    _ExtStrASetTerm0(dst, chLen);

    return TRUE;
}

