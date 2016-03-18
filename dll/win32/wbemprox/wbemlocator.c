/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include "wbemprox_private.h"

typedef struct
{
    IWbemLocator IWbemLocator_iface;
    LONG refs;
} wbem_locator;

static inline wbem_locator *impl_from_IWbemLocator( IWbemLocator *iface )
{
    return CONTAINING_RECORD(iface, wbem_locator, IWbemLocator_iface);
}

static ULONG WINAPI wbem_locator_AddRef(
    IWbemLocator *iface )
{
    wbem_locator *wl = impl_from_IWbemLocator( iface );
    return InterlockedIncrement( &wl->refs );
}

static ULONG WINAPI wbem_locator_Release(
    IWbemLocator *iface )
{
    wbem_locator *wl = impl_from_IWbemLocator( iface );
    LONG refs = InterlockedDecrement( &wl->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", wl);
        heap_free( wl );
    }
    return refs;
}

static HRESULT WINAPI wbem_locator_QueryInterface(
    IWbemLocator *iface,
    REFIID riid,
    void **ppvObject )
{
    wbem_locator *This = impl_from_IWbemLocator( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemLocator ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemLocator_AddRef( iface );
    return S_OK;
}

static BOOL is_local_machine( const WCHAR *server )
{
    static const WCHAR dotW[] = {'.',0};
    static const WCHAR localhostW[] = {'l','o','c','a','l','h','o','s','t',0};
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = sizeof(buffer) / sizeof(buffer[0]);

    if (!server || !strcmpW( server, dotW ) || !strcmpiW( server, localhostW )) return TRUE;
    if (GetComputerNameW( buffer, &len ) && !strcmpiW( server, buffer )) return TRUE;
    return FALSE;
}

static HRESULT parse_resource( const WCHAR *resource, WCHAR **server, WCHAR **namespace )
{
    static const WCHAR rootW[] = {'R','O','O','T'};
    static const WCHAR cimv2W[] = {'C','I','M','V','2'};
    static const WCHAR defaultW[] = {'D','E','F','A','U','L','T'};
    HRESULT hr = WBEM_E_INVALID_NAMESPACE;
    const WCHAR *p, *q;
    unsigned int len;

    *server = NULL;
    *namespace = NULL;
    p = q = resource;
    if (*p == '\\' || *p == '/')
    {
        p++;
        if (*p == '\\' || *p == '/') p++;
        if (!*p) return WBEM_E_INVALID_NAMESPACE;
        if (*p == '\\' || *p == '/') return WBEM_E_INVALID_PARAMETER;
        q = p + 1;
        while (*q && *q != '\\' && *q != '/') q++;
        if (!*q) return WBEM_E_INVALID_NAMESPACE;
        len = q - p;
        if (!(*server = heap_alloc( (len + 1) * sizeof(WCHAR) )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        memcpy( *server, p, len * sizeof(WCHAR) );
        (*server)[len] = 0;
        q++;
    }
    if (!*q) goto done;
    p = q;
    while (*q && *q != '\\' && *q != '/') q++;
    len = q - p;
    if (len >= sizeof(rootW) / sizeof(rootW[0]) && memicmpW( rootW, p, len )) goto done;
    if (!*q)
    {
        hr = S_OK;
        goto done;
    }
    q++;
    len = strlenW( q );
    if ((len != sizeof(cimv2W) / sizeof(cimv2W[0]) || memicmpW( q, cimv2W, len )) &&
        (len != sizeof(defaultW) / sizeof(defaultW[0]) || memicmpW( q, defaultW, len )))
        goto done;
    if (!(*namespace = heap_alloc( (len + 1) * sizeof(WCHAR) ))) hr = E_OUTOFMEMORY;
    else
    {
        memcpy( *namespace, p, len * sizeof(WCHAR) );
        (*namespace)[len] = 0;
        hr = S_OK;
    }

done:
    if (hr != S_OK)
    {
        heap_free( *server );
        heap_free( *namespace );
    }
    return hr;
}

static HRESULT WINAPI wbem_locator_ConnectServer(
    IWbemLocator *iface,
    const BSTR NetworkResource,
    const BSTR User,
    const BSTR Password,
    const BSTR Locale,
    LONG SecurityFlags,
    const BSTR Authority,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace)
{
    HRESULT hr;
    WCHAR *server, *namespace;

    TRACE("%p, %s, %s, %s, %s, 0x%08x, %s, %p, %p)\n", iface, debugstr_w(NetworkResource), debugstr_w(User),
          debugstr_w(Password), debugstr_w(Locale), SecurityFlags, debugstr_w(Authority), pCtx, ppNamespace);

    hr = parse_resource( NetworkResource, &server, &namespace );
    if (hr != S_OK) return hr;

    if (!is_local_machine( server ))
    {
        FIXME("remote computer not supported\n");
        heap_free( server );
        heap_free( namespace );
        return WBEM_E_TRANSPORT_FAILURE;
    }
    if (User || Password || Authority)
        FIXME("authentication not supported\n");
    if (Locale)
        FIXME("specific locale not supported\n");
    if (SecurityFlags)
        FIXME("unsupported flags\n");

    hr = WbemServices_create( namespace, (void **)ppNamespace );
    heap_free( namespace );
    heap_free( server );
    if (SUCCEEDED( hr ))
        return WBEM_NO_ERROR;

    return WBEM_E_FAILED;
}

static const IWbemLocatorVtbl wbem_locator_vtbl =
{
    wbem_locator_QueryInterface,
    wbem_locator_AddRef,
    wbem_locator_Release,
    wbem_locator_ConnectServer
};

HRESULT WbemLocator_create( LPVOID *ppObj )
{
    wbem_locator *wl;

    TRACE("(%p)\n", ppObj);

    wl = heap_alloc( sizeof(*wl) );
    if (!wl) return E_OUTOFMEMORY;

    wl->IWbemLocator_iface.lpVtbl = &wbem_locator_vtbl;
    wl->refs = 1;

    *ppObj = &wl->IWbemLocator_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
