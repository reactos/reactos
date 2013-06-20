/*
 * Support functions for Wine dll registrations
 *
 * Copyright (c) 2010 Alexandre Julliard
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

#include "config.h"
#include <stdarg.h>

#define COBJMACROS
#define ATL_INITGUID
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "atliface.h"

static const WCHAR ole32W[] = {'o','l','e','3','2','.','d','l','l',0};
static const WCHAR regtypeW[] = {'W','I','N','E','_','R','E','G','I','S','T','R','Y',0};
static const WCHAR moduleW[] = {'M','O','D','U','L','E',0};

struct reg_info
{
    IRegistrar  *registrar;
    BOOL         do_register;
    BOOL         uninit;
    HRESULT      result;
};

static HMODULE ole32;
static HRESULT (WINAPI *pCoInitialize)(LPVOID);
static void (WINAPI *pCoUninitialize)(void);
static HRESULT (WINAPI *pCoCreateInstance)(REFCLSID,LPUNKNOWN,DWORD,REFIID,LPVOID*);

static IRegistrar *create_registrar( HMODULE inst, struct reg_info *info )
{
    if (!pCoCreateInstance)
    {
        if (!(ole32 = LoadLibraryW( ole32W )) ||
            !(pCoInitialize = (void *)GetProcAddress( ole32, "CoInitialize" )) ||
            !(pCoUninitialize = (void *)GetProcAddress( ole32, "CoUninitialize" )) ||
            !(pCoCreateInstance = (void *)GetProcAddress( ole32, "CoCreateInstance" )))
        {
            info->result = E_NOINTERFACE;
            return NULL;
        }
    }
    info->uninit = SUCCEEDED( pCoInitialize( NULL ));

    info->result = pCoCreateInstance( &CLSID_Registrar, NULL, CLSCTX_INPROC_SERVER,
                                      &IID_IRegistrar, (void **)&info->registrar );
    if (SUCCEEDED( info->result ))
    {
        WCHAR str[MAX_PATH];

        GetModuleFileNameW( inst, str, MAX_PATH );
        IRegistrar_AddReplacement( info->registrar, moduleW, str );
    }
    return info->registrar;
}

static BOOL CALLBACK register_resource( HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR arg )
{
    struct reg_info *info = (struct reg_info *)arg;
    WCHAR *buffer;
    HRSRC rsrc = FindResourceW( module, name, type );
    char *str = LoadResource( module, rsrc );
    DWORD lenW, lenA = SizeofResource( module, rsrc );

    if (!str) return FALSE;
    if (!info->registrar && !create_registrar( module, info )) return FALSE;
    lenW = MultiByteToWideChar( CP_UTF8, 0, str, lenA, NULL, 0 ) + 1;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        info->result = E_OUTOFMEMORY;
        return FALSE;
    }
    MultiByteToWideChar( CP_UTF8, 0, str, lenA, buffer, lenW );
    buffer[lenW - 1] = 0;

    if (info->do_register)
        info->result = IRegistrar_StringRegister( info->registrar, buffer );
    else
        info->result = IRegistrar_StringUnregister( info->registrar, buffer );

    HeapFree( GetProcessHeap(), 0, buffer );
    return SUCCEEDED(info->result);
}

HRESULT __wine_register_resources( HMODULE module )
{
    struct reg_info info;

    info.registrar = NULL;
    info.do_register = TRUE;
    info.uninit = FALSE;
    info.result = S_OK;
    EnumResourceNamesW( module, regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    if (info.uninit) pCoUninitialize();
    return info.result;
}

HRESULT __wine_unregister_resources( HMODULE module )
{
    struct reg_info info;

    info.registrar = NULL;
    info.do_register = FALSE;
    info.uninit = FALSE;
    info.result = S_OK;
    EnumResourceNamesW( module, regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    if (info.uninit) pCoUninitialize();
    return info.result;
}
