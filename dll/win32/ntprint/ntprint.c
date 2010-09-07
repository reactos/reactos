/*
 * Implementation of the Spooler Setup API (Printing)
 *
 * Copyright 2007 Detlef Riekenberg
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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winnls.h"
#include "winver.h"
#include "winspool.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntprint);

HINSTANCE NTPRINT_hInstance = NULL;

typedef struct {
  LPMONITOR_INFO_2W mi2;    /* Buffer for installed Monitors */
  DWORD installed;          /* Number of installed Monitors */
} monitorinfo_t;

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            NTPRINT_hInstance = hinstDLL;
            DisableThreadLibraryCalls( hinstDLL );
            break;
    }
    return TRUE;
}

/*****************************************************
 *  PSetupCreateMonitorInfo  [NTPRINT.@]
 *
 *
 */

HANDLE WINAPI PSetupCreateMonitorInfo(LPVOID unknown1, LPVOID  unknown2,LPVOID unknown3)
{
    monitorinfo_t * mi=NULL;
    DWORD needed;
    DWORD res;

    TRACE("(%p, %p, %p)\n", unknown1, unknown2, unknown3);

    if ((unknown2 != NULL) || (unknown3 != NULL)) {
        FIXME("got unknown parameter: (%p, %p, %p)\n", unknown1, unknown2, unknown3);
        return NULL;
    }

    mi = HeapAlloc(GetProcessHeap(), 0, sizeof(monitorinfo_t));
    if (!mi) {
        /* FIXME: SetLastError() needed? */
        return NULL;
    }

    /* Get the needed size for all Monitors */
    res = EnumMonitorsW(NULL, 2, NULL, 0, &needed, &mi->installed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        mi->mi2 = HeapAlloc(GetProcessHeap(), 0, needed);
        res = EnumMonitorsW(NULL, 2, (LPBYTE) mi->mi2, needed, &needed, &mi->installed);
    }

    if (!res) {
        HeapFree(GetProcessHeap(), 0, mi);
        /* FIXME: SetLastError() needed? */
        return NULL;
    }

    TRACE("=> %p (%u monitors installed)\n", mi, mi->installed);
    return mi;
}

/*****************************************************
 *  PSetupDestroyMonitorInfo  [NTPRINT.@]
 *
 */

VOID WINAPI PSetupDestroyMonitorInfo(HANDLE monitorinfo)
{
    monitorinfo_t * mi = monitorinfo;

    TRACE("(%p)\n", mi);
    if (mi) {
        if (mi->installed) HeapFree(GetProcessHeap(), 0, mi->mi2);
        HeapFree(GetProcessHeap(), 0, mi);
    }
}

/*****************************************************
 *  PSetupEnumMonitor  [NTPRINT.@]
 *
 * Copy the selected Monitorname to a buffer
 *
 * PARAMS
 *  monitorinfo [I]  HANDLE from PSetupCreateMonitorInfo
 *  index       [I]  Nr. of the Monitorname to copy
 *  buffer      [I]  Target, that receive the Monitorname
 *  psize       [IO] PTR to a DWORD that hold the size of the buffer and receive
 *                   the needed size, when the buffer is too small
 *
 * RETURNS
 *  Success:  TRUE
 *  Failure:  FALSE
 *
 * NOTES
 *   size is in Bytes on w2k and WCHAR on XP
 *
 */

BOOL WINAPI PSetupEnumMonitor(HANDLE monitorinfo, DWORD index, LPWSTR buffer, LPDWORD psize)
{
    monitorinfo_t * mi = monitorinfo;
    LPWSTR  nameW;
    DWORD   len;

    TRACE("(%p, %u, %p, %p) => %d\n", mi, index, buffer, psize, psize ? *psize : 0);

    if (index < mi->installed) {
        nameW = mi->mi2[index].pName;
        len = lstrlenW(nameW) + 1;
        if (len <= *psize) {
            memcpy(buffer, nameW, len * sizeof(WCHAR));
            TRACE("#%u: %s\n", index, debugstr_w(buffer));
            return TRUE;
        }
        *psize = len;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}
