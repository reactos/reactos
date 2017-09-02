/*
 * Copyright 2010 Louis Lenders
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

//#include <stdio.h>
//#include "windows.h"
//#include "ocidl.h"
#include <initguid.h>
//#include "objidl.h"
#include <wbemcli.h>
#include "wmic.h"

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(wmic);

static const WCHAR biosW[] =
    {'b','i','o','s',0};
static const WCHAR computersystemW[] =
    {'c','o','m','p','u','t','e','r','s','y','s','t','e','m',0};
static const WCHAR cpuW[] =
    {'c','p','u',0};
static const WCHAR logicaldiskW[] =
    {'L','o','g','i','c','a','l','D','i','s','k',0};
static const WCHAR nicW[] =
    {'n','i','c',0};
static const WCHAR osW[] =
    {'o','s',0};
static const WCHAR processW[] =
    {'p','r','o','c','e','s','s',0};

static const WCHAR win32_biosW[] =
    {'W','i','n','3','2','_','B','I','O','S',0};
static const WCHAR win32_computersystemW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
static const WCHAR win32_logicaldiskW[] =
    {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k',0};
static const WCHAR win32_networkadapterW[] =
    {'W','i','n','3','2','_','N','e','t','w','o','r','k','A','d','a','p','t','e','r',0};
static const WCHAR win32_operatingsystemW[] =
    {'W','i','n','3','2','_','O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR win32_processW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
static const WCHAR win32_processorW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};

static const struct
{
    const WCHAR *alias;
    const WCHAR *class;
}
alias_map[] =
{
    { biosW, win32_biosW },
    { computersystemW, win32_computersystemW },
    { cpuW, win32_processorW },
    { logicaldiskW, win32_logicaldiskW },
    { nicW, win32_networkadapterW },
    { osW, win32_operatingsystemW },
    { processW, win32_processW }
};

static const WCHAR *find_class( const WCHAR *alias )
{
    unsigned int i;

    for (i = 0; i < sizeof(alias_map)/sizeof(alias_map[0]); i++)
    {
        if (!strcmpiW( alias, alias_map[i].alias )) return alias_map[i].class;
    }
    return NULL;
}

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    if (!(dst = HeapAlloc( GetProcessHeap(), 0, (strlenW( src ) + 1) * sizeof(WCHAR) ))) return NULL;
    strcpyW( dst, src );
    return dst;
}

static WCHAR *find_prop( IWbemClassObject *class, const WCHAR *prop )
{
    SAFEARRAY *sa;
    WCHAR *ret = NULL;
    LONG i, last_index = 0;
    BSTR str;

    if (IWbemClassObject_GetNames( class, NULL, WBEM_FLAG_ALWAYS, NULL, &sa ) != S_OK) return NULL;

    SafeArrayGetUBound( sa, 1, &last_index );
    for (i = 0; i <= last_index; i++)
    {
        SafeArrayGetElement( sa, &i, &str );
        if (!strcmpiW( str, prop ))
        {
            ret = strdupW( str );
            break;
        }
    }
    SafeArrayDestroy( sa );
    return ret;
}

static int output_string( const WCHAR *msg, ... )
{
    va_list va_args;
    int wlen;
    DWORD count, ret;
    WCHAR buffer[8192];

    va_start( va_args, msg );
    vsprintfW( buffer, msg, va_args );
    va_end( va_args );

    wlen = strlenW( buffer );
    ret = WriteConsoleW( GetStdHandle(STD_OUTPUT_HANDLE), buffer, wlen, &count, NULL );
    if (!ret)
    {
        DWORD len;
        char *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte( GetConsoleOutputCP(), 0, buffer, wlen, NULL, 0, NULL, NULL );
        if (!(msgA = HeapAlloc( GetProcessHeap(), 0, len * sizeof(char) ))) return 0;

        WideCharToMultiByte( GetConsoleOutputCP(), 0, buffer, wlen, msgA, len, NULL, NULL );
        WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE );
        HeapFree( GetProcessHeap(), 0, msgA );
    }
    return count;
}

static int output_message( int msg )
{
    static const WCHAR fmtW[] = {'%','s',0};
    WCHAR buffer[8192];

    LoadStringW( GetModuleHandleW(NULL), msg, buffer, sizeof(buffer)/sizeof(WCHAR) );
    return output_string( fmtW, buffer );
}

static int query_prop( const WCHAR *class, const WCHAR *propname )
{
    static const WCHAR select_allW[] = {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',0};
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    static const WCHAR wqlW[] = {'W','Q','L',0};
    static const WCHAR newlineW[] = {'\n',0};
    static const WCHAR fmtW[] = {'%','s','\n',0};
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *result = NULL;
    LONG flags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
    BSTR path = NULL, wql = NULL, query = NULL;
    WCHAR *prop = NULL;
    BOOL first = TRUE;
    int len, ret = -1;

    WINE_TRACE("%s, %s\n", debugstr_w(class), debugstr_w(propname));

    CoInitialize( NULL );
    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                          RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                           (void **)&locator );
    if (hr != S_OK) goto done;

    if (!(path = SysAllocString( cimv2W ))) goto done;
    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    if (hr != S_OK) goto done;

    len = strlenW( class ) + sizeof(select_allW) / sizeof(select_allW[0]);
    if (!(query = SysAllocStringLen( NULL, len ))) goto done;
    strcpyW( query, select_allW );
    strcatW( query, class );

    if (!(wql = SysAllocString( wqlW ))) goto done;
    hr = IWbemServices_ExecQuery( services, wql, query, flags, NULL, &result );
    if (hr != S_OK) goto done;

    for (;;)
    {
        IWbemClassObject *obj;
        ULONG count;
        VARIANT v;

        IEnumWbemClassObject_Next( result, WBEM_INFINITE, 1, &obj, &count );
        if (!count) break;

        if (first)
        {
            if (!(prop = find_prop( obj, propname )))
            {
                output_message( STRING_INVALID_QUERY );
                goto done;
            }
            output_string( fmtW, prop );
            first = FALSE;
        }
        if (IWbemClassObject_Get( obj, prop, 0, &v, NULL, NULL ) == WBEM_S_NO_ERROR)
        {
            VariantChangeType( &v, &v, 0, VT_BSTR );
            output_string( fmtW, V_BSTR( &v ) );
            VariantClear( &v );
        }
        IWbemClassObject_Release( obj );
    }
    output_string( newlineW );
    ret = 0;

done:
    if (result) IEnumWbemClassObject_Release( result );
    if (services) IWbemServices_Release( services );
    if (locator) IWbemLocator_Release( locator );
    SysFreeString( path );
    SysFreeString( query );
    SysFreeString( wql );
    HeapFree( GetProcessHeap(), 0, prop );
    CoUninitialize();
    return ret;
}

int wmain(int argc, WCHAR *argv[])
{
    static const WCHAR getW[] = {'g','e','t',0};
    static const WCHAR quitW[] = {'q','u','i','t',0};
    static const WCHAR exitW[] = {'e','x','i','t',0};
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR classW[] = {'c','l','a','s','s',0};
    static const WCHAR contextW[] = {'c','o','n','t','e','x','t',0};
    const WCHAR *class, *value;
    int i;

    for (i = 1; i < argc && argv[i][0] == '/'; i++)
        WINE_FIXME( "command line switch %s not supported\n", debugstr_w(argv[i]) );

    if (i >= argc)
        goto not_supported;

    if (!strcmpiW( argv[i], quitW ) ||
        !strcmpiW( argv[i], exitW ))
    {
        return 0;
    }

    if (!strcmpiW( argv[i], classW) ||
        !strcmpiW( argv[i], contextW ))
    {
        WINE_FIXME( "command %s not supported\n", debugstr_w(argv[i]) );
        goto not_supported;
    }

    if (!strcmpiW( argv[i], pathW ))
    {
        if (++i >= argc)
        {
            output_message( STRING_INVALID_PATH );
            return 1;
        }
        class = argv[i];
    }
    else
    {
        class = find_class( argv[i] );
        if (!class)
        {
            output_message( STRING_ALIAS_NOT_FOUND );
            return 1;
        }
    }

    if (++i >= argc)
        goto not_supported;

    if (!strcmpiW( argv[i], getW ))
    {
        if (++i >= argc)
            goto not_supported;
        value = argv[i];
        return query_prop( class, value );
    }

not_supported:
    output_message( STRING_CMDLINE_NOT_SUPPORTED );
    return 1;
}
