/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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

#include <wine/config.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <oleauto.h>
#include <dispex.h>
#include <activscp.h>
#include <wininet.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#if defined(__MINGW32__) || defined (_MSC_VER)
# include <ws2tcpip.h>
#else
# define closesocket close
# define ioctlsocket ioctl
#endif

#ifndef __MINGW32__
#define USE_WS_PREFIX
#endif

static HINSTANCE instance;

WINE_DEFAULT_DEBUG_CHANNEL(jsproxy);

static CRITICAL_SECTION cs_jsproxy;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &cs_jsproxy,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs_jsproxy") }
};
static CRITICAL_SECTION cs_jsproxy = { &critsect_debug, -1, 0, 0, 0, 0 };

static const WCHAR global_funcsW[] = {'g','l','o','b','a','l','_','f','u','n','c','s',0};
static const WCHAR dns_resolveW[] = {'d','n','s','_','r','e','s','o','l','v','e',0};

/******************************************************************
 *      DllMain (jsproxy.@)
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline WCHAR *strdupAW( const char *src, int len )
{
    WCHAR *dst = NULL;
    if (src)
    {
        int dst_len = MultiByteToWideChar( CP_ACP, 0, src, len, NULL, 0 );
        if ((dst = heap_alloc( (dst_len + 1) * sizeof(WCHAR) )))
        {
            len = MultiByteToWideChar( CP_ACP, 0, src, len, dst, dst_len );
            dst[dst_len] = 0;
        }
    }
    return dst;
}

static inline char *strdupWA( const WCHAR *src )
{
    char *dst = NULL;
    if (src)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
        if ((dst = heap_alloc( len ))) WideCharToMultiByte( CP_ACP, 0, src, -1, dst, len, NULL, NULL );
    }
    return dst;
}

static struct pac_script
{
    WCHAR *text;
} pac_script;
static struct pac_script *global_script = &pac_script;

/******************************************************************
 *      InternetDeInitializeAutoProxyDll (jsproxy.@)
 */
BOOL WINAPI InternetDeInitializeAutoProxyDll( LPSTR mime, DWORD reserved )
{
    TRACE( "%s, %u\n", debugstr_a(mime), reserved );

    EnterCriticalSection( &cs_jsproxy );

    heap_free( global_script->text );
    global_script->text = NULL;

    LeaveCriticalSection( &cs_jsproxy );
    return TRUE;
}

