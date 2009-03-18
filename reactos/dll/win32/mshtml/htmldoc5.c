/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLDOC5_THIS(iface) DEFINE_THIS(HTMLDocument, HTMLDocument5, iface)

static HRESULT WINAPI HTMLDocument5_QueryInterface(IHTMLDocument5 *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI HTMLDocument5_AddRef(IHTMLDocument5 *iface)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI HTMLDocument5_Release(IHTMLDocument5 *iface)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfoCount(IHTMLDocument5 *iface, UINT *pctinfo)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfo(IHTMLDocument5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument5_GetIDsOfNames(IHTMLDocument5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDocument5_Invoke(IHTMLDocument5 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument5_put_onmousewheel(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onmousewheel(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_doctype(IHTMLDocument5 *iface, IHTMLDOMNode **p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_implementation(IHTMLDocument5 *iface, IHTMLDOMImplementation **p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_createAttribute(IHTMLDocument5 *iface, BSTR bstrattrName,
        IHTMLDOMAttribute **ppattribute)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrattrName), ppattribute);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_createComment(IHTMLDocument5 *iface, BSTR bstrdata,
        IHTMLDOMNode **ppRetNode)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    nsIDOMComment *nscomment;
    HTMLDOMNode *node;
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrdata), ppRetNode);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&str, bstrdata);
    nsres = nsIDOMHTMLDocument_CreateComment(This->nsdoc, &str, &nscomment);
    nsAString_Finish(&str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    node = &HTMLCommentElement_Create(This, (nsIDOMNode*)nscomment)->node;
    nsIDOMElement_Release(nscomment);

    *ppRetNode = HTMLDOMNODE(node);
    IHTMLDOMNode_AddRef(HTMLDOMNODE(node));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusin(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onfocusin(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusout(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onfocusout(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_ondeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_ondeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_compatMode(IHTMLDocument5 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC5_THIS(iface);
    nsIDOMNSHTMLDocument *nshtmldoc;
    nsAString mode_str;
    const PRUnichar *mode;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_QueryInterface(This->nsdoc, &IID_nsIDOMNSHTMLDocument, (void**)&nshtmldoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLDocument: %08x\n", nsres);
        return S_OK;
    }

    nsAString_Init(&mode_str, NULL);
    nsIDOMNSHTMLDocument_GetCompatMode(nshtmldoc, &mode_str);
    nsIDOMNSHTMLDocument_Release(nshtmldoc);

    nsAString_GetData(&mode_str, &mode);
    *p = SysAllocString(mode);
    nsAString_Finish(&mode_str);

    return S_OK;
}

#undef HTMLDOC5_THIS

static const IHTMLDocument5Vtbl HTMLDocument5Vtbl = {
    HTMLDocument5_QueryInterface,
    HTMLDocument5_AddRef,
    HTMLDocument5_Release,
    HTMLDocument5_GetTypeInfoCount,
    HTMLDocument5_GetTypeInfo,
    HTMLDocument5_GetIDsOfNames,
    HTMLDocument5_Invoke,
    HTMLDocument5_put_onmousewheel,
    HTMLDocument5_get_onmousewheel,
    HTMLDocument5_get_doctype,
    HTMLDocument5_get_implementation,
    HTMLDocument5_createAttribute,
    HTMLDocument5_createComment,
    HTMLDocument5_put_onfocusin,
    HTMLDocument5_get_onfocusin,
    HTMLDocument5_put_onfocusout,
    HTMLDocument5_get_onfocusout,
    HTMLDocument5_put_onactivate,
    HTMLDocument5_get_onactivate,
    HTMLDocument5_put_ondeactivate,
    HTMLDocument5_get_ondeactivate,
    HTMLDocument5_put_onbeforeactivate,
    HTMLDocument5_get_onbeforeactivate,
    HTMLDocument5_put_onbeforedeactivate,
    HTMLDocument5_get_onbeforedeactivate,
    HTMLDocument5_get_compatMode
};

void HTMLDocument_HTMLDocument5_Init(HTMLDocument *This)
{
    This->lpHTMLDocument5Vtbl = &HTMLDocument5Vtbl;
}
