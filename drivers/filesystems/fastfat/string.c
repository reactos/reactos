/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/string.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMERS:      Jason Filby (jasonfilby@yahoo.com)
 *                   Doug Lyons (douglyons at douglyons dot com)
 *
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

const WCHAR *long_illegals = L"\"*\\<>/?:|";

BOOLEAN
vfatIsLongIllegal(
    WCHAR c)
{
    return wcschr(long_illegals, c) ? TRUE : FALSE;
}

BOOLEAN
IsDotOrDotDot(PCUNICODE_STRING Name)
{
    return ((Name->Length == sizeof(WCHAR) && Name->Buffer[0] == L'.') ||
        (Name->Length == 2 * sizeof(WCHAR) && Name->Buffer[0] == L'.' && Name->Buffer[1] == L'.'));
}
