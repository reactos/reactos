/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include <wine/unicode.h>

#define NDEBUG
#include <debug.h>

/* Taken from Wine kernel32/locale.c */

/******************************************************************************
 *           NormalizeString (KERNEL32.@)
 */
INT WINAPI NormalizeString(NORM_FORM NormForm, LPCWSTR lpSrcString, INT cwSrcLength,
                           LPWSTR lpDstString, INT cwDstLength)
{
    DPRINT1("%x %p %d %p %d\n", NormForm, lpSrcString, cwSrcLength, lpDstString, cwDstLength);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************************
 *           IsNormalizedString (KERNEL32.@)
 */
BOOL WINAPI IsNormalizedString(NORM_FORM NormForm, LPCWSTR lpString, INT cwLength)
{
    DPRINT1("%x %p %d\n", NormForm, lpString, cwLength);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
