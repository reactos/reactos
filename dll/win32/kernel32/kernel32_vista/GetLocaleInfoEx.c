/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementation of GetLocaleInfoEx (taken from wine-locale.c)
 * COPYRIGHT:   Copyright 1995 Martin von Loewis
 *              Copyright 1998 David Lee Lambert
 *              Copyright 2000 Julio César Gázquez
 *              Copyright 2002 Alexandre Julliard for CodeWeavers
 */

#include "k32_vista.h"
#include <winnls.h>

#define NDEBUG
#include <debug.h>

/******************************************************************************
 *           GetLocaleInfoEx (KERNEL32.@)
 */
INT WINAPI GetLocaleInfoEx(LPCWSTR locale, LCTYPE info, LPWSTR buffer, INT len)
{
    LCID lcid = LocaleNameToLCID(locale, 0);

    DPRINT("%s, lcid=0x%x, 0x%x\n", debugstr_w(locale), lcid, info);

    if (!lcid) return 0;

    /* special handling for neutral locale names */
    if (locale && strlenW(locale) == 2)
    {
        switch (info)
        {
        case LOCALE_SNAME:
            if (len && len < 3)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            if (len) strcpyW(buffer, locale);
            return 3;
        case LOCALE_SPARENT:
            if (len) buffer[0] = 0;
            return 1;
        }
    }

    return GetLocaleInfoW(lcid, info, buffer, len);
}
