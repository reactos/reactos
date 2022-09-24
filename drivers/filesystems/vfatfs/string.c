/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume routines
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2020 Doug Lyons <douglyons@douglyons.com>
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
