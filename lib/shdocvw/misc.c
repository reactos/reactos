/*
 * Implementation of miscellaneous interfaces for WebBrowser control:
 *
 *  - IQuickActivate
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IQuickActivate interface
 */

#define QUICKACT_THIS(iface) DEFINE_THIS(WebBrowser, QuickActivate, iface)

static HRESULT WINAPI QuickActivate_QueryInterface(IQuickActivate *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI QuickActivate_AddRef(IQuickActivate *iface)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI QuickActivate_Release(IQuickActivate *iface)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI QuickActivate_QuickActivate(IQuickActivate *iface,
        QACONTAINER *pQaContainer, QACONTROL *pQaControl)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pQaContainer, pQaControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI QuickActivate_SetContentExtent(IQuickActivate *iface, LPSIZEL pSizel)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pSizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI QuickActivate_GetContentExtent(IQuickActivate *iface, LPSIZEL pSizel)
{
    WebBrowser *This = QUICKACT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pSizel);
    return E_NOTIMPL;
}

#undef QUICKACT_THIS

static const IQuickActivateVtbl QuickActivateVtbl =
{
    QuickActivate_QueryInterface,
    QuickActivate_AddRef,
    QuickActivate_Release,
    QuickActivate_QuickActivate,
    QuickActivate_SetContentExtent,
    QuickActivate_GetContentExtent
};

void WebBrowser_Misc_Init(WebBrowser *This)
{
    This->lpQuickActivateVtbl = &QuickActivateVtbl;
}

/**********************************************************************
 * OpenURL  (SHDOCVW.@)
 */
void WINAPI OpenURL(HWND hWnd, HINSTANCE hInst, LPCSTR lpcstrUrl, int nShowCmd)
{
    FIXME("%p %p %s %d\n", hWnd, hInst, debugstr_a(lpcstrUrl), nShowCmd);
}
