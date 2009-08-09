/*
 * Copyright (C) 2009 Andrey Turkin
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

#ifndef __WINE_LOADPERF_H
#define __WINE_LOADPERF_H

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI InstallPerfDllA(LPCSTR, LPCSTR, ULONG_PTR);
DWORD WINAPI InstallPerfDllW(LPCWSTR, LPCWSTR, ULONG_PTR);
#define      InstallPerfDll WINELIB_NAME_AW(InstallPerfDll);

DWORD WINAPI LoadPerfCounterTextStringsA(LPCSTR, BOOL);
DWORD WINAPI LoadPerfCounterTextStringsW(LPCWSTR, BOOL);
#define      LoadPerfCounterTextStrings WINELIB_NAME_AW(LoadPerfCounterTextStrings)

DWORD WINAPI UnloadPerfCounterTextStringsA(LPCSTR, BOOL);
DWORD WINAPI UnloadPerfCounterTextStringsW(LPCWSTR, BOOL);
#define      UnloadPerfCounterTextStrings WINELIB_NAME_AW(UnloadPerfCounterTextStrings)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_LOADPERF_H */
