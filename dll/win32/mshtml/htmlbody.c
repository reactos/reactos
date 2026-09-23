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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "htmlstyle.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    IHTMLBodyElement IHTMLBodyElement_iface;
    IHTMLTextContainer IHTMLTextContainer_iface;

    nsIDOMHTMLBodyElement *nsbody;
} HTMLBodyElement;

static const struct {
    LPCWSTR keyword;
    DWORD rgb;
} keyword_table[] = {
    {L"aqua",     0x00ffff},
    {L"black",    0x000000},
    {L"blue",     0x0000ff},
    {L"fuchsia",  0xff00ff},
    {L"gray",     0x808080},
    {L"green",    0x008000},
    {L"lime",     0x00ff00},
    {L"maroon",   0x800000},
    {L"navy",     0x000080},
    {L"olive",    0x808000},
    {L"purple",   0x800080},
    {L"red",      0xff0000},
    {L"silver",   0xc0c0c0},
    {L"teal",     0x008080},
    {L"white",    0xffffff},
    {L"yellow",   0xffff00}
};

static int comp_value(const WCHAR *ptr, int dpc)
{
    int ret = 0;
    WCHAR ch;

    if(dpc > 2)
        dpc = 2;

    while(dpc--) {
        if(!*ptr)
            ret *= 16;
        else if(is_digit(ch = *ptr++))
            ret = ret*16 + (ch-'0');
        else if('a' <= ch && ch <= 'f')
            ret = ret*16 + (ch-'a') + 10;
        else if('A' <= ch && ch <= 'F')
            ret = ret*16 + (ch-'A') + 10;
        else
            ret *= 16;
    }

    return ret;
}

/* Based on Gecko NS_LooseHexToRGB */
static int loose_hex_to_rgb(const WCHAR *hex)
{
    int len, dpc;

    len = lstrlenW(hex);
    if(*hex == '#') {
        hex++;
        len--;
    }
    if(len <= 3)
        return 0;

    dpc = min(len/3 + (len%3 ? 1 : 0), 4);
    return (comp_value(hex, dpc) << 16)
        | (comp_value(hex+dpc, dpc) << 8)
        | comp_value(hex+2*dpc, dpc);
}

HRESULT nscolor_to_str(LPCWSTR color, BSTR *ret)
{
    unsigned int i;
    int rgb = -1;

    if(!color || !*color) {
        *ret = NULL;
        return S_OK;
    }

    if(*color != '#') {
        for(i=0; i < ARRAY_SIZE(keyword_table); i++) {
            if(!wcsicmp(color, keyword_table[i].keyword))
                rgb = keyword_table[i].rgb;
        }
    }
    if(rgb == -1)
        rgb = loose_hex_to_rgb(color);

    *ret = SysAllocStringLen(NULL, 7);
    if(!*ret)
        return E_OUTOFMEMORY;

    swprintf(*ret, 8, L"#%02x%02x%02x", rgb>>16, (rgb>>8)&0xff, rgb&0xff);

    TRACE("%s -> %s\n", debugstr_w(color), debugstr_w(*ret));
    return S_OK;
}

BOOL variant_to_nscolor(const VARIANT *v, nsAString *nsstr)
{
    switch(V_VT(v)) {
    case VT_BSTR:
        nsAString_Init(nsstr, V_BSTR(v));
        return TRUE;

    case VT_I4: {
        PRUnichar buf[10];

        wsprintfW(buf, L"#%x", V_I4(v));
        nsAString_Init(nsstr, buf);
        return TRUE;
    }

    default:
        FIXME("invalid color %s\n", debugstr_variant(v));
    }

    return FALSE;

}

static HRESULT return_nscolor(nsresult nsres, nsAString *nsstr, VARIANT *p)
{
    const PRUnichar *color;

    if(NS_FAILED(nsres)) {
        ERR("failed: %08lx\n", nsres);
        nsAString_Finish(nsstr);
        return E_FAIL;
    }

    nsAString_GetData(nsstr, &color);

    if(*color == '#') {
        V_VT(p) = VT_I4;
        V_I4(p) = wcstol(color+1, NULL, 16);
    }else {
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = SysAllocString(color);
        if(!V_BSTR(p)) {
            nsAString_Finish(nsstr);
            return E_OUTOFMEMORY;
        }
    }

    nsAString_Finish(nsstr);
    TRACE("ret %s\n", debugstr_variant(p));
    return S_OK;
}

