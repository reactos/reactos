/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

HRESULT get_window(DocHost *doc_host, IHTMLWindow2 **ret)
{
    IHTMLDocument2 *doc_obj;
    HRESULT hres;

    if(!doc_host->document) {
        *ret = NULL;
        return S_OK;
    }

    hres = IUnknown_QueryInterface(doc_host->document, &IID_IHTMLDocument2, (void**)&doc_obj);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDocument2_get_parentWindow(doc_obj, ret);
    IHTMLDocument2_Release(doc_obj);
    return hres;
}

static inline IEHTMLWindow *impl_from_IHTMLWindow2(IHTMLWindow2 *iface)
{
    return CONTAINING_RECORD(iface, IEHTMLWindow, IHTMLWindow2_iface);
}

static HRESULT WINAPI IEHTMLWindow2_QueryInterface(IHTMLWindow2 *iface, REFIID riid, void **ppv)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IHTMLFramesCollection2, riid)) {
        TRACE("(%p)->(IID_IHTMLFramesCollection2 %p)\n", This, ppv);
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow2, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow2 %p)\n", This, ppv);
        *ppv = &This->IHTMLWindow2_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IEHTMLWindow2_AddRef(IHTMLWindow2 *iface)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)\n", This);

    return IOleClientSite_AddRef(&This->doc_host->IOleClientSite_iface);
}

static ULONG WINAPI IEHTMLWindow2_Release(IHTMLWindow2 *iface)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)\n", This);

    return IOleClientSite_Release(&This->doc_host->IOleClientSite_iface);
}

