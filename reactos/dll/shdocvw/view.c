/*
 * Copyright 2005 Jacek Caban
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

#define VIEWOBJ_THIS(iface) DEFINE_THIS(WebBrowser, ViewObject, iface)

static HRESULT WINAPI ViewObject_QueryInterface(IViewObject2 *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_QueryInterface(WEBBROWSER(This), riid, ppv);
}

static ULONG WINAPI ViewObject_AddRef(IViewObject2 *iface)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ViewObject_Release(IViewObject2 *iface)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ViewObject_Draw(IViewObject2 *iface, DWORD dwDrawAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev,
        HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
        BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p %p %p %p %08lx)\n", This, dwDrawAspect, lindex,
            pvAspect, ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue,
            dwContinue);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObject2 *iface, DWORD dwAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
        LOGPALETTE **ppColorSet)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, dwAspect, lindex, pvAspect, ptd,
            hicTargetDev, ppColorSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex,
                                        void *pvAspect, DWORD *pdwFreeze)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld %ld %p %p)\n", This, dwDrawAspect, lindex, pvAspect, pdwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObject2 *iface, DWORD dwFreeze)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObject2 *iface, DWORD aspects, DWORD advf,
        IAdviseSink *pAdvSink)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld %08lx %p)\n", This, aspects, advf, pAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObject2 *iface, DWORD *pAspects,
        DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pAspects, pAdvf, ppAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObject2 *iface, DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE *ptd, LPSIZEL lpsizel)
{
    WebBrowser *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%ld %ld %p %p)\n", This, dwAspect, lindex, ptd, lpsizel);
    return E_NOTIMPL;
}

static const IViewObject2Vtbl ViewObjectVtbl = {
    ViewObject_QueryInterface,
    ViewObject_AddRef,
    ViewObject_Release,
    ViewObject_Draw,
    ViewObject_GetColorSet,
    ViewObject_Freeze,
    ViewObject_Unfreeze,
    ViewObject_SetAdvise,
    ViewObject_GetAdvise,
    ViewObject_GetExtent
};

#undef VIEWOBJ_THIS

void WebBrowser_ViewObject_Init(WebBrowser *This)
{
    This->lpViewObjectVtbl = &ViewObjectVtbl;
}
