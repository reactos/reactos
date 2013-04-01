/* Fault report handling
 *
 * Copyright 2007 Peter Dons Tychsen
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "errorrep.h"

WINE_DEFAULT_DEBUG_CHANNEL(faultrep);

static const WCHAR SZ_EXCLUSIONLIST_KEY[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'P','C','H','e','a','l','t','h','\\',
    'E','r','r','o','r','R','e','p','o','r','t','i','n','g','\\',
    'E','x','c','l','u','s','i','o','n','L','i','s','t', 0};

/*************************************************************************
 * AddERExcludedApplicationW  [FAULTREP.@]
 *
 * Adds an application to a list of applications for which fault reports
 * shouldn't be generated
 *
 * PARAMS
 * lpAppFileName  [I] The filename of the application executable
 *
 * RETURNS
 * TRUE on success, FALSE of failure
 *
 * NOTES
 * Wine doesn't use this data but stores it in the registry (in the same place
 * as Windows would) in case it will be useful in a future version
 *
 * According to MSDN this function should succeed even if the user has no write
 * access to HKLM. This probably means that there is no error checking.
 */
BOOL WINAPI AddERExcludedApplicationW(LPCWSTR lpAppFileName)
{
    WCHAR *bslash;
    DWORD value = 1;
    HKEY hkey;

    TRACE("(%s)\n", wine_dbgstr_w(lpAppFileName));
    bslash = strrchrW(lpAppFileName, '\\');
    if (bslash != NULL)
        lpAppFileName = bslash + 1;
    if (*lpAppFileName == '\0')
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RegCreateKeyW(HKEY_LOCAL_MACHINE, SZ_EXCLUSIONLIST_KEY, &hkey))
    {
        RegSetValueExW(hkey, lpAppFileName, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
        RegCloseKey(hkey);
    }

    return TRUE;
}

/*************************************************************************
 * AddERExcludedApplicationA  [FAULTREP.@]
 *
 * See AddERExcludedApplicationW
 */
BOOL WINAPI AddERExcludedApplicationA(LPCSTR lpAppFileName)
{
    int len = MultiByteToWideChar(CP_ACP, 0, lpAppFileName, -1, NULL, 0);
    WCHAR *wstr;
    BOOL ret;

    TRACE("(%s)\n", wine_dbgstr_a(lpAppFileName));
    if (len == 0)
        return FALSE;
    wstr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
    MultiByteToWideChar(CP_ACP, 0, lpAppFileName, -1, wstr, len);
    ret = AddERExcludedApplicationW(wstr);
    HeapFree(GetProcessHeap(), 0, wstr);
    return ret;
}

/*************************************************************************
 * ReportFault  [FAULTREP.@]
 */
EFaultRepRetVal WINAPI ReportFault(LPEXCEPTION_POINTERS pep, DWORD dwOpt)
{
    FIXME("%p 0x%x stub\n", pep, dwOpt);
    return frrvOk;
}

/***********************************************************************
 * DllMain.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
