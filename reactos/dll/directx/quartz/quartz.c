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
#include <debug.h>

#define NDEBUG

#define MAX_ERROR_TEXT_LEN 160

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

DWORD WINAPI
AMGetErrorTextA(HRESULT hr, LPSTR buffer, DWORD maxlen)
{
    int len;
    static const char format[] = "Error: 0x%lx";
    char error[MAX_ERROR_TEXT_LEN];

    DPRINT1("FIXME (%x,%p,%d) stub\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    sprintf(error, format, hr);
    if ((len = strlen(error)) >= maxlen) return 0; 
    strcpy(buffer, error);
    return len;
}

DWORD WINAPI
AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    int len;
    static const WCHAR format[] = {'E','r','r','o','r',':',' ','0','x','%','l','x',0};
    WCHAR error[MAX_ERROR_TEXT_LEN];

    DPRINT1("FIXME (%x,%p,%d) stub\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    swprintf(error, format, hr);
    if ((len = wcslen(error)) >= maxlen) return 0; 
    wcscpy(buffer, error);
    return len;
}

LONG WINAPI
AmpFactorToDB(LONG ampfactor)
{
    DPRINT1("FIXME (%d) Stub!\n", ampfactor);
    return 0;
}

LONG WINAPI
DBToAmpFactor(LONG db)
{
    DPRINT1("FIXME (%d) Stub!\n", db);
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
