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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    DispatchEx dispex;
    const IHTMLScreenVtbl *lpIHTMLScreenVtbl;

    LONG ref;
} HTMLScreen;

#define HTMLSCREEN(x)  ((IHTMLScreen*)  &(x)->lpIHTMLScreenVtbl)

#define HTMLSCREEN_THIS(iface) DEFINE_THIS(HTMLScreen, IHTMLScreen, iface)

static HRESULT WINAPI HTMLScreen_QueryInterface(IHTMLScreen *iface, REFIID riid, void **ppv)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSCREEN(This);
    }else if(IsEqualGUID(&IID_IHTMLScreen, riid)) {
        TRACE("(%p)->(IID_IHTMLScreen %p)\n", This, ppv);
        *ppv = HTMLSCREEN(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLScreen_AddRef(IHTMLScreen *iface)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLScreen_Release(IHTMLScreen *iface)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
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
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_GetTypeInfo(IHTMLScreen *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_GetIDsOfNames(IHTMLScreen *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_Invoke(IHTMLScreen *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_colorDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), BITSPIXEL);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_bufferDepth(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_bufferDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_width(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), HORZRES);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_height(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(get_display_dc(), VERTRES);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_updateInterval(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_updateInterval(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_availHeight(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_availWidth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_fontSmoothingEnabled(IHTMLScreen *iface, VARIANT_BOOL *p)
{
    HTMLScreen *This = HTMLSCREEN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLSCREEN_THIS

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

    screen->lpIHTMLScreenVtbl = &HTMLSreenVtbl;
    screen->ref = 1;

    init_dispex(&screen->dispex, (IUnknown*)HTMLSCREEN(screen), &HTMLScreen_dispex);

    *ret = HTMLSCREEN(screen);
    return S_OK;
}