static inline HTMLBodyElement *impl_from_IHTMLBodyElement(IHTMLBodyElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLBodyElement, IHTMLBodyElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLBodyElement, IHTMLBodyElement,
                      impl_from_IHTMLBodyElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLBodyElement_put_background(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLBodyElement_SetBackground(This->nsbody, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_background(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString background_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&background_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetBackground(This->nsbody, &background_str);
    return return_nsstr(nsres, &background_str, p);
}

static HRESULT WINAPI HTMLBodyElement_put_bgProperties(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bgProperties(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_leftMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_leftMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_topMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_topMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_rightMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_rightMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bottomMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bottomMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bgColor(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString strColor;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &strColor))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetBgColor(This->nsbody, &strColor);
    nsAString_Finish(&strColor);
    if(NS_FAILED(nsres))
        ERR("SetBgColor failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_bgColor(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLBodyElement_GetBgColor(This->nsbody, &nsstr);
    return return_nsstr_variant(nsres, &nsstr, NSSTR_COLOR, p);
}

static HRESULT WINAPI HTMLBodyElement_put_text(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString text;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &text))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetText(This->nsbody, &text);
    nsAString_Finish(&text);
    if(NS_FAILED(nsres)) {
        ERR("SetText failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_text(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString text;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&text, NULL);
    nsres = nsIDOMHTMLBodyElement_GetText(This->nsbody, &text);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *color;

        nsAString_GetData(&text, &color);
        V_VT(p) = VT_BSTR;
        hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
        ERR("GetText failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&text);

    return hres;
}

static HRESULT WINAPI HTMLBodyElement_put_link(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString link_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &link_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetLink(This->nsbody, &link_str);
    nsAString_Finish(&link_str);
    if(NS_FAILED(nsres))
        ERR("SetLink failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_link(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString link_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&link_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetLink(This->nsbody, &link_str);
    return return_nscolor(nsres, &link_str, p);
}

static HRESULT WINAPI HTMLBodyElement_put_vLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString vlink_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &vlink_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetVLink(This->nsbody, &vlink_str);
    nsAString_Finish(&vlink_str);
    if(NS_FAILED(nsres))
        ERR("SetLink failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_vLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString vlink_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&vlink_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetVLink(This->nsbody, &vlink_str);
    return return_nscolor(nsres, &vlink_str, p);
}

static HRESULT WINAPI HTMLBodyElement_put_aLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString alink_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &alink_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetALink(This->nsbody, &alink_str);
    nsAString_Finish(&alink_str);
    if(NS_FAILED(nsres))
        ERR("SetALink failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_aLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString alink_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&alink_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetALink(This->nsbody, &alink_str);
    return return_nscolor(nsres, &alink_str, p);
}

static HRESULT WINAPI HTMLBodyElement_put_onload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLBodyElement_get_onload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLBodyElement_put_onunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_scroll(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    static const WCHAR *val;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* Emulate with CSS visibility attribute */
    if(!wcscmp(v, L"yes")) {
        val = L"scroll";
    }else if(!wcscmp(v, L"auto")) {
        val = L"visible";
    }else if(!wcscmp(v, L"no")) {
        val = L"hidden";
    }else {
        WARN("Invalid argument %s\n", debugstr_w(v));
        return E_INVALIDARG;
    }

    return set_elem_style(&This->element, STYLEID_OVERFLOW, val);
}

static HRESULT WINAPI HTMLBodyElement_get_scroll(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    const WCHAR *ret = NULL;
    BSTR overflow;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    /* Emulate with CSS visibility attribute */
    hres = get_elem_style(&This->element, STYLEID_OVERFLOW, &overflow);
    if(FAILED(hres))
        return hres;

    if(!overflow || !*overflow) {
        *p = NULL;
        hres = S_OK;
    }else if(!wcscmp(overflow, L"visible") || !wcscmp(overflow, L"auto")) {
        ret = L"auto";
    }else if(!wcscmp(overflow, L"scroll")) {
        ret = L"yes";
    }else if(!wcscmp(overflow, L"hidden")) {
        ret = L"no";
    }else {
        TRACE("Defaulting %s to NULL\n", debugstr_w(overflow));
        *p = NULL;
        hres = S_OK;
    }

    SysFreeString(overflow);
    if(ret) {
        *p = SysAllocString(ret);
        hres = *p ? S_OK : E_OUTOFMEMORY;
    }

    return hres;
}

static HRESULT WINAPI HTMLBodyElement_put_onselect(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onselect(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onbeforeunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onbeforeunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_createTextRange(IHTMLBodyElement *iface, IHTMLTxtRange **range)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsIDOMRange *nsrange = NULL;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, range);

    if(!This->element.node.doc->dom_document) {
        WARN("No dom_document\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_CreateRange(This->element.node.doc->dom_document, &nsrange);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIDOMRange_SelectNodeContents(nsrange, This->element.node.nsnode);
        if(NS_FAILED(nsres))
            ERR("SelectNodeContents failed: %08lx\n", nsres);
    }else {
        ERR("CreateRange failed: %08lx\n", nsres);
    }

    hres = HTMLTxtRange_Create(This->element.node.doc, nsrange, range);

    nsIDOMRange_Release(nsrange);
    return hres;
}

static const IHTMLBodyElementVtbl HTMLBodyElementVtbl = {
    HTMLBodyElement_QueryInterface,
    HTMLBodyElement_AddRef,
    HTMLBodyElement_Release,
    HTMLBodyElement_GetTypeInfoCount,
    HTMLBodyElement_GetTypeInfo,
    HTMLBodyElement_GetIDsOfNames,
    HTMLBodyElement_Invoke,
    HTMLBodyElement_put_background,
    HTMLBodyElement_get_background,
    HTMLBodyElement_put_bgProperties,
    HTMLBodyElement_get_bgProperties,
    HTMLBodyElement_put_leftMargin,
    HTMLBodyElement_get_leftMargin,
    HTMLBodyElement_put_topMargin,
    HTMLBodyElement_get_topMargin,
    HTMLBodyElement_put_rightMargin,
    HTMLBodyElement_get_rightMargin,
    HTMLBodyElement_put_bottomMargin,
    HTMLBodyElement_get_bottomMargin,
    HTMLBodyElement_put_noWrap,
    HTMLBodyElement_get_noWrap,
    HTMLBodyElement_put_bgColor,
    HTMLBodyElement_get_bgColor,
    HTMLBodyElement_put_text,
    HTMLBodyElement_get_text,
    HTMLBodyElement_put_link,
    HTMLBodyElement_get_link,
    HTMLBodyElement_put_vLink,
    HTMLBodyElement_get_vLink,
    HTMLBodyElement_put_aLink,
    HTMLBodyElement_get_aLink,
    HTMLBodyElement_put_onload,
    HTMLBodyElement_get_onload,
    HTMLBodyElement_put_onunload,
    HTMLBodyElement_get_onunload,
    HTMLBodyElement_put_scroll,
    HTMLBodyElement_get_scroll,
    HTMLBodyElement_put_onselect,
    HTMLBodyElement_get_onselect,
    HTMLBodyElement_put_onbeforeunload,
    HTMLBodyElement_get_onbeforeunload,
    HTMLBodyElement_createTextRange
};

static inline HTMLBodyElement *impl_from_IHTMLTextContainer(IHTMLTextContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLBodyElement, IHTMLTextContainer_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLTextContainer, IHTMLTextContainer,
                      impl_from_IHTMLTextContainer(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLTextContainer_createControlRange(IHTMLTextContainer *iface,
                                                           IDispatch **range)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_scrollHeight(IHTMLTextContainer *iface, LONG *p)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollHeight(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollWidth(IHTMLTextContainer *iface, LONG *p)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollWidth(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollTop(IHTMLTextContainer *iface, LONG v)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLElement2_put_scrollTop(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollTop(IHTMLTextContainer *iface, LONG *p)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollTop(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollLeft(IHTMLTextContainer *iface, LONG v)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLElement2_put_scrollLeft(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollLeft(IHTMLTextContainer *iface, LONG *p)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollLeft(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_onscroll(IHTMLTextContainer *iface, VARIANT v)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_onscroll(IHTMLTextContainer *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLTextContainerVtbl HTMLTextContainerVtbl = {
    HTMLTextContainer_QueryInterface,
    HTMLTextContainer_AddRef,
    HTMLTextContainer_Release,
    HTMLTextContainer_GetTypeInfoCount,
    HTMLTextContainer_GetTypeInfo,
    HTMLTextContainer_GetIDsOfNames,
    HTMLTextContainer_Invoke,
    HTMLTextContainer_createControlRange,
    HTMLTextContainer_get_scrollHeight,
    HTMLTextContainer_get_scrollWidth,
    HTMLTextContainer_put_scrollTop,
    HTMLTextContainer_get_scrollTop,
    HTMLTextContainer_put_scrollLeft,
    HTMLTextContainer_get_scrollLeft,
    HTMLTextContainer_put_onscroll,
    HTMLTextContainer_get_onscroll
};

static inline HTMLBodyElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLBodyElement, element.node);
}

static EventTarget *HTMLBodyElement_get_event_prop_target(HTMLDOMNode *iface, int event_id)
{
    HTMLBodyElement *This = impl_from_HTMLDOMNode(iface);

    switch(event_id) {
    case EVENTID_BLUR:
    case EVENTID_ERROR:
    case EVENTID_FOCUS:
    case EVENTID_LOAD:
    case EVENTID_SCROLL:
        return This->element.node.doc && This->element.node.doc->window
            ? &This->element.node.doc->window->event_target
            : &This->element.node.event_target;
    default:
        return &This->element.node.event_target;
    }
}

static BOOL HTMLBodyElement_is_text_edit(HTMLDOMNode *iface)
{
    return TRUE;
}

static BOOL HTMLBodyElement_is_settable(HTMLDOMNode *iface, DISPID dispid)
{
    switch(dispid) {
    case DISPID_IHTMLELEMENT_OUTERTEXT:
        return FALSE;
    default:
        return TRUE;
    }
}

static inline HTMLBodyElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLBodyElement, element.node.event_target.dispex);
}

static void *HTMLBodyElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLBodyElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLBodyElement, riid))
        return &This->IHTMLBodyElement_iface;
    if(IsEqualGUID(&IID_IHTMLTextContainer, riid))
        return &This->IHTMLTextContainer_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLBodyElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLBodyElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsbody)
        note_cc_edge((nsISupports*)This->nsbody, "nsbody", cb);
}

static void HTMLBodyElement_unlink(DispatchEx *dispex)
{
    HTMLBodyElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsbody);
}

static const cpc_entry_t HTMLBodyElement_cpc[] = {
    {&DIID_HTMLTextContainerEvents},
    {&IID_IPropertyNotifySink},
    HTMLELEMENT_CPC,
    {NULL}
};

static const NodeImplVtbl HTMLBodyElementImplVtbl = {
    .clsid                 = &CLSID_HTMLBody,
    .cpc_entries           = HTMLBodyElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .get_event_prop_target = HTMLBodyElement_get_event_prop_target,
    .is_text_edit          = HTMLBodyElement_is_text_edit,
    .is_settable           = HTMLBodyElement_is_settable
};

static const event_target_vtbl_t HTMLBodyElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLBodyElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLBodyElement_traverse,
        .unlink         = HTMLBodyElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLBodyElement_iface_tids[] = {
    IHTMLBodyElement_tid,
    IHTMLBodyElement2_tid,
    IHTMLTextContainer_tid,
    0
};

dispex_static_data_t HTMLBodyElement_dispex = {
    .id           = PROT_HTMLBodyElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLBodyElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLBody_tid,
    .iface_tids   = HTMLBodyElement_iface_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLBodyElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLBodyElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLBodyElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLBodyElement_iface.lpVtbl = &HTMLBodyElementVtbl;
    ret->IHTMLTextContainer_iface.lpVtbl = &HTMLTextContainerVtbl;
    ret->element.node.vtbl = &HTMLBodyElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLBodyElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLBodyElement, (void**)&ret->nsbody);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
