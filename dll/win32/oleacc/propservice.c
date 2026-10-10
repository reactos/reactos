/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#include "oleacc_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

static HRESULT WINAPI AccPropServices_QueryInterface(IAccPropServices *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IAccPropServices, riid)) {
        TRACE("(IID_IAccPropServices %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AccPropServices_AddRef(IAccPropServices *iface)
{
    return 2;
}

static ULONG WINAPI AccPropServices_Release(IAccPropServices *iface)
{
    return 1;
}

static HRESULT WINAPI AccPropServices_SetPropValue(IAccPropServices *iface, const BYTE *pIDString,
        DWORD dwIDStringLen, MSAAPROPID idProp, VARIANT var)
{
    FIXME("(%p %lu %s %s)\n", pIDString, dwIDStringLen, debugstr_guid(&idProp), debugstr_variant(&var));
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetPropServer(IAccPropServices *iface, const BYTE *pIDString,
        DWORD dwIDStringLen, const MSAAPROPID *paProps, int cProps, IAccPropServer *pServer, AnnoScope AnnoScope)
{
    FIXME("(%p %lu %p %d %p %u)\n", pIDString, dwIDStringLen, paProps, cProps, pServer, AnnoScope);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_ClearProps(IAccPropServices *iface, const BYTE *pIDString,
        DWORD dwIDStringLen, const MSAAPROPID *paProps, int cProps)
{
    FIXME("(%p %lu %p %d)\n", pIDString, dwIDStringLen, paProps, cProps);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHwndProp(IAccPropServices *iface, HWND hwnd, DWORD idObject,
        DWORD idChild, MSAAPROPID idProp, VARIANT var)
{
    FIXME("(%p %lu %lu %s %s)\n", hwnd, idObject, idChild, debugstr_guid(&idProp), debugstr_variant(&var));
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHwndPropStr(IAccPropServices *iface, HWND hwnd, DWORD idObject,
        DWORD idChild, MSAAPROPID idProp, LPWSTR str)
{
    FIXME("(%p %lu %lu %s %s)\n", hwnd, idObject, idChild, debugstr_guid(&idProp), debugstr_w(str));
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHwndPropServer(IAccPropServices *iface, HWND hwnd, DWORD idObject,
        DWORD idChild, const MSAAPROPID *paProps, int cProps, IAccPropServer *pServer, AnnoScope AnnoScope)
{
    FIXME("(%p %lu %lu %p %d %p %u)\n", hwnd, idObject, idChild, paProps, cProps, pServer, AnnoScope);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_ClearHwndProps(IAccPropServices *iface, HWND hwnd, DWORD idObject,
        DWORD idChild, const MSAAPROPID *paProps, int cProps)
{
    FIXME("(%p %lu %lu %p %d)\n", hwnd, idObject, idChild, paProps, cProps);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_ComposeHwndIdentityString(IAccPropServices *iface, HWND hwnd,
        DWORD idObject, DWORD idChild, BYTE **ppIDString, DWORD *pdwIDStringLen)
{
    FIXME("(%p %lu %lu %p %p)\n", hwnd, idObject, idChild, ppIDString, pdwIDStringLen);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_DecomposeHwndIdentityString(IAccPropServices *iface, const BYTE *pIDString,
        DWORD dwIDStringLen, HWND *phwnd, DWORD *pidObject, DWORD *pidChild)
{
    FIXME("(%p %lu %p %p %p)\n", pIDString, dwIDStringLen, phwnd, pidObject, pidChild);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHmenuProp(IAccPropServices *iface, HMENU hmenu, DWORD idChild,
        MSAAPROPID idProp, VARIANT var)
{
    FIXME("(%p %lu %s %s)\n", hmenu, idChild, debugstr_guid(&idProp), debugstr_variant(&var));
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHmenuPropStr(IAccPropServices *iface, HMENU hmenu, DWORD idChild,
        MSAAPROPID idProp, LPWSTR str)
{
    FIXME("(%p %lu %s %s)\n", hmenu, idChild, debugstr_guid(&idProp), debugstr_w(str));
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_SetHmenuPropServer(IAccPropServices *iface, HMENU hmenu, DWORD idChild,
        const MSAAPROPID *paProps, int cProps, IAccPropServer *pServer, AnnoScope AnnoScope)
{
    FIXME("(%p %lu %p %d %p %u)\n", hmenu, idChild, paProps, cProps, pServer, AnnoScope);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_ClearHmenuProps(IAccPropServices *iface, HMENU hmenu, DWORD idChild,
        const MSAAPROPID *paProps, int cProps)
{
    FIXME("(%p %lu %p %d)\n", hmenu, idChild, paProps, cProps);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_ComposeHmenuIdentityString(IAccPropServices *iface, HMENU hmenu, DWORD idChild,
        BYTE **ppIDString, DWORD *pdwIDStringLen)
{
    FIXME("(%p %lu %p %p)\n", hmenu, idChild, ppIDString, pdwIDStringLen);
    return E_NOTIMPL;
}

static HRESULT WINAPI AccPropServices_DecomposeHmenuIdentityString(IAccPropServices *iface, const BYTE *pIDString,
        DWORD dwIDStringLen, HMENU *phmenu, DWORD *pidChild)
{
    FIXME("(%p %lu %p %p\n", pIDString, dwIDStringLen, phmenu, pidChild);
    return E_NOTIMPL;
}

static const IAccPropServicesVtbl AccPropServicesVtbl = {
    AccPropServices_QueryInterface,
    AccPropServices_AddRef,
    AccPropServices_Release,
    AccPropServices_SetPropValue,
    AccPropServices_SetPropServer,
    AccPropServices_ClearProps,
    AccPropServices_SetHwndProp,
    AccPropServices_SetHwndPropStr,
    AccPropServices_SetHwndPropServer,
    AccPropServices_ClearHwndProps,
    AccPropServices_ComposeHwndIdentityString,
    AccPropServices_DecomposeHwndIdentityString,
    AccPropServices_SetHmenuProp,
    AccPropServices_SetHmenuPropStr,
    AccPropServices_SetHmenuPropServer,
    AccPropServices_ClearHmenuProps,
    AccPropServices_ComposeHmenuIdentityString,
    AccPropServices_DecomposeHmenuIdentityString
};

static IAccPropServices AccPropServices = { &AccPropServicesVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static HRESULT WINAPI CAccPropServices_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    if(outer) {
        *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return IAccPropServices_QueryInterface(&AccPropServices, riid, ppv);
}

static const IClassFactoryVtbl CAccPropServicesFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    CAccPropServices_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory CAccPropServicesFactory = { &CAccPropServicesFactoryVtbl };

HRESULT get_accpropservices_factory(REFIID riid, void **ppv)
{
    return IClassFactory_QueryInterface(&CAccPropServicesFactory, riid, ppv);
}
