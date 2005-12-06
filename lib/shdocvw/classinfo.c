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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/* Get the IID for generic default event callbacks.  This IID will
 * in theory be used to later query for an IConnectionPoint to connect
 * an event sink (callback implementation in the OLE control site)
 * to this control.
*/
static HRESULT WINAPI ProvideClassInfo_GetGUID(IProvideClassInfo2 *iface,
        DWORD dwGuidKind, GUID *pGUID)
{
    WebBrowser *This = CLASSINFO_THIS(iface);

    FIXME("(%p)->(%ld %p)\n", This, dwGuidKind, pGUID);

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        FIXME ("Requested unsupported GUID type: %ld\n", dwGuidKind);
        return E_FAIL;  /* Is there a better return type here? */
    }

    /* FIXME: Returning IPropertyNotifySink interface, but should really
     * return a more generic event set (???) dispinterface.
     * However, this hack, allows a control site to return with success
     * (MFC's COleControlSite falls back to older IProvideClassInfo interface
     * if GetGUID() fails to return a non-NULL GUID).
     */
    memcpy(pGUID, &IID_IPropertyNotifySink, sizeof(GUID));
    FIXME("Wrongly returning IPropertyNotifySink interface %s\n",
          debugstr_guid(pGUID));

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
