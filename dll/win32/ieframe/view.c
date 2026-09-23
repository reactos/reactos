/*
 * Copyright 2005 Jacek Caban
 * Copyright 2010 Ilya Shpigor
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
 * Implement the IViewObject interface
 */

static inline WebBrowser *impl_from_IViewObject2(IViewObject2 *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IViewObject2_iface);
}

static HRESULT WINAPI ViewObject_QueryInterface(IViewObject2 *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI ViewObject_AddRef(IViewObject2 *iface)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI ViewObject_Release(IViewObject2 *iface)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI ViewObject_Draw(IViewObject2 *iface, DWORD dwDrawAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev,
        HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
        BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p %p %p %p %08Ix)\n", This, dwDrawAspect, lindex,
            pvAspect, ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue,
            dwContinue);
    return S_OK;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObject2 *iface, DWORD dwAspect,
        LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
        LOGPALETTE **ppColorSet)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, dwAspect, lindex, pvAspect, ptd,
            hicTargetDev, ppColorSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex,
                                        void *pvAspect, DWORD *pdwFreeze)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    FIXME("(%p)->(%ld %ld %p %p)\n", This, dwDrawAspect, lindex, pvAspect, pdwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObject2 *iface, DWORD dwFreeze)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
    FIXME("(%p)->(%ld)\n", This, dwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObject2 *iface, DWORD aspects, DWORD advf,
        IAdviseSink *pAdvSink)
{
    WebBrowser *This = impl_from_IViewObject2(iface);

    TRACE("(%p)->(%ld %08lx %p)\n", This, aspects, advf, pAdvSink);

    if (aspects || advf) FIXME("aspects and/or flags not supported yet\n");

    This->sink_aspects = aspects;
    This->sink_flags = advf;
    if (This->sink) IAdviseSink_Release(This->sink);
    This->sink = pAdvSink;
    if (This->sink) IAdviseSink_AddRef(This->sink);

    return S_OK;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObject2 *iface, DWORD *pAspects,
        DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    WebBrowser *This = impl_from_IViewObject2(iface);

    TRACE("(%p)->(%p %p %p)\n", This, pAspects, pAdvf, ppAdvSink);

    if (pAspects) *pAspects = This->sink_aspects;
    if (pAdvf) *pAdvf = This->sink_flags;
    if (ppAdvSink)
    {
        *ppAdvSink = This->sink;
        if (*ppAdvSink) IAdviseSink_AddRef(*ppAdvSink);
    }

    return S_OK;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObject2 *iface, DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE *ptd, LPSIZEL lpsizel)
{
    WebBrowser *This = impl_from_IViewObject2(iface);
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

/**********************************************************************
 * Implement the IDataObject interface
 */

static inline WebBrowser *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IDataObject_iface);
}

static HRESULT WINAPI DataObject_QueryInterface(LPDATAOBJECT iface, REFIID riid, LPVOID * ppvObj)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppvObj);
}

static ULONG WINAPI DataObject_AddRef(LPDATAOBJECT iface)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI DataObject_Release(LPDATAOBJECT iface)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI DataObject_GetData(LPDATAOBJECT iface, LPFORMATETC pformatetcIn, STGMEDIUM *pmedium)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM *pmedium)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(LPDATAOBJECT iface, LPFORMATETC pformatetc)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(LPDATAOBJECT iface, LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(LPDATAOBJECT iface, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DAdvise(LPDATAOBJECT iface, FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DUnadvise(LPDATAOBJECT iface, DWORD dwConnection)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumDAdvise(LPDATAOBJECT iface, IEnumSTATDATA **ppenumAdvise)
{
    WebBrowser *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IDataObjectVtbl DataObjectVtbl = {
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise
};

void WebBrowser_ViewObject_Init(WebBrowser *This)
{
    This->IViewObject2_iface.lpVtbl = &ViewObjectVtbl;
    This->IDataObject_iface.lpVtbl  = &DataObjectVtbl;
}