static HRESULT WINAPI IEHTMLWindow2_GetTypeInfoCount(IHTMLWindow2 *iface, UINT *pctinfo)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_GetTypeInfo(IHTMLWindow2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_GetIDsOfNames(IHTMLWindow2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_Invoke(IHTMLWindow2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_item(IHTMLWindow2 *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(pvarIndex), pvarResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_length(IHTMLWindow2 *iface, LONG *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_frames(IHTMLWindow2 *iface, IHTMLFramesCollection2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_defaultStatus(IHTMLWindow2 *iface, BSTR v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_defaultStatus(IHTMLWindow2 *iface, BSTR *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_status(IHTMLWindow2 *iface, BSTR v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_status(IHTMLWindow2 *iface, BSTR *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_setTimeout(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %ld %s %p)\n", This, debugstr_w(expression), msec, debugstr_variant(language), timerID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_clearTimeout(IHTMLWindow2 *iface, LONG timerID)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld)\n", This, timerID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_alert(IHTMLWindow2 *iface, BSTR message)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(message));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_confirm(IHTMLWindow2 *iface, BSTR message,
        VARIANT_BOOL *confirmed)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(message), confirmed);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_prompt(IHTMLWindow2 *iface, BSTR message,
        BSTR dststr, VARIANT *textdata)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(message), debugstr_w(dststr), textdata);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_Image(IHTMLWindow2 *iface, IHTMLImageElementFactory **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_location(IHTMLWindow2 *iface, IHTMLLocation **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_history(IHTMLWindow2 *iface, IOmHistory **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_close(IHTMLWindow2 *iface)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);

    FIXME("(%p) semi-stub\n", This);

    if(!This->doc_host->wb)
        return E_UNEXPECTED;

    return IWebBrowser2_put_Visible(This->doc_host->wb, VARIANT_FALSE);
}

static HRESULT WINAPI IEHTMLWindow2_put_opener(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_opener(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_navigator(IHTMLWindow2 *iface, IOmNavigator **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_name(IHTMLWindow2 *iface, BSTR v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_name(IHTMLWindow2 *iface, BSTR *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_parent(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_open(IHTMLWindow2 *iface, BSTR url, BSTR name,
         BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %s %x %p)\n", This, debugstr_w(url), debugstr_w(name),
          debugstr_w(features), replace, pomWindowResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_self(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_top(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_window(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_navigate(IHTMLWindow2 *iface, BSTR url)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onfocus(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onfocus(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onblur(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onblur(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onload(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onload(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onbeforeunload(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onbeforeunload(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onunload(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onunload(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onhelp(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onhelp(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onerror(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onerror(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onresize(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onresize(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_onscroll(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_onscroll(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_document(IHTMLWindow2 *iface, IHTMLDocument2 **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    IHTMLWindow2 *window;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_window(This->doc_host, &window);
    if(FAILED(hres))
        return hres;

    if(!window)
        *p = NULL;
    else {
        hres = IHTMLWindow2_get_document(window, p);
        IHTMLWindow2_Release(window);
    }
    return hres;
}

static HRESULT WINAPI IEHTMLWindow2_get_event(IHTMLWindow2 *iface, IHTMLEventObj **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get__newEnum(IHTMLWindow2 *iface, IUnknown **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_showModalDialog(IHTMLWindow2 *iface, BSTR dialog,
        VARIANT *varArgIn, VARIANT *varOptions, VARIANT *varArgOut)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %s %p)\n", This, debugstr_w(dialog), debugstr_variant(varArgIn),
        debugstr_variant(varOptions), varArgOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_showHelp(IHTMLWindow2 *iface, BSTR helpURL, VARIANT helpArg,
        BSTR features)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(helpURL), debugstr_variant(&helpArg), debugstr_w(features));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_screen(IHTMLWindow2 *iface, IHTMLScreen **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_Option(IHTMLWindow2 *iface, IHTMLOptionElementFactory **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_focus(IHTMLWindow2 *iface)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_closed(IHTMLWindow2 *iface, VARIANT_BOOL *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_blur(IHTMLWindow2 *iface)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_scroll(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_clientInformation(IHTMLWindow2 *iface, IOmNavigator **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_setInterval(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %ld %s %p)\n", This, debugstr_w(expression), msec, debugstr_variant(language), timerID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_clearInterval(IHTMLWindow2 *iface, LONG timerID)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld)\n", This, timerID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_put_offscreenBuffering(IHTMLWindow2 *iface, VARIANT v)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_offscreenBuffering(IHTMLWindow2 *iface, VARIANT *p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_execScript(IHTMLWindow2 *iface, BSTR scode, BSTR language,
        VARIANT *pvarRet)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(scode), debugstr_w(language), pvarRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_toString(IHTMLWindow2 *iface, BSTR *String)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_scrollBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_scrollTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_moveTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_moveBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_resizeTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_resizeBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI IEHTMLWindow2_get_external(IHTMLWindow2 *iface, IDispatch **p)
{
    IEHTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLWindow2Vtbl IEHTMLWindow2Vtbl = {
    IEHTMLWindow2_QueryInterface,
    IEHTMLWindow2_AddRef,
    IEHTMLWindow2_Release,
    IEHTMLWindow2_GetTypeInfoCount,
    IEHTMLWindow2_GetTypeInfo,
    IEHTMLWindow2_GetIDsOfNames,
    IEHTMLWindow2_Invoke,
    IEHTMLWindow2_item,
    IEHTMLWindow2_get_length,
    IEHTMLWindow2_get_frames,
    IEHTMLWindow2_put_defaultStatus,
    IEHTMLWindow2_get_defaultStatus,
    IEHTMLWindow2_put_status,
    IEHTMLWindow2_get_status,
    IEHTMLWindow2_setTimeout,
    IEHTMLWindow2_clearTimeout,
    IEHTMLWindow2_alert,
    IEHTMLWindow2_confirm,
    IEHTMLWindow2_prompt,
    IEHTMLWindow2_get_Image,
    IEHTMLWindow2_get_location,
    IEHTMLWindow2_get_history,
    IEHTMLWindow2_close,
    IEHTMLWindow2_put_opener,
    IEHTMLWindow2_get_opener,
    IEHTMLWindow2_get_navigator,
    IEHTMLWindow2_put_name,
    IEHTMLWindow2_get_name,
    IEHTMLWindow2_get_parent,
    IEHTMLWindow2_open,
    IEHTMLWindow2_get_self,
    IEHTMLWindow2_get_top,
    IEHTMLWindow2_get_window,
    IEHTMLWindow2_navigate,
    IEHTMLWindow2_put_onfocus,
    IEHTMLWindow2_get_onfocus,
    IEHTMLWindow2_put_onblur,
    IEHTMLWindow2_get_onblur,
    IEHTMLWindow2_put_onload,
    IEHTMLWindow2_get_onload,
    IEHTMLWindow2_put_onbeforeunload,
    IEHTMLWindow2_get_onbeforeunload,
    IEHTMLWindow2_put_onunload,
    IEHTMLWindow2_get_onunload,
    IEHTMLWindow2_put_onhelp,
    IEHTMLWindow2_get_onhelp,
    IEHTMLWindow2_put_onerror,
    IEHTMLWindow2_get_onerror,
    IEHTMLWindow2_put_onresize,
    IEHTMLWindow2_get_onresize,
    IEHTMLWindow2_put_onscroll,
    IEHTMLWindow2_get_onscroll,
    IEHTMLWindow2_get_document,
    IEHTMLWindow2_get_event,
    IEHTMLWindow2_get__newEnum,
    IEHTMLWindow2_showModalDialog,
    IEHTMLWindow2_showHelp,
    IEHTMLWindow2_get_screen,
    IEHTMLWindow2_get_Option,
    IEHTMLWindow2_focus,
    IEHTMLWindow2_get_closed,
    IEHTMLWindow2_blur,
    IEHTMLWindow2_scroll,
    IEHTMLWindow2_get_clientInformation,
    IEHTMLWindow2_setInterval,
    IEHTMLWindow2_clearInterval,
    IEHTMLWindow2_put_offscreenBuffering,
    IEHTMLWindow2_get_offscreenBuffering,
    IEHTMLWindow2_execScript,
    IEHTMLWindow2_toString,
    IEHTMLWindow2_scrollBy,
    IEHTMLWindow2_scrollTo,
    IEHTMLWindow2_moveTo,
    IEHTMLWindow2_moveBy,
    IEHTMLWindow2_resizeTo,
    IEHTMLWindow2_resizeBy,
    IEHTMLWindow2_get_external
};

void IEHTMLWindow_Init(DocHost *doc_host)
{
    doc_host->html_window.IHTMLWindow2_iface.lpVtbl = &IEHTMLWindow2Vtbl;
    doc_host->html_window.doc_host = doc_host;
}
