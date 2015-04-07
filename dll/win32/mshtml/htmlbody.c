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

#include "mshtml_private.h"

typedef struct {
    HTMLTextContainer textcont;

    IHTMLBodyElement IHTMLBodyElement_iface;

    nsIDOMHTMLBodyElement *nsbody;
} HTMLBodyElement;

static const WCHAR aquaW[] = {'a','q','u','a',0};
static const WCHAR blackW[] = {'b','l','a','c','k',0};
static const WCHAR blueW[] = {'b','l','u','e',0};
static const WCHAR fuchsiaW[] = {'f','u','s','h','s','i','a',0};
static const WCHAR grayW[] = {'g','r','a','y',0};
static const WCHAR greenW[] = {'g','r','e','e','n',0};
static const WCHAR limeW[] = {'l','i','m','e',0};
static const WCHAR maroonW[] = {'m','a','r','o','o','n',0};
static const WCHAR navyW[] = {'n','a','v','y',0};
static const WCHAR oliveW[] = {'o','l','i','v','e',0};
static const WCHAR purpleW[] = {'p','u','r','p','l','e',0};
static const WCHAR redW[] = {'r','e','d',0};
static const WCHAR silverW[] = {'s','i','l','v','e','r',0};
static const WCHAR tealW[] = {'t','e','a','l',0};
static const WCHAR whiteW[] = {'w','h','i','t','e',0};
static const WCHAR yellowW[] = {'y','e','l','l','o','w',0};