static WCHAR *load_script( const char *filename )
{
    HANDLE handle;
    DWORD size, bytes_read;
    char *buffer;
    int len;
    WCHAR *script = NULL;

    handle = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return NULL;

    size = GetFileSize( handle, NULL );
    if (!(buffer = heap_alloc( size ))) goto done;
    if (!ReadFile( handle, buffer, size, &bytes_read, NULL ) || bytes_read != size) goto done;

    len = MultiByteToWideChar( CP_ACP, 0, buffer, size, NULL, 0 );
    if (!(script = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto done;
    MultiByteToWideChar( CP_ACP, 0, buffer, size, script, len );
    script[len] = 0;

done:
    CloseHandle( handle );
    heap_free( buffer );
    return script;
}

/******************************************************************
 *      InternetInitializeAutoProxyDll (jsproxy.@)
 */
BOOL WINAPI JSPROXY_InternetInitializeAutoProxyDll( DWORD version, LPSTR tmpfile, LPSTR mime,
                                                    AutoProxyHelperFunctions *callbacks,
                                                    LPAUTO_PROXY_SCRIPT_BUFFER buffer )
{
    BOOL ret = FALSE;

    TRACE( "%u, %s, %s, %p, %p\n", version, debugstr_a(tmpfile), debugstr_a(mime), callbacks, buffer );

    if (callbacks) FIXME( "callbacks not supported\n" );

    EnterCriticalSection( &cs_jsproxy );

    if (buffer && buffer->dwStructSize == sizeof(*buffer) && buffer->lpszScriptBuffer)
    {
        DWORD i, len = 0;
        for (i = 0; i < buffer->dwScriptBufferSize; i++)
        {
            if (!buffer->lpszScriptBuffer[i]) break;
            len++;
        }
        if (len == buffer->dwScriptBufferSize)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            LeaveCriticalSection( &cs_jsproxy );
            return FALSE;
        }
        heap_free( global_script->text );
        if ((global_script->text = strdupAW( buffer->lpszScriptBuffer, len ))) ret = TRUE;
    }
    else
    {
        heap_free( global_script->text );
        if ((global_script->text = load_script( tmpfile ))) ret = TRUE;
    }

    LeaveCriticalSection( &cs_jsproxy );
    return ret;
}

static HRESULT WINAPI dispex_QueryInterface(
    IDispatchEx *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;

    if (IsEqualGUID( riid, &IID_IUnknown )  ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IDispatchEx ))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI dispex_AddRef(
    IDispatchEx *iface )
{
    return 2;
}

static ULONG WINAPI dispex_Release(
    IDispatchEx *iface )
{
    return 1;
}

static HRESULT WINAPI dispex_GetTypeInfoCount(
    IDispatchEx *iface, UINT *info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetTypeInfo(
    IDispatchEx *iface, UINT info, LCID lcid, ITypeInfo **type_info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetIDsOfNames(
    IDispatchEx *iface, REFIID riid, LPOLESTR *names, UINT count, LCID lcid, DISPID *id )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_Invoke(
    IDispatchEx *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep, UINT *err )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_DeleteMemberByName(
    IDispatchEx *iface, BSTR name, DWORD flags )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_DeleteMemberByDispID(
    IDispatchEx *iface, DISPID id )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberProperties(
    IDispatchEx *iface, DISPID id, DWORD flags_fetch, DWORD *flags )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberName(
    IDispatchEx *iface, DISPID id, BSTR *name )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNextDispID(
    IDispatchEx *iface, DWORD flags, DISPID id, DISPID *next )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNameSpaceParent(
    IDispatchEx *iface, IUnknown **unk )
{
    return E_NOTIMPL;
}

#define DISPID_GLOBAL_DNSRESOLVE  0x1000

static HRESULT WINAPI dispex_GetDispID(
    IDispatchEx *iface, BSTR name, DWORD flags, DISPID *id )
{
    if (!strcmpW( name, dns_resolveW ))
    {
        *id = DISPID_GLOBAL_DNSRESOLVE;
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

static char *get_computer_name( COMPUTER_NAME_FORMAT format )
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( format, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = heap_alloc( size ))) return NULL;
    if (!GetComputerNameExA( format, ret, &size ))
    {
        heap_free( ret );
        return NULL;
    }
    return ret;
}

static void printf_addr( const WCHAR *fmt, WCHAR *buf, struct sockaddr_in *addr )
{
    sprintfW( buf, fmt,
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 24 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 16 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) >> 8 & 0xff),
              (unsigned int)(ntohl( addr->sin_addr.s_addr ) & 0xff) );
}

static HRESULT dns_resolve( const WCHAR *hostname, VARIANT *result )
{
#ifdef HAVE_GETADDRINFO
        static const WCHAR fmtW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
        WCHAR addr[16];
        struct addrinfo *ai, *elem;
        char *hostnameA;
        int res;

        if (hostname[0])
            hostnameA = strdupWA( hostname );
        else
            hostnameA = get_computer_name( ComputerNamePhysicalDnsFullyQualified );

        if (!hostnameA) return E_OUTOFMEMORY;
        res = getaddrinfo( hostnameA, NULL, NULL, &ai );
        heap_free( hostnameA );
        if (res) return S_FALSE;

        elem = ai;
        while (elem && elem->ai_family != AF_INET) elem = elem->ai_next;
        if (!elem)
        {
            freeaddrinfo( ai );
            return S_FALSE;
        }
        printf_addr( fmtW, addr, (struct sockaddr_in *)elem->ai_addr );
        freeaddrinfo( ai );
        V_VT( result ) = VT_BSTR;
        V_BSTR( result ) = SysAllocString( addr );
        return S_OK;
#else
        FIXME("getaddrinfo not found at build time\n");
        return S_FALSE;
#endif
}

static HRESULT WINAPI dispex_InvokeEx(
    IDispatchEx *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
    VARIANT *result, EXCEPINFO *exep, IServiceProvider *caller )
{
    if (id == DISPID_GLOBAL_DNSRESOLVE)
    {
        if (params->cArgs != 1) return DISP_E_BADPARAMCOUNT;
        if (V_VT(&params->rgvarg[0]) != VT_BSTR) return DISP_E_BADVARTYPE;
        return dns_resolve( V_BSTR(&params->rgvarg[0]), result );
    }
    return DISP_E_MEMBERNOTFOUND;
}

static const IDispatchExVtbl dispex_vtbl =
{
    dispex_QueryInterface,
    dispex_AddRef,
    dispex_Release,
    dispex_GetTypeInfoCount,
    dispex_GetTypeInfo,
    dispex_GetIDsOfNames,
    dispex_Invoke,
    dispex_GetDispID,
    dispex_InvokeEx,
    dispex_DeleteMemberByName,
    dispex_DeleteMemberByDispID,
    dispex_GetMemberProperties,
    dispex_GetMemberName,
    dispex_GetNextDispID,
    dispex_GetNameSpaceParent
};

static IDispatchEx global_dispex = { &dispex_vtbl };

static HRESULT WINAPI site_QueryInterface(
    IActiveScriptSite *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;

    if (IsEqualGUID( &IID_IUnknown, riid ))
        *ppv = iface;
    else if (IsEqualGUID( &IID_IActiveScriptSite, riid ))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef( (IUnknown *)*ppv );
    return S_OK;
}

static ULONG WINAPI site_AddRef(
    IActiveScriptSite *iface )
{
    return 2;
}

static ULONG WINAPI site_Release(
    IActiveScriptSite *iface )
{
    return 1;
}

static HRESULT WINAPI site_GetLCID(
    IActiveScriptSite *iface, LCID *lcid )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_GetItemInfo(
    IActiveScriptSite *iface, LPCOLESTR name, DWORD mask,
    IUnknown **item, ITypeInfo **type_info )
{
    if (!strcmpW( name, global_funcsW ) && mask == SCRIPTINFO_IUNKNOWN)
    {
        *item = (IUnknown *)&global_dispex;
        return S_OK;
    }
    return E_NOTIMPL;
}

static HRESULT WINAPI site_GetDocVersionString(
    IActiveScriptSite *iface, BSTR *version )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnScriptTerminate(
    IActiveScriptSite *iface, const VARIANT *result, const EXCEPINFO *info )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnStateChange(
    IActiveScriptSite *iface, SCRIPTSTATE state )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnScriptError(
    IActiveScriptSite *iface, IActiveScriptError *error )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnEnterScript(
    IActiveScriptSite *iface )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI site_OnLeaveScript(
    IActiveScriptSite *iface )
{
    return E_NOTIMPL;
}

static const IActiveScriptSiteVtbl site_vtbl =
{
    site_QueryInterface,
    site_AddRef,
    site_Release,
    site_GetLCID,
    site_GetItemInfo,
    site_GetDocVersionString,
    site_OnScriptTerminate,
    site_OnStateChange,
    site_OnScriptError,
    site_OnEnterScript,
    site_OnLeaveScript
};

static IActiveScriptSite script_site = { &site_vtbl };

static BSTR include_pac_utils( const WCHAR *script )
{
    static const WCHAR pacjsW[] = {'p','a','c','.','j','s',0};
    HMODULE hmod = GetModuleHandleA( "jsproxy.dll" );
    HRSRC rsrc;
    DWORD size;
    const char *data;
    BSTR ret;
    int len;

    if (!(rsrc = FindResourceW( hmod, pacjsW, (LPCWSTR)40 ))) return NULL;
    size = SizeofResource( hmod, rsrc );
    data = LoadResource( hmod, rsrc );

    len = MultiByteToWideChar( CP_ACP, 0, data, size, NULL, 0 );
    if (!(ret = SysAllocStringLen( NULL, len + strlenW( script ) + 1 ))) return NULL;
    MultiByteToWideChar( CP_ACP, 0, data, size, ret, len );
    strcpyW( ret + len, script );
    return ret;
}

#ifdef _WIN64
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#else
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#endif

static BOOL run_script( const WCHAR *script, const WCHAR *url, const WCHAR *hostname, char **result_str, DWORD *result_len )
{
    static const WCHAR jscriptW[] = {'J','S','c','r','i','p','t',0};
    static const WCHAR findproxyW[] = {'F','i','n','d','P','r','o','x','y','F','o','r','U','R','L',0};
    IActiveScriptParse *parser = NULL;
    IActiveScript *engine = NULL;
    IDispatch *dispatch = NULL;
    BOOL ret = FALSE;
    CLSID clsid;
    DISPID dispid;
    BSTR func = NULL, full_script = NULL;
    VARIANT args[2], retval;
    DISPPARAMS params;
    HRESULT hr, init;

    init = CoInitialize( NULL );
    hr = CLSIDFromProgID( jscriptW, &clsid );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                           &IID_IActiveScript, (void **)&engine );
    if (hr != S_OK) goto done;

    hr = IActiveScript_QueryInterface( engine, &IID_IActiveScriptParse, (void **)&parser );
    if (hr != S_OK) goto done;

    hr = IActiveScriptParse_InitNew( parser );
    if (hr != S_OK) goto done;

    hr = IActiveScript_SetScriptSite( engine, &script_site );
    if (hr != S_OK) goto done;

    hr = IActiveScript_AddNamedItem( engine, global_funcsW, SCRIPTITEM_GLOBALMEMBERS );
    if (hr != S_OK) goto done;

    if (!(full_script = include_pac_utils( script ))) goto done;

    hr = IActiveScriptParse_ParseScriptText( parser, full_script, NULL, NULL, NULL, 0, 0, 0, NULL, NULL );
    if (hr != S_OK) goto done;

    hr = IActiveScript_SetScriptState( engine, SCRIPTSTATE_STARTED );
    if (hr != S_OK) goto done;

    hr = IActiveScript_GetScriptDispatch( engine, NULL, &dispatch );
    if (hr != S_OK) goto done;

    if (!(func = SysAllocString( findproxyW ))) goto done;
    hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &func, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
    if (hr != S_OK) goto done;

    V_VT( &args[0] ) = VT_BSTR;
    V_BSTR( &args[0] ) = SysAllocString( hostname );
    V_VT( &args[1] ) = VT_BSTR;
    V_BSTR( &args[1] ) = SysAllocString( url );

    params.rgvarg = args;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 2;
    params.cNamedArgs = 0;
    hr = IDispatch_Invoke( dispatch, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
                           &params, &retval, NULL, NULL );
    VariantClear( &args[0] );
    VariantClear( &args[1] );
    if (hr != S_OK)
    {
        WARN("script failed 0x%08x\n", hr);
        goto done;
    }
    if ((*result_str = strdupWA( V_BSTR( &retval ) )))
    {
        TRACE( "result: %s\n", debugstr_a(*result_str) );
        *result_len = strlen( *result_str ) + 1;
        ret = TRUE;
    }
    VariantClear( &retval );

done:
    SysFreeString( full_script );
    SysFreeString( func );
    if (dispatch) IDispatch_Release( dispatch );
    if (parser) IActiveScriptParse_Release( parser );
    if (engine) IActiveScript_Release( engine );
    if (SUCCEEDED( init )) CoUninitialize();
    return ret;
}

/******************************************************************
 *      InternetGetProxyInfo (jsproxy.@)
 */
BOOL WINAPI InternetGetProxyInfo( LPCSTR url, DWORD len_url, LPCSTR hostname, DWORD len_hostname, LPSTR *proxy,
                                  LPDWORD len_proxy )
{
    WCHAR *urlW = NULL, *hostnameW = NULL;
    BOOL ret = FALSE;

    TRACE( "%s, %u, %s, %u, %p, %p\n", debugstr_a(url), len_url, hostname, len_hostname, proxy, len_proxy );

    EnterCriticalSection( &cs_jsproxy );

    if (!global_script->text)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        goto done;
    }
    if (hostname && len_hostname < strlen( hostname ))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto done;
    }
    if (!(urlW = strdupAW( url, -1 ))) goto done;
    if (hostname && !(hostnameW = strdupAW( hostname, -1 ))) goto done;

    TRACE( "%s\n", debugstr_w(global_script->text) );
    ret = run_script( global_script->text, urlW, hostnameW, proxy, len_proxy );

done:
    heap_free( hostnameW );
    heap_free( urlW );
    LeaveCriticalSection( &cs_jsproxy );
    return ret;
}
