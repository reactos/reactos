/*
 * IUpdateSession implementation
 *
 * Copyright 2008 Hans Leidekker
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
#include "winuser.h"
#include "ole2.h"
#include "wuapi.h"

#include "wine/debug.h"
#include "wuapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef struct _update_session
{
    IUpdateSession IUpdateSession_iface;
    LONG refs;
} update_session;

static inline update_session *impl_from_IUpdateSession( IUpdateSession *iface )
{
    return CONTAINING_RECORD(iface, update_session, IUpdateSession_iface);
}

static ULONG WINAPI update_session_AddRef(
    IUpdateSession *iface )
{
    update_session *update_session = impl_from_IUpdateSession( iface );
    return InterlockedIncrement( &update_session->refs );
}

static ULONG WINAPI update_session_Release(
    IUpdateSession *iface )
{
    update_session *update_session = impl_from_IUpdateSession( iface );
    LONG refs = InterlockedDecrement( &update_session->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", update_session);
        HeapFree( GetProcessHeap(), 0, update_session );
    }
    return refs;
}

static HRESULT WINAPI update_session_QueryInterface(
    IUpdateSession *iface,
    REFIID riid,
    void **ppvObject )
{
    update_session *This = impl_from_IUpdateSession( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IUpdateSession ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IUpdateSession_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI update_session_GetTypeInfoCount(
    IUpdateSession *iface,
    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_GetTypeInfo(
    IUpdateSession *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_GetIDsOfNames(
    IUpdateSession *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_Invoke(
    IUpdateSession *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_get_ClientApplicationID(
    IUpdateSession *This,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_put_ClientApplicationID(
    IUpdateSession *This,
    BSTR value )
{
    FIXME("%p, %s\n", This, debugstr_w(value));
    return S_OK;
}

static HRESULT WINAPI update_session_get_ReadOnly(
    IUpdateSession *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_get_WebProxy(
    IUpdateSession *This,
    IWebProxy **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_put_WebProxy(
    IUpdateSession *This,
    IWebProxy *value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_session_CreateUpdateSearcher(
    IUpdateSession *This,
    IUpdateSearcher **retval )
{
    TRACE("%p\n", This);
    return UpdateSearcher_create( (LPVOID *)retval );
}

static HRESULT WINAPI update_session_CreateUpdateDownloader(
    IUpdateSession *This,
    IUpdateDownloader **retval )
{
    TRACE("%p\n", This);
    return UpdateDownloader_create( (LPVOID *)retval );
}

static HRESULT WINAPI update_session_CreateUpdateInstaller(
    IUpdateSession *This,
    IUpdateInstaller **retval )
{
    TRACE("%p\n", This);
    return UpdateInstaller_create( (LPVOID *)retval );
}

static const struct IUpdateSessionVtbl update_session_vtbl =
{
    update_session_QueryInterface,
    update_session_AddRef,
    update_session_Release,
    update_session_GetTypeInfoCount,
    update_session_GetTypeInfo,
    update_session_GetIDsOfNames,
    update_session_Invoke,
    update_session_get_ClientApplicationID,
    update_session_put_ClientApplicationID,
    update_session_get_ReadOnly,
    update_session_get_WebProxy,
    update_session_put_WebProxy,
    update_session_CreateUpdateSearcher,
    update_session_CreateUpdateDownloader,
    update_session_CreateUpdateInstaller
};

HRESULT UpdateSession_create( LPVOID *ppObj )
{
    update_session *session;

    TRACE("(%p)\n", ppObj);

    session = HeapAlloc( GetProcessHeap(), 0, sizeof(*session) );
    if (!session) return E_OUTOFMEMORY;

    session->IUpdateSession_iface.lpVtbl = &update_session_vtbl;
    session->refs = 1;

    *ppObj = &session->IUpdateSession_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
