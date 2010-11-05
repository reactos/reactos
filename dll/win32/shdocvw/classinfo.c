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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "shdocvw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IProvideClassInfo2 interface
 */

#define CLASSINFO_THIS(iface) DEFINE_THIS(WebBrowser, ProvideClassInfo, iface)

static HRESULT WINAPI ProvideClassInfo_QueryInterface(IProvideClassInfo2 *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = CLASSINFO_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI ProvideClassInfo_AddRef(IProvideClassInfo2 *iface)
{
    WebBrowser *This = CLASSINFO_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ProvideClassInfo_Release(IProvideClassInfo2 *iface)
{
    WebBrowser *This = CLASSINFO_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ProvideClassInfo_GetClassInfo(IProvideClassInfo2 *iface, LPTYPEINFO *ppTI)
{
    WebBrowser *This = CLASSINFO_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppTI);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideClassInfo_GetGUID(IProvideClassInfo2 *iface,
        DWORD dwGuidKind, GUID *pGUID)
{
    WebBrowser *This = CLASSINFO_THIS(iface);

    TRACE("(%p)->(%d %p)\n", This, dwGuidKind, pGUID);

    if(!pGUID)
        return E_POINTER;

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID) {
        WARN("Wrong GUID type: %d\n", dwGuidKind);
        *pGUID = IID_NULL;
        return E_FAIL;
    }

    memcpy(pGUID, This->version == 1 ? &DIID_DWebBrowserEvents : &DIID_DWebBrowserEvents2,
           sizeof(GUID));
    return S_OK;
}

#undef CLASSINFO_THIS

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
    This->lpProvideClassInfoVtbl = &ProvideClassInfoVtbl;
}
