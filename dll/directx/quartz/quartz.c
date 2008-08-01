/*
 * QUERTZ.DLL - ReactOS DirectShow Runtime
 *
 * Copyright 2008 Dmitry Chapyshev
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

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>

#define MAX_ERROR_TEXT_LEN 160

DWORD WINAPI
AMGetErrorTextA(HRESULT hr, LPTSTR buffer, DWORD maxlen)
{
    int len;
    static const TCHAR format[] = {'E','r','r','o','r',':',' ','0','x','%','l','x',0};
    TCHAR error[MAX_ERROR_TEXT_LEN];

    if (!buffer) return 0;
    _stprintf(error, format, hr);
    if ((len = _tcslen(error)) >= maxlen) return 0; 
    _tcscpy(buffer, error);
    return len;
}

DWORD WINAPI
AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    int len;
    static const WCHAR format[] = {'E','r','r','o','r',':',' ','0','x','%','l','x',0};
    WCHAR error[MAX_ERROR_TEXT_LEN];

    if (!buffer) return 0;
    swprintf(error, format, hr);
    if ((len = wcslen(error)) >= maxlen) return 0; 
    wcscpy(buffer, error);
    return len;
}

LONG WINAPI
AmpFactorToDB(LONG ampfactor)
{
    return 0;
}

LONG WINAPI
DBToAmpFactor(LONG db)
{
    /* Avoid divide by zero (probably during range computation) in Windows Media Player 6.4 */
    if (db < -1000)
	return 0;
    return 100;
}

HRESULT WINAPI
DllCanUnloadNow(void)
{
	return S_OK;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return S_OK;
}

HRESULT WINAPI
DllRegisterServer(void)
{
	return S_OK;
}

HRESULT WINAPI
DllUnregisterServer(void)
{
	return S_OK;
}