static const struct {
    LPCWSTR keyword;
    DWORD rgb;
} keyword_table[] = {
    {aquaW,     0x00ffff},
    {blackW,    0x000000},
    {blueW,     0x0000ff},
    {fuchsiaW,  0xff00ff},
    {grayW,     0x808080},
    {greenW,    0x008000},
    {limeW,     0x00ff00},
    {maroonW,   0x800000},
    {navyW,     0x000080},
    {oliveW,    0x808000},
    {purpleW,   0x800080},
    {redW,      0xff0000},
    {silverW,   0xc0c0c0},
    {tealW,     0x008080},
    {whiteW,    0xffffff},
    {yellowW,   0xffff00}
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
        else if(isdigitW(ch = *ptr++))
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

    len = strlenW(hex);
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

    static const WCHAR formatW[] = {'#','%','0','2','x','%','0','2','x','%','0','2','x',0};

    if(!color || !*color) {
        *ret = NULL;
        return S_OK;
    }

    if(*color != '#') {
        for(i=0; i < sizeof(keyword_table)/sizeof(keyword_table[0]); i++) {
            if(!strcmpiW(color, keyword_table[i].keyword))
                rgb = keyword_table[i].rgb;
        }
    }
    if(rgb == -1)
        rgb = loose_hex_to_rgb(color);

    *ret = SysAllocStringLen(NULL, 7);
    if(!*ret)
        return E_OUTOFMEMORY;

    sprintfW(*ret, formatW, rgb>>16, (rgb>>8)&0xff, rgb&0xff);

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
        static const WCHAR formatW[] = {'#','%','x',0};

        wsprintfW(buf, formatW, V_I4(v));
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
        ERR("failed: %08x\n", nsres);
        nsAString_Finish(nsstr);
        return E_FAIL;
    }

    nsAString_GetData(nsstr, &color);

    if(*color == '#') {
        V_VT(p) = VT_I4;
        V_I4(p) = strtolW(color+1, NULL, 16);
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

static HRESULT WINAPI HTMLBodyElement_QueryInterface(IHTMLBodyElement *iface,
                                                     REFIID riid, void **ppv)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->textcont.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLBodyElement_AddRef(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    return IHTMLDOMNode_AddRef(&This->textcont.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLBodyElement_Release(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    return IHTMLDOMNode_Release(&This->textcont.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfoCount(IHTMLBodyElement *iface, UINT *pctinfo)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->textcont.element.node.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfo(IHTMLBodyElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    return IDispatchEx_GetTypeInfo(&This->textcont.element.node.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLBodyElement_GetIDsOfNames(IHTMLBodyElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->textcont.element.node.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLBodyElement_Invoke(IHTMLBodyElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    return IDispatchEx_Invoke(&This->textcont.element.node.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

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
        ERR("SetBgColor failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_bgColor(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLBodyElement_GetBgColor(This->nsbody, &strColor);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *color;

        nsAString_GetData(&strColor, &color);
        V_VT(p) = VT_BSTR;
        hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
        ERR("SetBgColor failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&strColor);
    return hres;
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
        ERR("SetText failed: %08x\n", nsres);
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
        ERR("GetText failed: %08x\n", nsres);
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
        ERR("SetLink failed: %08x\n", nsres);

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
        ERR("SetLink failed: %08x\n", nsres);

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
        ERR("SetALink failed: %08x\n", nsres);

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

    return set_node_event(&This->textcont.element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLBodyElement_get_onload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->textcont.element.node, EVENTID_LOAD, p);
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

static const WCHAR autoW[] = {'a','u','t','o',0};
static const WCHAR hiddenW[] = {'h','i','d','d','e','n',0};
static const WCHAR scrollW[] = {'s','c','r','o','l','l',0};
static const WCHAR visibleW[] = {'v','i','s','i','b','l','e',0};
static const WCHAR yesW[] = {'y','e','s',0};
static const WCHAR noW[] = {'n','o',0};

static HRESULT WINAPI HTMLBodyElement_put_scroll(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    static const WCHAR *val;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* Emulate with CSS visibility attribute */
    if(!strcmpW(v, yesW)) {
        val = scrollW;
    }else if(!strcmpW(v, autoW)) {
        val = visibleW;
    }else if(!strcmpW(v, noW)) {
        val = hiddenW;
    }else {
        WARN("Invalid argument %s\n", debugstr_w(v));
        return E_INVALIDARG;
    }

    return set_elem_style(&This->textcont.element, STYLEID_OVERFLOW, val);
}

static HRESULT WINAPI HTMLBodyElement_get_scroll(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = impl_from_IHTMLBodyElement(iface);
    const WCHAR *ret = NULL;
    BSTR overflow;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    /* Emulate with CSS visibility attribute */
    hres = get_elem_style(&This->textcont.element, STYLEID_OVERFLOW, &overflow);
    if(FAILED(hres))
        return hres;

    if(!overflow || !*overflow) {
        *p = NULL;
        hres = S_OK;
    }else if(!strcmpW(overflow, visibleW) || !strcmpW(overflow, autoW)) {
        ret = autoW;
    }else if(!strcmpW(overflow, scrollW)) {
        ret = yesW;
    }else if(!strcmpW(overflow, hiddenW)) {
        ret = noW;
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

    if(!This->textcont.element.node.doc->nsdoc) {
        WARN("No nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_CreateRange(This->textcont.element.node.doc->nsdoc, &nsrange);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIDOMRange_SelectNodeContents(nsrange, This->textcont.element.node.nsnode);
        if(NS_FAILED(nsres))
            ERR("SelectNodeContents failed: %08x\n", nsres);
    }else {
        ERR("CreateRange failed: %08x\n", nsres);
    }

    hres = HTMLTxtRange_Create(This->textcont.element.node.doc->basedoc.doc_node, nsrange, range);

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

static inline HTMLBodyElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLBodyElement, textcont.element.node);
}

static HRESULT HTMLBodyElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLBodyElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLBodyElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLBodyElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLBodyElement, riid)) {
        TRACE("(%p)->(IID_IHTMLBodyElement %p)\n", This, ppv);
        *ppv = &This->IHTMLBodyElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLTextContainer, riid)) {
        TRACE("(%p)->(IID_IHTMLTextContainer %p)\n", &This->textcont, ppv);
        *ppv = &This->textcont.IHTMLTextContainer_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->textcont.element.node, riid, ppv);
}

static void HTMLBodyElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLBodyElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsbody)
        note_cc_edge((nsISupports*)This->nsbody, "This->nsbody", cb);
}

static void HTMLBodyElement_unlink(HTMLDOMNode *iface)
{
    HTMLBodyElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsbody) {
        nsIDOMHTMLBodyElement *nsbody = This->nsbody;
        This->nsbody = NULL;
        nsIDOMHTMLBodyElement_Release(nsbody);
    }
}

static event_target_t **HTMLBodyElement_get_event_target(HTMLDOMNode *iface)
{
    HTMLBodyElement *This = impl_from_HTMLDOMNode(iface);

    return This->textcont.element.node.doc
        ? &This->textcont.element.node.doc->body_event_target
        : &This->textcont.element.node.event_target;
}

static BOOL HTMLBodyElement_is_text_edit(HTMLDOMNode *iface)
{
    return TRUE;
}

static const cpc_entry_t HTMLBodyElement_cpc[] = {
    {&DIID_HTMLTextContainerEvents},
    {&IID_IPropertyNotifySink},
    HTMLELEMENT_CPC,
    {NULL}
};

static const NodeImplVtbl HTMLBodyElementImplVtbl = {
    HTMLBodyElement_QI,
    HTMLElement_destructor,
    HTMLBodyElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    HTMLBodyElement_get_event_target,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLBodyElement_traverse,
    HTMLBodyElement_unlink,
    HTMLBodyElement_is_text_edit
};

static const tid_t HTMLBodyElement_iface_tids[] = {
    IHTMLBodyElement_tid,
    IHTMLBodyElement2_tid,
    HTMLELEMENT_TIDS,
    IHTMLTextContainer_tid,
    IHTMLUniqueName_tid,
    0
};

static dispex_static_data_t HTMLBodyElement_dispex = {
    NULL,
    DispHTMLBody_tid,
    NULL,
    HTMLBodyElement_iface_tids
};

HRESULT HTMLBodyElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLBodyElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLBodyElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLBodyElement_iface.lpVtbl = &HTMLBodyElementVtbl;
    ret->textcont.element.node.vtbl = &HTMLBodyElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLBodyElement, (void**)&ret->nsbody);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsDOMHTMLBodyElement: %08x\n", nsres);
        heap_free(ret);
        return E_OUTOFMEMORY;
    }

    HTMLTextContainer_Init(&ret->textcont, doc, nselem, &HTMLBodyElement_dispex);

    *elem = &ret->textcont.element;
    return S_OK;
}
