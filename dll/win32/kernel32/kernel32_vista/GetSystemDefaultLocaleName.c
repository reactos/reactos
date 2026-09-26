/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of GetLocaleInfoEx (taken from wine-locale.c)
 * COPYRIGHT:   Copyright 1995 Martin von Loewis
 *              Copyright 1998 David Lee Lambert
 *              Copyright 2000 Julio César Gázquez
 *              Copyright 2002 Alexandre Julliard for CodeWeavers
 */

#include "k32_vista.h"
#include <ndk/rtlfuncs.h>

INT WINAPI GetSystemDefaultLocaleName(LPWSTR localename, INT len)
{
    LCID lcid = GetSystemDefaultLCID();
    return LCIDToLocaleName(lcid, localename, len, 0);
}

