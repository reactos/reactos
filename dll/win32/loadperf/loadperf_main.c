/*
 * Implementation of loadperf.dll
 *
 * Copyright 2009 Andrey Turkin
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
#include "winerror.h"
#include "winnls.h"
#include "wine/debug.h"

#include "loadperf.h"

WINE_DEFAULT_DEBUG_CHANNEL(loadperf);

static WCHAR *strdupAW(const char *str)
{
    WCHAR *ret = NULL;
    if (str)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        if (!(ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)))) return NULL;
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }
    return ret;
}

/*************************************************************
 *     InstallPerfDllA (loadperf.@)
 */
DWORD WINAPI InstallPerfDllA(LPCSTR computer, LPCSTR ini, ULONG_PTR flags)
{
    DWORD ret;
    LPWSTR computerW = NULL, iniW = NULL;

    if (computer && !(computerW = strdupAW(computer))) return ERROR_OUTOFMEMORY;
    if (ini && !(iniW = strdupAW(ini)))
    {
        HeapFree(GetProcessHeap(), 0, computerW);
        return ERROR_OUTOFMEMORY;
    }

    ret = InstallPerfDllW(computerW, iniW, flags);

    HeapFree(GetProcessHeap(), 0, computerW);
    HeapFree(GetProcessHeap(), 0, iniW);

    return ret;
}

/*************************************************************
 *     InstallPerfDllW (loadperf.@)
 */
DWORD WINAPI InstallPerfDllW(LPCWSTR computer, LPCWSTR ini, ULONG_PTR flags)
{
    FIXME("(%s, %s, %Ix)\n", debugstr_w(computer), debugstr_w(ini), flags);
    return ERROR_SUCCESS;
}

/*************************************************************
 *     LoadPerfCounterTextStringsA (loadperf.@)
 *
 * NOTES
 *   See LoadPerfCounterTextStringsW
 */
DWORD WINAPI LoadPerfCounterTextStringsA(LPCSTR cmdline, BOOL quiet)
{
    DWORD ret;
    LPWSTR cmdlineW = NULL;

    if (cmdline && !(cmdlineW = strdupAW(cmdline))) return ERROR_OUTOFMEMORY;

    ret = LoadPerfCounterTextStringsW(cmdlineW, quiet);

    HeapFree(GetProcessHeap(), 0, cmdlineW);

    return ret;
}

/*************************************************************
 *     LoadPerfCounterTextStringsW (loadperf.@)
 *
 * PARAMS
 *   cmdline [in] Last argument in command line - ini file to be used
 *   quiet   [in] FALSE - the function may write to stdout
 *
 */
DWORD WINAPI LoadPerfCounterTextStringsW(LPCWSTR cmdline, BOOL quiet)
{
    FIXME("(%s, %d): stub\n", debugstr_w(cmdline), quiet);

    return ERROR_SUCCESS;
}

/*************************************************************
 *     UnloadPerfCounterTextStringsA (loadperf.@)
 *
 * NOTES
 *   See UnloadPerfCounterTextStringsW
 */
DWORD WINAPI UnloadPerfCounterTextStringsA(LPCSTR cmdline, BOOL quiet)
{
    DWORD ret;
    LPWSTR cmdlineW = NULL;

    if (cmdline && !(cmdlineW = strdupAW(cmdline))) return ERROR_OUTOFMEMORY;

    ret = UnloadPerfCounterTextStringsW(cmdlineW, quiet);

    HeapFree(GetProcessHeap(), 0, cmdlineW);

    return ret;
}

/*************************************************************
 *     UnloadPerfCounterTextStringsW (loadperf.@)
 *
 * PARAMS
 *   cmdline [in] Last argument in command line - application counters to be removed
 *   quiet   [in] FALSE - the function may write to stdout
 *
 */
DWORD WINAPI UnloadPerfCounterTextStringsW(LPCWSTR cmdline, BOOL quiet)
{
    FIXME("(%s, %d): stub\n", debugstr_w(cmdline), quiet);

    return ERROR_SUCCESS;
}
