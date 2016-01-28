/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

typedef struct {
    DispatchEx dispex;
    IHTMLScreen IHTMLScreen_iface;

    LONG ref;
} HTMLScreen;

static inline HTMLScreen *impl_from_IHTMLScreen(IHTMLScreen *iface)
{
    return CONTAINING_RECORD(iface, HTMLScreen, IHTMLScreen_iface);
}

static HRESULT WINAPI HTMLScreen_QueryInterface(IHTMLScreen *iface, REFIID riid, void **ppv)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLScreen_iface;
    }else if(IsEqualGUID(&IID_IHTMLScreen, riid)) {
        *ppv = &This->IHTMLScreen_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLScreen_AddRef(IHTMLScreen *iface)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLScreen_Release(IHTMLScreen *iface)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLScreen_GetTypeInfoCount(IHTMLScreen *iface, UINT *pctinfo)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLScreen_GetTypeInfo(IHTMLScreen *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLScreen_GetIDsOfNames(IHTMLScreen *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLScreen_Invoke(IHTMLScreen *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLScreen_get_colorDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), BITSPIXEL);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_bufferDepth(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_bufferDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_width(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), HORZRES);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_height(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), VERTRES);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_updateInterval(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_updateInterval(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_availHeight(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.bottom-work_area.top;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_availWidth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.right-work_area.left;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_fontSmoothingEnabled(IHTMLScreen *iface, VARIANT_BOOL *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLScreenVtbl HTMLSreenVtbl = {
    HTMLScreen_QueryInterface,
    HTMLScreen_AddRef,
    HTMLScreen_Release,
    HTMLScreen_GetTypeInfoCount,
    HTMLScreen_GetTypeInfo,
    HTMLScreen_GetIDsOfNames,
    HTMLScreen_Invoke,
    HTMLScreen_get_colorDepth,
    HTMLScreen_put_bufferDepth,
    HTMLScreen_get_bufferDepth,
    HTMLScreen_get_width,
    HTMLScreen_get_height,
    HTMLScreen_put_updateInterval,
    HTMLScreen_get_updateInterval,
    HTMLScreen_get_availHeight,
    HTMLScreen_get_availWidth,
    HTMLScreen_get_fontSmoothingEnabled
};

static const tid_t HTMLScreen_iface_tids[] = {
    IHTMLScreen_tid,
    0
};
static dispex_static_data_t HTMLScreen_dispex = {
    NULL,
    DispHTMLScreen_tid,
    NULL,
    HTMLScreen_iface_tids
};

HRESULT HTMLScreen_Create(IHTMLScreen **ret)
{
    HTMLScreen *screen;

    screen = heap_alloc_zero(sizeof(HTMLScreen));
    if(!screen)
        return E_OUTOFMEMORY;

    screen->IHTMLScreen_iface.lpVtbl = &HTMLSreenVtbl;
    screen->ref = 1;

    init_dispex(&screen->dispex, (IUnknown*)&screen->IHTMLScreen_iface, &HTMLScreen_dispex);

    *ret = &screen->IHTMLScreen_iface;
    return S_OK;
}
