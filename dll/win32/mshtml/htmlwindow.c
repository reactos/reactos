/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static struct list window_list = LIST_INIT(window_list);

#define HTMLWINDOW2_THIS(iface) DEFINE_THIS(HTMLWindow, HTMLWindow2, iface)

static HRESULT WINAPI HTMLWindow2_QueryInterface(IHTMLWindow2 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_IHTMLFramesCollection2, riid)) {
        TRACE("(%p)->(IID_IHTMLFramesCollection2 %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IHTMLWindow2, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow2 %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IHTMLWindow3, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow2 %p)\n", This, ppv);
        *ppv = HTMLWINDOW3(This);
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

static ULONG WINAPI HTMLWindow2_AddRef(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLWindow2_Release(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        list_remove(&This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfoCount(IHTMLWindow2 *iface, UINT *pctinfo)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfo(IHTMLWindow2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow2_GetIDsOfNames(IHTMLWindow2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLWindow2_Invoke(IHTMLWindow2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow2_item(IHTMLWindow2 *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarIndex, pvarResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_length(IHTMLWindow2 *iface, long *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_frames(IHTMLWindow2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_defaultStatus(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_defaultStatus(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_status(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_status(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_setTimeout(IHTMLWindow2 *iface, BSTR expression,
        long msec, VARIANT *language, long *timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    VARIANT expr_var;

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr_var) = VT_BSTR;
    V_BSTR(&expr_var) = expression;

    return IHTMLWindow3_setTimeout(HTMLWINDOW3(This), &expr_var, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearTimeout(IHTMLWindow2 *iface, long timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, timerID);

    return clear_task_timer(This->doc, FALSE, timerID);
}

static HRESULT WINAPI HTMLWindow2_alert(IHTMLWindow2 *iface, BSTR message)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    WCHAR wszTitle[100];

    TRACE("(%p)->(%s)\n", This, debugstr_w(message));

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle,
                    sizeof(wszTitle)/sizeof(WCHAR))) {
        WARN("Could not load message box title: %d\n", GetLastError());
        return S_OK;
    }

    MessageBoxW(This->doc->hwnd, message, wszTitle, MB_ICONWARNING);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_confirm(IHTMLWindow2 *iface, BSTR message,
        VARIANT_BOOL *confirmed)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(message), confirmed);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_prompt(IHTMLWindow2 *iface, BSTR message,
        BSTR dststr, VARIANT *textdata)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(message), debugstr_w(dststr), textdata);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_Image(IHTMLWindow2 *iface, IHTMLImageElementFactory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_location(IHTMLWindow2 *iface, IHTMLLocation **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_history(IHTMLWindow2 *iface, IOmHistory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_close(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_opener(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_opener(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_navigator(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = OmNavigator_Create();
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_put_name(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_name(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_parent(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_open(IHTMLWindow2 *iface, BSTR url, BSTR name,
         BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %s %s %x %p)\n", This, debugstr_w(url), debugstr_w(name),
          debugstr_w(features), replace, pomWindowResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_self(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(HTMLWINDOW2(This));
    *p = HTMLWINDOW2(This);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_top(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_window(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(HTMLWINDOW2(This));
    *p = HTMLWINDOW2(This);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_navigate(IHTMLWindow2 *iface, BSTR url)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onfocus(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onfocus(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onblur(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onblur(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onbeforeunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onbeforeunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onhelp(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onhelp(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onerror(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onerror(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onresize(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onresize(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onscroll(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onscroll(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_document(IHTMLWindow2 *iface, IHTMLDocument2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->doc) {
        /* FIXME: We should return a wrapper object here */
        *p = HTMLDOC(This->doc);
        IHTMLDocument2_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_event(IHTMLWindow2 *iface, IHTMLEventObj **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get__newEnum(IHTMLWindow2 *iface, IUnknown **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showModalDialog(IHTMLWindow2 *iface, BSTR dialog,
        VARIANT *varArgIn, VARIANT *varOptions, VARIANT *varArgOut)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(dialog), varArgIn, varOptions, varArgOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showHelp(IHTMLWindow2 *iface, BSTR helpURL, VARIANT helpArg,
        BSTR features)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s v(%d) %s)\n", This, debugstr_w(helpURL), V_VT(&helpArg), debugstr_w(features));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_screen(IHTMLWindow2 *iface, IHTMLScreen **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_Option(IHTMLWindow2 *iface, IHTMLOptionElementFactory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc->option_factory)
        This->doc->option_factory = HTMLOptionElementFactory_Create(This->doc);

    *p = HTMLOPTFACTORY(This->doc->option_factory);
    IHTMLOptionElementFactory_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_focus(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_closed(IHTMLWindow2 *iface, VARIANT_BOOL *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_blur(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_scroll(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_clientInformation(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_setInterval(IHTMLWindow2 *iface, BSTR expression,
        long msec, VARIANT *language, long *timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    VARIANT expr;

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr) = VT_BSTR;
    V_BSTR(&expr) = expression;
    return IHTMLWindow3_setInterval(HTMLWINDOW3(This), &expr, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearInterval(IHTMLWindow2 *iface, long timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, timerID);

    return clear_task_timer(This->doc, TRUE, timerID);
}

static HRESULT WINAPI HTMLWindow2_put_offscreenBuffering(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_offscreenBuffering(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_execScript(IHTMLWindow2 *iface, BSTR scode, BSTR language,
        VARIANT *pvarRet)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(scode), debugstr_w(language), pvarRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_toString(IHTMLWindow2 *iface, BSTR *String)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_scrollBy(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld %ld)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollBy(This->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollBy failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_scrollTo(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld %ld)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollTo(This->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollTo failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_moveTo(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_moveBy(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeTo(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeBy(IHTMLWindow2 *iface, long x, long y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_external(IHTMLWindow2 *iface, IDispatch **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->doc->hostui)
        return S_OK;

    return IDocHostUIHandler_GetExternal(This->doc->hostui, p);
}

#undef HTMLWINDOW2_THIS

static const IHTMLWindow2Vtbl HTMLWindow2Vtbl = {
    HTMLWindow2_QueryInterface,
    HTMLWindow2_AddRef,
    HTMLWindow2_Release,
    HTMLWindow2_GetTypeInfoCount,
    HTMLWindow2_GetTypeInfo,
    HTMLWindow2_GetIDsOfNames,
    HTMLWindow2_Invoke,
    HTMLWindow2_item,
    HTMLWindow2_get_length,
    HTMLWindow2_get_frames,
    HTMLWindow2_put_defaultStatus,
    HTMLWindow2_get_defaultStatus,
    HTMLWindow2_put_status,
    HTMLWindow2_get_status,
    HTMLWindow2_setTimeout,
    HTMLWindow2_clearTimeout,
    HTMLWindow2_alert,
    HTMLWindow2_confirm,
    HTMLWindow2_prompt,
    HTMLWindow2_get_Image,
    HTMLWindow2_get_location,
    HTMLWindow2_get_history,
    HTMLWindow2_close,
    HTMLWindow2_put_opener,
    HTMLWindow2_get_opener,
    HTMLWindow2_get_navigator,
    HTMLWindow2_put_name,
    HTMLWindow2_get_name,
    HTMLWindow2_get_parent,
    HTMLWindow2_open,
    HTMLWindow2_get_self,
    HTMLWindow2_get_top,
    HTMLWindow2_get_window,
    HTMLWindow2_navigate,
    HTMLWindow2_put_onfocus,
    HTMLWindow2_get_onfocus,
    HTMLWindow2_put_onblur,
    HTMLWindow2_get_onblur,
    HTMLWindow2_put_onload,
    HTMLWindow2_get_onload,
    HTMLWindow2_put_onbeforeunload,
    HTMLWindow2_get_onbeforeunload,
    HTMLWindow2_put_onunload,
    HTMLWindow2_get_onunload,
    HTMLWindow2_put_onhelp,
    HTMLWindow2_get_onhelp,
    HTMLWindow2_put_onerror,
    HTMLWindow2_get_onerror,
    HTMLWindow2_put_onresize,
    HTMLWindow2_get_onresize,
    HTMLWindow2_put_onscroll,
    HTMLWindow2_get_onscroll,
    HTMLWindow2_get_document,
    HTMLWindow2_get_event,
    HTMLWindow2_get__newEnum,
    HTMLWindow2_showModalDialog,
    HTMLWindow2_showHelp,
    HTMLWindow2_get_screen,
    HTMLWindow2_get_Option,
    HTMLWindow2_focus,
    HTMLWindow2_get_closed,
    HTMLWindow2_blur,
    HTMLWindow2_scroll,
    HTMLWindow2_get_clientInformation,
    HTMLWindow2_setInterval,
    HTMLWindow2_clearInterval,
    HTMLWindow2_put_offscreenBuffering,
    HTMLWindow2_get_offscreenBuffering,
    HTMLWindow2_execScript,
    HTMLWindow2_toString,
    HTMLWindow2_scrollBy,
    HTMLWindow2_scrollTo,
    HTMLWindow2_moveTo,
    HTMLWindow2_moveBy,
    HTMLWindow2_resizeTo,
    HTMLWindow2_resizeBy,
    HTMLWindow2_get_external
};

#define HTMLWINDOW3_THIS(iface) DEFINE_THIS(HTMLWindow, HTMLWindow3, iface)

static HRESULT WINAPI HTMLWindow3_QueryInterface(IHTMLWindow3 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_QueryInterface(HTMLWINDOW2(This), riid, ppv);
}

static ULONG WINAPI HTMLWindow3_AddRef(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_AddRef(HTMLWINDOW2(This));
}

static ULONG WINAPI HTMLWindow3_Release(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_Release(HTMLWINDOW2(This));
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfoCount(IHTMLWindow3 *iface, UINT *pctinfo)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfo(IHTMLWindow3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow3_GetIDsOfNames(IHTMLWindow3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLWindow3_Invoke(IHTMLWindow3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow3_get_screenLeft(IHTMLWindow3 *iface, long *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_screenTop(IHTMLWindow3 *iface, long *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_attachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp, VARIANT_BOOL *pfResult)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_detachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT window_set_timer(HTMLWindow *This, VARIANT *expr, long msec, VARIANT *language,
        BOOL interval, long *timer_id)
{
    IDispatch *disp = NULL;

    switch(V_VT(expr)) {
    case VT_DISPATCH:
        disp = V_DISPATCH(expr);
        IDispatch_AddRef(disp);
        break;

    case VT_BSTR:
        disp = script_parse_event(This->doc, V_BSTR(expr));
        break;

    default:
        FIXME("unimplemented vt=%d\n", V_VT(expr));
        return E_NOTIMPL;
    }

    if(!disp)
        return E_FAIL;

    *timer_id = set_task_timer(This->doc, msec, interval, disp);
    IDispatch_Release(disp);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow3_setTimeout(IHTMLWindow3 *iface, VARIANT *expression, long msec,
        VARIANT *language, long *timerID)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    TRACE("(%p)->(%p(%d) %ld %p %p)\n", This, expression, V_VT(expression), msec, language, timerID);

    return window_set_timer(This, expression, msec, language, FALSE, timerID);
}

static HRESULT WINAPI HTMLWindow3_setInterval(IHTMLWindow3 *iface, VARIANT *expression, long msec,
        VARIANT *language, long *timerID)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    TRACE("(%p)->(%p %ld %p %p)\n", This, expression, msec, language, timerID);

    return window_set_timer(This, expression, msec, language, TRUE, timerID);
}

static HRESULT WINAPI HTMLWindow3_print(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onbeforeprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onbeforeprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onafterprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onafterprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_clipboardData(IHTMLWindow3 *iface, IHTMLDataTransfer **p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_showModelessDialog(IHTMLWindow3 *iface, BSTR url,
        VARIANT *varArgIn, VARIANT *options, IHTMLWindow2 **pDialog)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(url), varArgIn, options, pDialog);
    return E_NOTIMPL;
}

#undef HTMLWINDOW3_THIS

static const IHTMLWindow3Vtbl HTMLWindow3Vtbl = {
    HTMLWindow3_QueryInterface,
    HTMLWindow3_AddRef,
    HTMLWindow3_Release,
    HTMLWindow3_GetTypeInfoCount,
    HTMLWindow3_GetTypeInfo,
    HTMLWindow3_GetIDsOfNames,
    HTMLWindow3_Invoke,
    HTMLWindow3_get_screenLeft,
    HTMLWindow3_get_screenTop,
    HTMLWindow3_attachEvent,
    HTMLWindow3_detachEvent,
    HTMLWindow3_setTimeout,
    HTMLWindow3_setInterval,
    HTMLWindow3_print,
    HTMLWindow3_put_onbeforeprint,
    HTMLWindow3_get_onbeforeprint,
    HTMLWindow3_put_onafterprint,
    HTMLWindow3_get_onafterprint,
    HTMLWindow3_get_clipboardData,
    HTMLWindow3_showModelessDialog
};

#define DISPEX_THIS(iface) DEFINE_THIS(HTMLWindow, IDispatchEx, iface)

static HRESULT WINAPI WindowDispEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_QueryInterface(HTMLWINDOW2(This), riid, ppv);
}

static ULONG WINAPI WindowDispEx_AddRef(IDispatchEx *iface)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_AddRef(HTMLWINDOW2(This));
}

static ULONG WINAPI WindowDispEx_Release(IDispatchEx *iface)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_Release(HTMLWINDOW2(This));
}

static HRESULT WINAPI WindowDispEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI WindowDispEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI WindowDispEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    /* FIXME: Use script dispatch */

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI WindowDispEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    /* FIXME: Use script dispatch */

    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
                              pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI WindowDispEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    return IDispatchEx_GetDispID(DISPATCHEX(&This->dispex), bstrName, grfdex, pid);
}

static HRESULT WINAPI WindowDispEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    return IDispatchEx_InvokeEx(DISPATCHEX(&This->dispex), id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);

    return IDispatchEx_DeleteMemberByName(DISPATCHEX(&This->dispex), bstrName, grfdex);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x)\n", This, id);

    return IDispatchEx_DeleteMemberByDispID(DISPATCHEX(&This->dispex), id);
}

static HRESULT WINAPI WindowDispEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);

    return IDispatchEx_GetMemberProperties(DISPATCHEX(&This->dispex), id, grfdexFetch, pgrfdex);
}

static HRESULT WINAPI WindowDispEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %p)\n", This, id, pbstrName);

    return IDispatchEx_GetMemberName(DISPATCHEX(&This->dispex), id, pbstrName);
}

static HRESULT WINAPI WindowDispEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %p)\n", This, grfdex, id, pid);

    return IDispatchEx_GetNextDispID(DISPATCHEX(&This->dispex), grfdex, id, pid);
}

static HRESULT WINAPI WindowDispEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppunk);

    *ppunk = NULL;
    return S_OK;
}

#undef DISPEX_THIS

static const IDispatchExVtbl WindowDispExVtbl = {
    WindowDispEx_QueryInterface,
    WindowDispEx_AddRef,
    WindowDispEx_Release,
    WindowDispEx_GetTypeInfoCount,
    WindowDispEx_GetTypeInfo,
    WindowDispEx_GetIDsOfNames,
    WindowDispEx_Invoke,
    WindowDispEx_GetDispID,
    WindowDispEx_InvokeEx,
    WindowDispEx_DeleteMemberByName,
    WindowDispEx_DeleteMemberByDispID,
    WindowDispEx_GetMemberProperties,
    WindowDispEx_GetMemberName,
    WindowDispEx_GetNextDispID,
    WindowDispEx_GetNameSpaceParent
};

static const tid_t HTMLWindow_iface_tids[] = {
    IHTMLWindow2_tid,
    IHTMLWindow3_tid,
    0
};
static dispex_static_data_t HTMLWindow_dispex = {
    NULL,
    DispHTMLWindow2_tid,
    NULL,
    HTMLWindow_iface_tids
};

static const char wineConfig_func[] =
"window.__defineGetter__(\"external\",function() {\n"
"    return window.__wineWindow__.external;\n"
"});\n"
"window.__wineWindow__ = wineWindow;\n";

static void astr_to_nswstr(const char *str, nsAString *nsstr)
{
    LPWSTR wstr;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    wstr = heap_alloc(len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, len);

    nsAString_Init(nsstr, wstr);

    heap_free(wstr);
}

static nsresult call_js_func(nsIScriptContainer *script_container, nsISupports *target,
                             const char *name, const char *body,
                             PRUint32 argc, const char **arg_names, nsIArray *argv)
{
    nsACString name_str;
    nsAString body_str;
    JSObject func_obj, jsglobal;
    nsIVariant *jsret;
    nsresult nsres;

    nsres = nsIScriptContainer_GetGlobalObject(script_container, &jsglobal);
    if(NS_FAILED(nsres))
        ERR("GetGlobalObject: %08x\n", nsres);

    nsACString_Init(&name_str, name);
    astr_to_nswstr(body, &body_str);

    nsres =  nsIScriptContainer_CompileFunction(script_container, jsglobal, &name_str, argc, arg_names,
                                                &body_str, NULL, 1, FALSE, &func_obj);

    nsACString_Finish(&name_str);
    nsAString_Finish(&body_str);

    if(NS_FAILED(nsres)) {
        ERR("CompileFunction failed: %08x\n", nsres);
        return nsres;
    }

    nsres = nsIScriptContainer_CallFunction(script_container, target, jsglobal, func_obj, argv, &jsret);

    nsIScriptContainer_DropScriptObject(script_container, func_obj);
    nsIScriptContainer_DropScriptObject(script_container, jsglobal);
    if(NS_FAILED(nsres)) {
        ERR("CallFunction failed: %08x\n", nsres);
        return nsres;
    }

    nsIVariant_Release(jsret);
    return NS_OK;
}

void setup_nswindow(HTMLWindow *This)
{
    nsIScriptContainer *script_container;
    nsIDOMWindow *nswindow;
    nsIDOMDocument *domdoc;
    nsIWritableVariant *nsvar;
    nsIMutableArray *argv;
    nsresult nsres;

    static const char *args[] = {"wineWindow"};

    TRACE("(%p)\n", This);

    nsIWebNavigation_GetDocument(This->doc->nscontainer->navigation, &domdoc);
    nsres = nsIDOMDocument_QueryInterface(domdoc, &IID_nsIScriptContainer, (void**)&script_container);
    nsIDOMDocument_Release(domdoc);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsIDOMScriptContainer: %08x\n", nsres);
        return;
    }

    nsIWebBrowser_GetContentDOMWindow(This->doc->nscontainer->webbrowser, &nswindow);

    nsvar = create_nsvariant();
    nsres = nsIWritableVariant_SetAsInterface(nsvar, &IID_IDispatch, HTMLWINDOW2(This));
    if(NS_FAILED(nsres))
        ERR("SetAsInterface failed: %08x\n", nsres);

    argv = create_nsarray();
    nsres = nsIMutableArray_AppendElement(argv, (nsISupports*)nsvar, FALSE);
    nsIWritableVariant_Release(nsvar);
    if(NS_FAILED(nsres))
        ERR("AppendElement failed: %08x\n", nsres);

    call_js_func(script_container, (nsISupports*)nswindow/*HTMLWINDOW2(This)*/, "wineConfig",
                 wineConfig_func, 1, args, (nsIArray*)argv);

    nsIMutableArray_Release(argv);
    nsIScriptContainer_Release(script_container);
}

HTMLWindow *HTMLWindow_Create(HTMLDocument *doc)
{
    HTMLWindow *ret = heap_alloc_zero(sizeof(HTMLWindow));

    ret->lpHTMLWindow2Vtbl = &HTMLWindow2Vtbl;
    ret->lpHTMLWindow3Vtbl = &HTMLWindow3Vtbl;
    ret->lpIDispatchExVtbl = &WindowDispExVtbl;
    ret->ref = 1;
    ret->doc = doc;

    init_dispex(&ret->dispex, (IUnknown*)HTMLWINDOW2(ret), &HTMLWindow_dispex);

    if(doc->nscontainer) {
        nsresult nsres;

        nsres = nsIWebBrowser_GetContentDOMWindow(doc->nscontainer->webbrowser, &ret->nswindow);
        if(NS_FAILED(nsres))
            ERR("GetContentDOMWindow failed: %08x\n", nsres);
    }

    list_add_head(&window_list, &ret->entry);

    return ret;
}

HTMLWindow *nswindow_to_window(const nsIDOMWindow *nswindow)
{
    HTMLWindow *iter;

    LIST_FOR_EACH_ENTRY(iter, &window_list, HTMLWindow, entry) {
        if(iter->nswindow == nswindow)
            return iter;
    }

    return NULL;
}
