/*
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

struct client_security
{
    IClientSecurity IClientSecurity_iface;
};

static inline struct client_security *impl_from_IClientSecurity( IClientSecurity *iface )
{
    return CONTAINING_RECORD( iface, struct client_security, IClientSecurity_iface );
}

static HRESULT WINAPI client_security_QueryInterface(
    IClientSecurity *iface,
    REFIID riid,
    void **ppvObject )
{
    struct client_security *cs = impl_from_IClientSecurity( iface );

    TRACE("%p %s %p\n", cs, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IClientSecurity ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &cs->IClientSecurity_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IClientSecurity_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI client_security_AddRef(
    IClientSecurity *iface )
{
    FIXME("%p\n", iface);
    return 2;
}

static ULONG WINAPI client_security_Release(
    IClientSecurity *iface )
{
    FIXME("%p\n", iface);
    return 1;
}

static HRESULT WINAPI client_security_QueryBlanket(
    IClientSecurity *iface,
    IUnknown *pProxy,
    DWORD *pAuthnSvc,
    DWORD *pAuthzSvc,
    OLECHAR **pServerPrincName,
    DWORD *pAuthnLevel,
    DWORD *pImpLevel,
    void **pAuthInfo,
    DWORD *pCapabilities )
{
    FIXME("semi-stub.\n");

    if (pAuthnSvc)
        *pAuthnSvc = RPC_C_AUTHN_NONE;
    if (pAuthzSvc)
        *pAuthzSvc = RPC_C_AUTHZ_NONE;
    if (pServerPrincName)
        *pServerPrincName = NULL;
    if (pAuthnLevel)
        *pAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
    if (pImpLevel)
        *pImpLevel = RPC_C_IMP_LEVEL_DEFAULT;
    if (pAuthInfo)
        *pAuthInfo = NULL;
    if (pCapabilities)
        *pCapabilities = 0;

    return WBEM_NO_ERROR;
}

static HRESULT WINAPI client_security_SetBlanket(
    IClientSecurity *iface,
    IUnknown *pProxy,
    DWORD AuthnSvc,
    DWORD AuthzSvc,
    OLECHAR *pServerPrincName,
    DWORD AuthnLevel,
    DWORD ImpLevel,
    void *pAuthInfo,
    DWORD Capabilities )
{
    const OLECHAR *princname = (pServerPrincName == COLE_DEFAULT_PRINCIPAL) ?
                               L"<COLE_DEFAULT_PRINCIPAL>" : pServerPrincName;

    FIXME( "%p, %p, %lu, %lu, %s, %lu, %lu, %p, %#lx\n", iface, pProxy, AuthnSvc, AuthzSvc,
           debugstr_w(princname), AuthnLevel, ImpLevel, pAuthInfo, Capabilities );
    return WBEM_NO_ERROR;
}

static HRESULT WINAPI client_security_CopyProxy(
    IClientSecurity *iface,
    IUnknown *pProxy,
    IUnknown **ppCopy )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static const IClientSecurityVtbl client_security_vtbl =
{
    client_security_QueryInterface,
    client_security_AddRef,
    client_security_Release,
    client_security_QueryBlanket,
    client_security_SetBlanket,
    client_security_CopyProxy
};

IClientSecurity client_security = { &client_security_vtbl };

struct async_header
{
    IWbemObjectSink *sink;
    void (*proc)( struct async_header * );
    HANDLE cancel;
    HANDLE wait;
};

struct async_query
{
    struct async_header hdr;
    enum wbm_namespace ns;
    WCHAR *str;
};

static void free_async( struct async_header *async )
{
    if (async->sink) IWbemObjectSink_Release( async->sink );
    CloseHandle( async->cancel );
    CloseHandle( async->wait );
    free( async );
}

static BOOL init_async( struct async_header *async, IWbemObjectSink *sink,
                        void (*proc)(struct async_header *) )
{
    if (!(async->wait = CreateEventW( NULL, FALSE, FALSE, NULL ))) return FALSE;
    if (!(async->cancel = CreateEventW( NULL, FALSE, FALSE, NULL )))
    {
        CloseHandle( async->wait );
        return FALSE;
    }
    async->proc = proc;
    async->sink = sink;
    IWbemObjectSink_AddRef( sink );
    return TRUE;
}

static DWORD CALLBACK async_proc( LPVOID param )
{
    struct async_header *async = param;
    HANDLE wait = async->wait;

    async->proc( async );

    WaitForSingleObject( async->cancel, INFINITE );
    SetEvent( wait );
    return ERROR_SUCCESS;
}

static HRESULT queue_async( struct async_header *async )
{
    if (QueueUserWorkItem( async_proc, async, WT_EXECUTELONGFUNCTION )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

struct wbem_services
{
    IWbemServices IWbemServices_iface;
    LONG refs;
    CRITICAL_SECTION cs;
    enum wbm_namespace ns;
    struct async_header *async;
    IWbemContext *context;
};

static inline struct wbem_services *impl_from_IWbemServices( IWbemServices *iface )
{
    return CONTAINING_RECORD( iface, struct wbem_services, IWbemServices_iface );
}

static ULONG WINAPI wbem_services_AddRef(
    IWbemServices *iface )
{
    struct wbem_services *ws = impl_from_IWbemServices( iface );
    return InterlockedIncrement( &ws->refs );
}

static ULONG WINAPI wbem_services_Release(
    IWbemServices *iface )
{
    struct wbem_services *ws = impl_from_IWbemServices( iface );
    LONG refs = InterlockedDecrement( &ws->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", ws);

        EnterCriticalSection( &ws->cs );
        if (ws->async) SetEvent( ws->async->cancel );
        LeaveCriticalSection( &ws->cs );
        if (ws->async)
        {
            WaitForSingleObject( ws->async->wait, INFINITE );
            free_async( ws->async );
        }
        ws->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &ws->cs );
        if (ws->context)
            IWbemContext_Release( ws->context );
        free( ws );
    }
    return refs;
}

static HRESULT WINAPI wbem_services_QueryInterface(
    IWbemServices *iface,
    REFIID riid,
    void **ppvObject )
{
    struct wbem_services *ws = impl_from_IWbemServices( iface );

    TRACE("%p %s %p\n", ws, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemServices ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &ws->IWbemServices_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IClientSecurity ) )
    {
        *ppvObject = &client_security;
        return S_OK;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemServices_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI wbem_services_OpenNamespace(
    IWbemServices *iface,
    const BSTR strNamespace,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppWorkingNamespace,
    IWbemCallResult **ppResult )
{
    struct wbem_services *ws = impl_from_IWbemServices( iface );

    TRACE( "%p, %s, %#lx, %p, %p, %p\n", iface, debugstr_w(strNamespace), lFlags,
           pCtx, ppWorkingNamespace, ppResult );

    if (ws->ns != WBEMPROX_NAMESPACE_LAST || !strNamespace)
        return WBEM_E_INVALID_NAMESPACE;

    return WbemServices_create( strNamespace, NULL, (void **)ppWorkingNamespace );
}

static HRESULT WINAPI wbem_services_CancelAsyncCall(
    IWbemServices *iface,
    IWbemObjectSink *pSink )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );
    struct async_header *async;

    TRACE("%p, %p\n", iface, pSink);

    if (!pSink) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &services->cs );

    if (!(async = services->async))
    {
        LeaveCriticalSection( &services->cs );
        return WBEM_E_INVALID_PARAMETER;
    }
    services->async = NULL;
    SetEvent( async->cancel );

    LeaveCriticalSection( &services->cs );

    WaitForSingleObject( async->wait, INFINITE );
    free_async( async );
    return S_OK;
}

static HRESULT WINAPI wbem_services_QueryObjectSink(
    IWbemServices *iface,
    LONG lFlags,
    IWbemObjectSink **ppResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

void free_path( struct path *path )
{
    if (!path) return;
    free( path->class );
    free( path->filter );
    free( path );
}

HRESULT parse_path( const WCHAR *str, struct path **ret )
{
    struct path *path;
    const WCHAR *p = str, *q;
    UINT len;

    if (!(path = calloc( 1, sizeof(*path) ))) return E_OUTOFMEMORY;

    if (*p == '\\')
    {
        static const WCHAR cimv2W[] = L"ROOT\\CIMV2";
        WCHAR server[MAX_COMPUTERNAME_LENGTH+1];
        DWORD server_len = ARRAY_SIZE(server);

        p++;
        if (*p != '\\')
        {
            free( path );
            return WBEM_E_INVALID_OBJECT_PATH;
        }
        p++;

        q = p;
        while (*p && *p != '\\') p++;
        if (!*p)
        {
            free( path );
            return WBEM_E_INVALID_OBJECT_PATH;
        }

        len = p - q;
        if (!GetComputerNameW( server, &server_len ) || server_len != len || wcsnicmp( q, server, server_len ))
        {
            free( path );
            return WBEM_E_NOT_SUPPORTED;
        }

        q = ++p;
        while (*p && *p != ':') p++;
        if (!*p)
        {
            free( path );
            return WBEM_E_INVALID_OBJECT_PATH;
        }

        len = p - q;
        if (len != ARRAY_SIZE(cimv2W) - 1 || wcsnicmp( q, cimv2W, ARRAY_SIZE(cimv2W) - 1 ))
        {
            free( path );
            return WBEM_E_INVALID_NAMESPACE;
        }
        p++;
    }

    q = p;
    while (*p && *p != '.' && *p != '=') p++;

    len = p - q;
    if (!(path->class = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        free( path );
        return E_OUTOFMEMORY;
    }
    memcpy( path->class, q, len * sizeof(WCHAR) );
    path->class[len] = 0;
    path->class_len = len;

    if (p[0] == '.' && p[1])
    {
        q = ++p;
        while (*q) q++;

        len = q - p;
        if (!(path->filter = malloc( (len + 1) * sizeof(WCHAR) )))
        {
            free_path( path );
            return E_OUTOFMEMORY;
        }
        memcpy( path->filter, p, len * sizeof(WCHAR) );
        path->filter[len] = 0;
        path->filter_len = len;
    }
    else if (p[0] == '=' && p[1])
    {
        WCHAR *key;
        UINT len_key;

        while (*q) q++;
        len = q - p;

        if (!(key = get_first_key_property( WBEMPROX_NAMESPACE_CIMV2, path->class )))
        {
            free_path( path );
            return WBEM_E_INVALID_OBJECT_PATH;
        }
        len_key = wcslen( key );

        if (!(path->filter = malloc( (len + len_key + 1) * sizeof(WCHAR) )))
        {
            free( key );
            free_path( path );
            return E_OUTOFMEMORY;
        }
        wcscpy( path->filter, key );
        memcpy( path->filter + len_key, p, len * sizeof(WCHAR) );
        path->filter_len = len + len_key;
        path->filter[path->filter_len] = 0;
        free( key );
    }
    else if (p[0])
    {
        free_path( path );
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    *ret = path;
    return S_OK;
}

WCHAR *query_from_path( const struct path *path )
{
    static const WCHAR selectW[] = L"SELECT * FROM %s WHERE %s";
    static const WCHAR select_allW[] = L"SELECT * FROM ";
    WCHAR *query;
    UINT len;

    if (path->filter)
    {
        len = path->class_len + path->filter_len + ARRAY_SIZE(selectW);
        if (!(query = malloc( len * sizeof(WCHAR) ))) return NULL;
        swprintf( query, len, selectW, path->class, path->filter );
    }
    else
    {
        len = path->class_len + ARRAY_SIZE(select_allW);
        if (!(query = malloc( len * sizeof(WCHAR) ))) return NULL;
        lstrcpyW( query, select_allW );
        lstrcatW( query, path->class );
    }
    return query;
}

static HRESULT create_instance_enum( enum wbm_namespace ns, const struct path *path, IEnumWbemClassObject **iter )
{
    WCHAR *query;
    HRESULT hr;

    if (!(query = query_from_path( path ))) return E_OUTOFMEMORY;
    hr = exec_query( ns, query, iter );
    free( query );
    return hr;
}

HRESULT get_object( enum wbm_namespace ns, const WCHAR *object_path, IWbemClassObject **obj )
{
    IEnumWbemClassObject *iter;
    struct path *path;
    ULONG count;
    HRESULT hr;

    hr = parse_path( object_path, &path );
    if (hr != S_OK) return hr;

    hr = create_instance_enum( ns, path, &iter );
    if (hr != S_OK)
    {
        free_path( path );
        return hr;
    }
    hr = IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, obj, &count );
    if (hr == WBEM_S_FALSE)
    {
        hr = WBEM_E_NOT_FOUND;
        *obj = NULL;
    }
    IEnumWbemClassObject_Release( iter );
    free_path( path );
    return hr;
}

static HRESULT WINAPI wbem_services_GetObject(
    IWbemServices *iface,
    const BSTR strObjectPath,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemClassObject **ppObject,
    IWbemCallResult **ppCallResult )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );

    TRACE( "%p, %s, %#lx, %p, %p, %p\n", iface, debugstr_w(strObjectPath), lFlags,
           pCtx, ppObject, ppCallResult );

    if (lFlags) FIXME( "unsupported flags %#lx\n", lFlags );

    if (!strObjectPath || !strObjectPath[0])
        return create_class_object( services->ns, NULL, NULL, 0, NULL, ppObject );

    return get_object( services->ns, strObjectPath, ppObject );
}

static HRESULT WINAPI wbem_services_GetObjectAsync(
    IWbemServices *iface,
    const BSTR strObjectPath,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_PutClass(
    IWbemServices *iface,
    IWbemClassObject *pObject,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemCallResult **ppCallResult )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_PutClassAsync(
    IWbemServices *iface,
    IWbemClassObject *pObject,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_DeleteClass(
    IWbemServices *iface,
    const BSTR strClass,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemCallResult **ppCallResult )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_DeleteClassAsync(
    IWbemServices *iface,
    const BSTR strClass,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_CreateClassEnum(
    IWbemServices *iface,
    const BSTR strSuperclass,
    LONG lFlags,
    IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_CreateClassEnumAsync(
    IWbemServices *iface,
    const BSTR strSuperclass,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_PutInstance(
    IWbemServices *iface,
    IWbemClassObject *pInst,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemCallResult **ppCallResult )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_PutInstanceAsync(
    IWbemServices *iface,
    IWbemClassObject *pInst,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_DeleteInstance(
    IWbemServices *iface,
    const BSTR strObjectPath,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemCallResult **ppCallResult )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_DeleteInstanceAsync(
    IWbemServices *iface,
    const BSTR strObjectPath,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_CreateInstanceEnum(
    IWbemServices *iface,
    const BSTR strClass,
    LONG lFlags,
    IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );
    struct path *path;
    HRESULT hr;

    TRACE( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(strClass), lFlags, pCtx, ppEnum );

    if (lFlags) FIXME( "unsupported flags %#lx\n", lFlags );

    hr = parse_path( strClass, &path );
    if (hr != S_OK) return hr;

    hr = create_instance_enum( services->ns, path, ppEnum );
    free_path( path );
    return hr;
}

static HRESULT WINAPI wbem_services_CreateInstanceEnumAsync(
    IWbemServices *iface,
    const BSTR strFilter,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_ExecQuery(
    IWbemServices *iface,
    const BSTR strQueryLanguage,
    const BSTR strQuery,
    LONG lFlags,
    IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );

    TRACE( "%p, %s, %s, %#lx, %p, %p\n", iface, debugstr_w(strQueryLanguage),
           debugstr_w(strQuery), lFlags, pCtx, ppEnum );

    if (!strQueryLanguage || !strQuery || !strQuery[0]) return WBEM_E_INVALID_PARAMETER;
    if (wcsicmp( strQueryLanguage, L"WQL" )) return WBEM_E_INVALID_QUERY_TYPE;
    return exec_query( services->ns, strQuery, ppEnum );
}

static void async_exec_query( struct async_header *hdr )
{
    struct async_query *query = (struct async_query *)hdr;
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = exec_query( query->ns, query->str, &result );
    if (hr == S_OK)
    {
        for (;;)
        {
            IEnumWbemClassObject_Next( result, WBEM_INFINITE, 1, &obj, &count );
            if (!count) break;
            IWbemObjectSink_Indicate( query->hdr.sink, 1, &obj );
            IWbemClassObject_Release( obj );
        }
        IEnumWbemClassObject_Release( result );
    }
    IWbemObjectSink_SetStatus( query->hdr.sink, WBEM_STATUS_COMPLETE, hr, NULL, NULL );
    free( query->str );
}

static HRESULT WINAPI wbem_services_ExecQueryAsync(
    IWbemServices *iface,
    const BSTR strQueryLanguage,
    const BSTR strQuery,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );
    IWbemObjectSink *sink;
    HRESULT hr = E_OUTOFMEMORY;
    struct async_header *async;
    struct async_query *query;

    TRACE( "%p, %s, %s, %#lx, %p, %p\n", iface, debugstr_w(strQueryLanguage), debugstr_w(strQuery),
           lFlags, pCtx, pResponseHandler );

    if (!pResponseHandler) return WBEM_E_INVALID_PARAMETER;

    hr = IWbemObjectSink_QueryInterface( pResponseHandler, &IID_IWbemObjectSink, (void **)&sink );
    if (FAILED(hr)) return hr;

    EnterCriticalSection( &services->cs );

    if (services->async)
    {
        FIXME("handle more than one pending async\n");
        hr = WBEM_E_FAILED;
        goto done;
    }
    if (!(query = calloc( 1, sizeof(*query) ))) goto done;
    query->ns = services->ns;
    async = (struct async_header *)query;

    if (!(init_async( async, sink, async_exec_query )))
    {
        free_async( async );
        goto done;
    }
    if (!(query->str = wcsdup( strQuery )))
    {
        free_async( async );
        goto done;
    }
    hr = queue_async( async );
    if (hr == S_OK) services->async = async;
    else
    {
        free( query->str );
        free_async( async );
    }

done:
    LeaveCriticalSection( &services->cs );
    IWbemObjectSink_Release( sink );
    return hr;
}

static HRESULT WINAPI wbem_services_ExecNotificationQuery(
    IWbemServices *iface,
    const BSTR strQueryLanguage,
    const BSTR strQuery,
    LONG lFlags,
    IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static HRESULT WINAPI wbem_services_ExecNotificationQueryAsync(
    IWbemServices *iface,
    const BSTR strQueryLanguage,
    const BSTR strQuery,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );
    IWbemObjectSink *sink;
    HRESULT hr = E_OUTOFMEMORY;
    struct async_header *async;
    struct async_query *query;

    TRACE( "%p, %s, %s, %#lx, %p, %p\n", iface, debugstr_w(strQueryLanguage), debugstr_w(strQuery),
           lFlags, pCtx, pResponseHandler );

    if (!pResponseHandler) return WBEM_E_INVALID_PARAMETER;

    hr = IWbemObjectSink_QueryInterface( pResponseHandler, &IID_IWbemObjectSink, (void **)&sink );
    if (FAILED(hr)) return hr;

    EnterCriticalSection( &services->cs );

    if (services->async)
    {
        FIXME("handle more than one pending async\n");
        hr = WBEM_E_FAILED;
        goto done;
    }
    if (!(query = calloc( 1, sizeof(*query) ))) goto done;
    async = (struct async_header *)query;

    if (!(init_async( async, sink, async_exec_query )))
    {
        free_async( async );
        goto done;
    }
    if (!(query->str = wcsdup( strQuery )))
    {
        free_async( async );
        goto done;
    }
    hr = queue_async( async );
    if (hr == S_OK) services->async = async;
    else
    {
        free( query->str );
        free_async( async );
    }

done:
    LeaveCriticalSection( &services->cs );
    IWbemObjectSink_Release( sink );
    return hr;
}

static HRESULT WINAPI wbem_services_ExecMethod(
    IWbemServices *iface,
    const BSTR strObjectPath,
    const BSTR strMethodName,
    LONG lFlags,
    IWbemContext *context,
    IWbemClassObject *pInParams,
    IWbemClassObject **ppOutParams,
    IWbemCallResult **ppCallResult )
{
    struct wbem_services *services = impl_from_IWbemServices( iface );
    IEnumWbemClassObject *result = NULL;
    IWbemClassObject *obj = NULL;
    struct query *query = NULL;
    struct path *path;
    WCHAR *str;
    class_method *func;
    struct table *table;
    HRESULT hr;

    TRACE( "%p, %s, %s, %#lx, %p, %p, %p, %p\n", iface, debugstr_w(strObjectPath),
           debugstr_w(strMethodName), lFlags, context, pInParams, ppOutParams, ppCallResult );

    if (lFlags) FIXME( "flags %#lx not supported\n", lFlags );

    if ((hr = parse_path( strObjectPath, &path )) != S_OK) return hr;
    if (!(str = query_from_path( path )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    if (!(query = create_query( services->ns )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    hr = parse_query( services->ns, str, &query->view, &query->mem );
    if (hr != S_OK) goto done;

    hr = execute_view( query->view );
    if (hr != S_OK) goto done;

    hr = EnumWbemClassObject_create( query, (void **)&result );
    if (hr != S_OK) goto done;

    table = get_view_table( query->view, 0 );
    hr = create_class_object( services->ns, table->name, result, 0, NULL, &obj );
    if (hr != S_OK) goto done;

    hr = get_method( table, strMethodName, &func );
    if (hr != S_OK) goto done;

    hr = func( obj, context ? context : services->context, pInParams, ppOutParams );

done:
    if (result) IEnumWbemClassObject_Release( result );
    if (obj) IWbemClassObject_Release( obj );
    free_query( query );
    free_path( path );
    free( str );
    return hr;
}

static HRESULT WINAPI wbem_services_ExecMethodAsync(
    IWbemServices *iface,
    const BSTR strObjectPath,
    const BSTR strMethodName,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemObjectSink *pResponseHandler )
{
    FIXME("\n");
    return WBEM_E_FAILED;
}

static const IWbemServicesVtbl wbem_services_vtbl =
{
    wbem_services_QueryInterface,
    wbem_services_AddRef,
    wbem_services_Release,
    wbem_services_OpenNamespace,
    wbem_services_CancelAsyncCall,
    wbem_services_QueryObjectSink,
    wbem_services_GetObject,
    wbem_services_GetObjectAsync,
    wbem_services_PutClass,
    wbem_services_PutClassAsync,
    wbem_services_DeleteClass,
    wbem_services_DeleteClassAsync,
    wbem_services_CreateClassEnum,
    wbem_services_CreateClassEnumAsync,
    wbem_services_PutInstance,
    wbem_services_PutInstanceAsync,
    wbem_services_DeleteInstance,
    wbem_services_DeleteInstanceAsync,
    wbem_services_CreateInstanceEnum,
    wbem_services_CreateInstanceEnumAsync,
    wbem_services_ExecQuery,
    wbem_services_ExecQueryAsync,
    wbem_services_ExecNotificationQuery,
    wbem_services_ExecNotificationQueryAsync,
    wbem_services_ExecMethod,
    wbem_services_ExecMethodAsync
};

HRESULT WbemServices_create( const WCHAR *namespace, IWbemContext *context, LPVOID *ppObj )
{
    struct wbem_services *ws;
    enum wbm_namespace ns;

    TRACE("namespace %s, context %p, ppObj %p.\n", debugstr_w(namespace), context, ppObj);

    if (!namespace)
        ns = WBEMPROX_NAMESPACE_LAST;
    else if ((ns = get_namespace_from_string( namespace )) == WBEMPROX_NAMESPACE_LAST)
        return WBEM_E_INVALID_NAMESPACE;

    if (!(ws = calloc( 1, sizeof(*ws) ))) return E_OUTOFMEMORY;

    ws->IWbemServices_iface.lpVtbl = &wbem_services_vtbl;
    ws->refs      = 1;
    ws->ns        = ns;
    InitializeCriticalSectionEx( &ws->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ws->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": wbemprox_services.cs");
    if (context)
        IWbemContext_Clone( context, &ws->context );

    *ppObj = &ws->IWbemServices_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

struct wbem_context_value
{
    struct list entry;
    WCHAR *name;
    VARIANT value;
};

struct wbem_context
{
    IWbemContext IWbemContext_iface;
    LONG refs;
    struct list values;
};

static void wbem_context_delete_values(struct wbem_context *context)
{
    struct wbem_context_value *value, *next;

    LIST_FOR_EACH_ENTRY_SAFE(value, next, &context->values, struct wbem_context_value, entry)
    {
        list_remove( &value->entry );
        VariantClear( &value->value );
        free( value->name );
        free( value );
    }
}

static struct wbem_context *impl_from_IWbemContext( IWbemContext *iface )
{
    return CONTAINING_RECORD( iface, struct wbem_context, IWbemContext_iface );
}

static HRESULT WINAPI wbem_context_QueryInterface(
    IWbemContext *iface,
    REFIID riid,
    void **obj)
{
    TRACE("%p, %s, %p\n", iface, debugstr_guid( riid ), obj );

    if ( IsEqualGUID( riid, &IID_IWbemContext ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IWbemContext_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI wbem_context_AddRef(
    IWbemContext *iface )
{
    struct wbem_context *context = impl_from_IWbemContext( iface );
    return InterlockedIncrement( &context->refs );
}

static ULONG WINAPI wbem_context_Release(
    IWbemContext *iface )
{
    struct wbem_context *context = impl_from_IWbemContext( iface );
    LONG refs = InterlockedDecrement( &context->refs );

    if (!refs)
    {
        TRACE("destroying %p\n", context);
        wbem_context_delete_values( context );
        free( context );
    }
    return refs;
}

static HRESULT WINAPI wbem_context_Clone(
    IWbemContext *iface,
    IWbemContext **newcopy )
{
    struct wbem_context *context = impl_from_IWbemContext( iface );
    struct wbem_context_value *value;
    IWbemContext *cloned_context;
    HRESULT hr;

    TRACE("%p, %p\n", iface, newcopy);

    if (SUCCEEDED(hr = WbemContext_create( (void **)&cloned_context, &IID_IWbemContext )))
    {
        LIST_FOR_EACH_ENTRY( value, &context->values, struct wbem_context_value, entry )
        {
            if (FAILED(hr = IWbemContext_SetValue( cloned_context, value->name, 0, &value->value ))) break;
        }
    }

    if (SUCCEEDED(hr))
    {
        *newcopy = cloned_context;
    }
    else
    {
        *newcopy = NULL;
        IWbemContext_Release( cloned_context );
    }

    return hr;
}

static HRESULT WINAPI wbem_context_GetNames(
    IWbemContext *iface,
    LONG flags,
    SAFEARRAY **names )
{
    FIXME( "%p, %#lx, %p\n", iface, flags, names );

    return E_NOTIMPL;
}

static HRESULT WINAPI wbem_context_BeginEnumeration(
    IWbemContext *iface,
    LONG flags )
{
    FIXME( "%p, %#lx\n", iface, flags );

    return E_NOTIMPL;
}

static HRESULT WINAPI wbem_context_Next(
    IWbemContext *iface,
    LONG flags,
    BSTR *name,
    VARIANT *value )
{
    FIXME( "%p, %#lx, %p, %p\n", iface, flags, name, value );

    return E_NOTIMPL;
}

static HRESULT WINAPI wbem_context_EndEnumeration(
    IWbemContext *iface )
{
    FIXME("%p\n", iface);

    return E_NOTIMPL;
}

static struct wbem_context_value *wbem_context_get_value( struct wbem_context *context, const WCHAR *name )
{
    struct wbem_context_value *value;

    LIST_FOR_EACH_ENTRY( value, &context->values, struct wbem_context_value, entry )
    {
        if (!lstrcmpiW( value->name, name )) return value;
    }

    return NULL;
}

static HRESULT WINAPI wbem_context_SetValue(
    IWbemContext *iface,
    LPCWSTR name,
    LONG flags,
    VARIANT *var )
{
    struct wbem_context *context = impl_from_IWbemContext( iface );
    struct wbem_context_value *value;
    HRESULT hr;

    TRACE( "%p, %s, %#lx, %s\n", iface, debugstr_w(name), flags, debugstr_variant(var) );

    if (!name || !var)
        return WBEM_E_INVALID_PARAMETER;

    if ((value = wbem_context_get_value( context, name )))
    {
        VariantClear( &value->value );
        hr = VariantCopy( &value->value, var );
    }
    else
    {
        if (!(value = calloc( 1, sizeof(*value) ))) return E_OUTOFMEMORY;
        if (!(value->name = wcsdup( name )))
        {
            free( value );
            return E_OUTOFMEMORY;
        }
        if (FAILED(hr = VariantCopy( &value->value, var )))
        {
            free( value->name );
            free( value );
            return hr;
        }

        list_add_tail( &context->values, &value->entry );
    }

    return hr;
}

static HRESULT WINAPI wbem_context_GetValue(
    IWbemContext *iface,
    LPCWSTR name,
    LONG flags,
    VARIANT *var )
{
    struct wbem_context *context = impl_from_IWbemContext( iface );
    struct wbem_context_value *value;

    TRACE( "%p, %s, %#lx, %p\n", iface, debugstr_w(name), flags, var );

    if (!name || !var)
        return WBEM_E_INVALID_PARAMETER;

    if (!(value = wbem_context_get_value( context, name )))
        return WBEM_E_NOT_FOUND;

    V_VT(var) = VT_EMPTY;
    return VariantCopy( var, &value->value );
}

static HRESULT WINAPI wbem_context_DeleteValue(
    IWbemContext *iface,
    LPCWSTR name,
    LONG flags )
{
    FIXME( "%p, %s, %#lx\n", iface, debugstr_w(name), flags );

    return E_NOTIMPL;
}

static HRESULT WINAPI wbem_context_DeleteAll(
    IWbemContext *iface )
{
    FIXME("%p\n", iface);

    return E_NOTIMPL;
}

static const IWbemContextVtbl wbem_context_vtbl =
{
    wbem_context_QueryInterface,
    wbem_context_AddRef,
    wbem_context_Release,
    wbem_context_Clone,
    wbem_context_GetNames,
    wbem_context_BeginEnumeration,
    wbem_context_Next,
    wbem_context_EndEnumeration,
    wbem_context_SetValue,
    wbem_context_GetValue,
    wbem_context_DeleteValue,
    wbem_context_DeleteAll,
};

HRESULT WbemContext_create( void **obj, REFIID riid )
{
    struct wbem_context *context;

    TRACE("(%p)\n", obj);

    if ( !IsEqualGUID( riid, &IID_IWbemContext ) &&
         !IsEqualGUID( riid, &IID_IUnknown ) )
        return E_NOINTERFACE;

    if (!(context = malloc( sizeof(*context) ))) return E_OUTOFMEMORY;

    context->IWbemContext_iface.lpVtbl = &wbem_context_vtbl;
    context->refs = 1;
    list_init(&context->values);

    *obj = &context->IWbemContext_iface;

    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
