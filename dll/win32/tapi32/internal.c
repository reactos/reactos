/*
 * TAPI32 internal functions
 *
 * Copyright 2008  Dmitry Chapyshev
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winerror.h"
#include "objbase.h"
#include "tapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *		internalConfig (TAPI32.@)
 */
LONG WINAPI internalConfig(HWND hParentWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    FIXME("internalConfig() not implemented!\n");
    return 0;
}

/***********************************************************************
 *		internalNewLocationW (TAPI32.@)
 */
DWORD WINAPI internalNewLocationW(LPWSTR lpName)
{
    FIXME("internalNewLocationW() not implemented!\n");
    return 0;
}

/***********************************************************************
 *		internalRemoveLocation (TAPI32.@)
 */
DWORD WINAPI internalRemoveLocation(DWORD dwLocationId)
{
    FIXME("internalRemoveLocation() not implemented!\n");
    return 0;
}

/***********************************************************************
 *		internalRenameLocationW (TAPI32.@)
 */
DWORD WINAPI internalRenameLocationW(WCHAR *szPrevName, WCHAR *szNewName)
{
    FIXME("internalRenameLocationW() not implemented!\n");
    return 0;
}
