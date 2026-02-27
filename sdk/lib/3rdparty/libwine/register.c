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

static inline void *image_base(void)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
    extern IMAGE_DOS_HEADER __ImageBase;
    return (void *)&__ImageBase;
#else
    extern IMAGE_NT_HEADERS __wine_spec_nt_header;
    return (void *)((__wine_spec_nt_header.OptionalHeader.ImageBase + 0xffff) & ~0xffff);
#endif
}

static const WCHAR atl100W[] = {'a','t','l','1','0','0','.','d','l','l',0};
static const WCHAR regtypeW[] = {'W','I','N','E','_','R','E','G','I','S','T','R','Y',0};
static const WCHAR moduleW[] = {'M','O','D','U','L','E',0};
static const WCHAR systemrootW[] = {'S','y','s','t','e','m','R','o','o','t',0};

struct reg_info
{
    IRegistrar  *registrar;
    BOOL         do_register;
    HRESULT      result;
};

static HMODULE atl100;
static HRESULT (WINAPI *pAtlCreateRegistrar)(IRegistrar**);

static IRegistrar *create_registrar( HMODULE inst, struct reg_info *info )
{
    if (!pAtlCreateRegistrar)
    {
        if (!(atl100 = LoadLibraryW( atl100W )) ||
            !(pAtlCreateRegistrar = (void *)GetProcAddress( atl100, "AtlCreateRegistrar" )))
        {
            info->result = E_NOINTERFACE;
            return NULL;
        }
    }

    info->result = pAtlCreateRegistrar( &info->registrar );
    if (SUCCEEDED( info->result ))
    {
        WCHAR str[MAX_PATH];

        GetModuleFileNameW( inst, str, MAX_PATH );
        IRegistrar_AddReplacement( info->registrar, moduleW, str );
        GetEnvironmentVariableW( systemrootW, str, MAX_PATH );
        IRegistrar_AddReplacement( info->registrar, systemrootW, str );
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
    info.result = S_OK;
    EnumResourceNamesW( module, regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    return info.result;
}

HRESULT __wine_unregister_resources( HMODULE module )
{
    struct reg_info info;

    info.registrar = NULL;
    info.do_register = FALSE;
    info.result = S_OK;
    EnumResourceNamesW( module, regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    return info.result;
}

// FIXME: Workaround until all modules use the new prototype
// See rpcproxy.h
HRESULT __cdecl __wine_register_resources_new(void)
{
    struct reg_info info;

    info.registrar = NULL;
    info.do_register = TRUE;
    info.result = S_OK;
    EnumResourceNamesW( image_base(), regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    return info.result;
}

HRESULT __cdecl __wine_unregister_resources_new(void)
{
    struct reg_info info;

    info.registrar = NULL;
    info.do_register = FALSE;
    info.result = S_OK;
    EnumResourceNamesW( image_base(), regtypeW, register_resource, (LONG_PTR)&info );
    if (info.registrar) IRegistrar_Release( info.registrar );
    return info.result;
}
