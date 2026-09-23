/*
 * Implementation of IProvideClassInfo interfaces for WebBrowser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include "ieframe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

/**********************************************************************
 * Implement the IProvideClassInfo2 interface
 */

static inline WebBrowser *impl_from_IProvideClassInfo2(IProvideClassInfo2 *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IProvideClassInfo2_iface);
}

static HRESULT WINAPI ProvideClassInfo_QueryInterface(IProvideClassInfo2 *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = impl_from_IProvideClassInfo2(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppobj);
}

static ULONG WINAPI ProvideClassInfo_AddRef(IProvideClassInfo2 *iface)
{
    WebBrowser *This = impl_from_IProvideClassInfo2(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI ProvideClassInfo_Release(IProvideClassInfo2 *iface)
{
    WebBrowser *This = impl_from_IProvideClassInfo2(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI ProvideClassInfo_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ppTI)
{
    WebBrowser *This = impl_from_IProvideClassInfo2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, ppTI);

    hres = get_typeinfo(This->version > 1 ? WebBrowser_tid : WebBrowser_V1_tid, ppTI);
    if(FAILED(hres))
        return hres;

    ITypeInfo_AddRef(*ppTI);
    return S_OK;
}

static HRESULT WINAPI ProvideClassInfo_GetGUID(IProvideClassInfo2 *iface,
        DWORD dwGuidKind, GUID *pGUID)
{
    WebBrowser *This = impl_from_IProvideClassInfo2(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwGuidKind, pGUID);

    if(!pGUID)
        return E_POINTER;

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID) {
        WARN("Wrong GUID type: %ld\n", dwGuidKind);
        *pGUID = IID_NULL;
        return E_FAIL;
    }

    memcpy(pGUID, This->version == 1 ? &DIID_DWebBrowserEvents : &DIID_DWebBrowserEvents2,
           sizeof(GUID));
    return S_OK;
}

static const IProvideClassInfo2Vtbl ProvideClassInfoVtbl =
{
    ProvideClassInfo_QueryInterface,
    ProvideClassInfo_AddRef,
    ProvideClassInfo_Release,
    ProvideClassInfo_GetClassInfo,
    ProvideClassInfo_GetGUID
};

void WebBrowser_ClassInfo_Init(WebBrowser *This)
{
    This->IProvideClassInfo2_iface.lpVtbl = &ProvideClassInfoVtbl;
}
