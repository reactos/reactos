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

#include "mshtml_private.h"

struct HTMLTable {
    HTMLElement element;

    IHTMLTable  IHTMLTable_iface;
    IHTMLTable2 IHTMLTable2_iface;
    IHTMLTable3 IHTMLTable3_iface;

    nsIDOMHTMLTableElement *nstable;
};

static inline HTMLTable *impl_from_IHTMLTable(IHTMLTable *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable_iface);
}

static inline HTMLTable *impl_from_IHTMLTable2(IHTMLTable2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable2_iface);
}

static inline HTMLTable *impl_from_IHTMLTable3(IHTMLTable3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, IHTMLTable3_iface);
}

static HRESULT var2str(const VARIANT *p, nsAString *nsstr)
{
    BSTR str;
    BOOL ret;
    HRESULT hres;

    switch(V_VT(p)) {
    case VT_BSTR:
        return nsAString_Init(nsstr, V_BSTR(p))?
            S_OK : E_OUTOFMEMORY;
    case VT_R8:
        hres = VarBstrFromR8(V_R8(p), 0, 0, &str);
        break;
    case VT_R4:
        hres = VarBstrFromR4(V_R4(p), 0, 0, &str);
        break;
    case VT_I4:
        hres = VarBstrFromI4(V_I4(p), 0, 0, &str);
        break;
    default:
        FIXME("unsupported arg %s\n", debugstr_variant(p));
        return E_NOTIMPL;
    }
    if (FAILED(hres))
        return hres;

    ret = nsAString_Init(nsstr, str);
    SysFreeString(str);
    return ret ? S_OK : E_OUTOFMEMORY;
}

static HRESULT nsstr_to_truncated_bstr(const nsAString *nsstr, BSTR *ret_ptr)
{
    const PRUnichar *str, *ptr, *end = NULL;
    BSTR ret;

    nsAString_GetData(nsstr, &str);

    for(ptr = str; isdigitW(*ptr); ptr++);
    if(*ptr == '.') {
        for(end = ptr++; isdigitW(*ptr); ptr++);
        if(*ptr)
            end = NULL;
    }

    ret = end ? SysAllocStringLen(str, end-str) : SysAllocString(str);

    *ret_ptr = ret;
    return ret ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLTable_QueryInterface(IHTMLTable *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable_AddRef(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable_Release(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable_GetTypeInfoCount(IHTMLTable *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable_GetTypeInfo(IHTMLTable *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable_GetIDsOfNames(IHTMLTable *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable_Invoke(IHTMLTable *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable_put_cols(IHTMLTable *iface, LONG v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_cols(IHTMLTable *iface, LONG *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_border(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_border(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_frame(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLTableElement_SetFrame(This->nstable, &str);
    nsAString_Finish(&str);

    if (NS_FAILED(nsres)) {
        ERR("SetFrame(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_frame(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableElement_GetFrame(This->nstable, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLTable_put_rules(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rules(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_cellSpacing(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString nsstr;
    WCHAR buf[64];
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        nsAString_InitDepend(&nsstr, V_BSTR(&v));
        break;
    case VT_I4: {
        static const WCHAR formatW[] = {'%','d',0};
        sprintfW(buf, formatW, V_I4(&v));
        nsAString_InitDepend(&nsstr, buf);
        break;
    }
    default:
        FIXME("unsupported arg %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLTableElement_SetCellSpacing(This->nstable, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetCellSpacing failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_cellSpacing(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLTableElement_GetCellSpacing(This->nstable, &nsstr);
    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static HRESULT WINAPI HTMLTable_put_cellPadding(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var2str(&v, &val);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLTableElement_SetCellPadding(This->nstable, &val);
    nsAString_Finish(&val);
    if(NS_FAILED(nsres)) {
        ERR("Set Width(%s) failed, err = %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_cellPadding(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetCellPadding(This->nstable, &val);
    return return_nsstr_variant(nsres, &val, p);
}

static HRESULT WINAPI HTMLTable_put_background(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_background(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_bgColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &val))
        return S_OK;

    nsres = nsIDOMHTMLTableElement_SetBgColor(This->nstable, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set BgColor(%s) failed!\n", debugstr_variant(&v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_bgColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;
    const PRUnichar *color;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableElement_GetBgColor(This->nstable, &strColor);

    if(NS_SUCCEEDED(nsres)) {
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

static HRESULT WINAPI HTMLTable_put_borderColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorLight(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorLight(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorDark(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorDark(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_align(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableElement_SetAlign(This->nstable, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_align(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetAlign(This->nstable, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTable_refresh(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rows(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLCollection *nscol;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableElement_GetRows(This->nstable, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetRows failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(This->element.node.doc, nscol);

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTable_put_width(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    hres = var2str(&v, &val);

    if (FAILED(hres)){
        ERR("Set Width(%s) failed when initializing a nsAString, err = %08x\n",
            debugstr_variant(&v), hres);
        return hres;
    }

    nsres = nsIDOMHTMLTableElement_SetWidth(This->nstable, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set Width(%s) failed, err = %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_width(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsAString val;
    BSTR bstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);
    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableElement_GetWidth(This->nstable, &val);
    if (NS_FAILED(nsres)){
        ERR("Get Width failed!\n");
        nsAString_Finish(&val);
        return E_FAIL;
    }

    hres = nsstr_to_truncated_bstr(&val, &bstr);
    nsAString_Finish(&val);

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = bstr;
    return hres;
}

static HRESULT WINAPI HTMLTable_put_height(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_height(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_dataPageSize(IHTMLTable *iface, LONG v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_dataPageSize(IHTMLTable *iface, LONG *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_nextPage(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_previousPage(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tHead(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tFoot(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tBodies(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLCollection *nscol = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableElement_GetTBodies(This->nstable, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetTBodies failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(This->element.node.doc, nscol);

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_caption(IHTMLTable *iface, IHTMLTableCaption **p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTHead(IHTMLTable *iface, IDispatch **head)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, head);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTHead(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTFoot(IHTMLTable *iface, IDispatch **foot)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, foot);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTFoot(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createCaption(IHTMLTable *iface, IHTMLTableCaption **caption)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, caption);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteCaption(IHTMLTable *iface)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_insertRow(IHTMLTable *iface, LONG index, IDispatch **row)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsIDOMHTMLElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, index, row);
    nsres = nsIDOMHTMLTableElement_InsertRow(This->nstable, index, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("Insert Row at %d failed: %08x\n", index, nsres);
        return E_FAIL;
    }

    hres = HTMLTableRow_Create(This->element.node.doc, nselem, &elem);
    nsIDOMHTMLElement_Release(nselem);
    if (FAILED(hres)) {
        ERR("Create TableRow failed: %08x\n", hres);
        return hres;
    }

    *row = (IDispatch *)&elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLTable_deleteRow(IHTMLTable *iface, LONG index)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, index);
    nsres = nsIDOMHTMLTableElement_DeleteRow(This->nstable, index);
    if(NS_FAILED(nsres)) {
        ERR("Delete Row failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable_get_readyState(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_onreadystatechange(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_onreadystatechange(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = impl_from_IHTMLTable(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLTableVtbl HTMLTableVtbl = {
    HTMLTable_QueryInterface,
    HTMLTable_AddRef,
    HTMLTable_Release,
    HTMLTable_GetTypeInfoCount,
    HTMLTable_GetTypeInfo,
    HTMLTable_GetIDsOfNames,
    HTMLTable_Invoke,
    HTMLTable_put_cols,
    HTMLTable_get_cols,
    HTMLTable_put_border,
    HTMLTable_get_border,
    HTMLTable_put_frame,
    HTMLTable_get_frame,
    HTMLTable_put_rules,
    HTMLTable_get_rules,
    HTMLTable_put_cellSpacing,
    HTMLTable_get_cellSpacing,
    HTMLTable_put_cellPadding,
    HTMLTable_get_cellPadding,
    HTMLTable_put_background,
    HTMLTable_get_background,
    HTMLTable_put_bgColor,
    HTMLTable_get_bgColor,
    HTMLTable_put_borderColor,
    HTMLTable_get_borderColor,
    HTMLTable_put_borderColorLight,
    HTMLTable_get_borderColorLight,
    HTMLTable_put_borderColorDark,
    HTMLTable_get_borderColorDark,
    HTMLTable_put_align,
    HTMLTable_get_align,
    HTMLTable_refresh,
    HTMLTable_get_rows,
    HTMLTable_put_width,
    HTMLTable_get_width,
    HTMLTable_put_height,
    HTMLTable_get_height,
    HTMLTable_put_dataPageSize,
    HTMLTable_get_dataPageSize,
    HTMLTable_nextPage,
    HTMLTable_previousPage,
    HTMLTable_get_tHead,
    HTMLTable_get_tFoot,
    HTMLTable_get_tBodies,
    HTMLTable_get_caption,
    HTMLTable_createTHead,
    HTMLTable_deleteTHead,
    HTMLTable_createTFoot,
    HTMLTable_deleteTFoot,
    HTMLTable_createCaption,
    HTMLTable_deleteCaption,
    HTMLTable_insertRow,
    HTMLTable_deleteRow,
    HTMLTable_get_readyState,
    HTMLTable_put_onreadystatechange,
    HTMLTable_get_onreadystatechange
};

/* IHTMLTable2 */
static HRESULT WINAPI HTMLTable2_QueryInterface(IHTMLTable2 *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable2_AddRef(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable2_Release(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable2_GetTypeInfoCount(IHTMLTable2 *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable2_GetTypeInfo(IHTMLTable2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable2_GetIDsOfNames(IHTMLTable2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable2_Invoke(IHTMLTable2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable2_firstPage(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_lastPage(IHTMLTable2 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_cells(IHTMLTable2 *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable2_moveRow(IHTMLTable2 *iface, LONG indexFrom, LONG indexTo, IDispatch **row)
{
    HTMLTable *This = impl_from_IHTMLTable2(iface);
    FIXME("(%p)->(%d %d %p)\n", This, indexFrom, indexTo, row);
    return E_NOTIMPL;
}


static const IHTMLTable2Vtbl HTMLTable2Vtbl = {
    HTMLTable2_QueryInterface,
    HTMLTable2_AddRef,
    HTMLTable2_Release,
    HTMLTable2_GetTypeInfoCount,
    HTMLTable2_GetTypeInfo,
    HTMLTable2_GetIDsOfNames,
    HTMLTable2_Invoke,
    HTMLTable2_firstPage,
    HTMLTable2_lastPage,
    HTMLTable2_cells,
    HTMLTable2_moveRow
};

/* IHTMLTable3 */
static HRESULT WINAPI HTMLTable3_QueryInterface(IHTMLTable3 *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTable3_AddRef(IHTMLTable3 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTable3_Release(IHTMLTable3 *iface)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTable3_GetTypeInfoCount(IHTMLTable3 *iface, UINT *pctinfo)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTable3_GetTypeInfo(IHTMLTable3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTable3_GetIDsOfNames(IHTMLTable3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTable3_Invoke(IHTMLTable3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTable3_put_summary(IHTMLTable3 *iface, BSTR v)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLTableElement_SetSummary(This->nstable, &str);

    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set summary(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTable3_get_summary(IHTMLTable3 *iface, BSTR * p)
{
    HTMLTable *This = impl_from_IHTMLTable3(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableElement_GetSummary(This->nstable, &str);

    return return_nsstr(nsres, &str, p);
}

static const IHTMLTable3Vtbl HTMLTable3Vtbl = {
    HTMLTable3_QueryInterface,
    HTMLTable3_AddRef,
    HTMLTable3_Release,
    HTMLTable3_GetTypeInfoCount,
    HTMLTable3_GetTypeInfo,
    HTMLTable3_GetIDsOfNames,
    HTMLTable3_Invoke,
    HTMLTable3_put_summary,
    HTMLTable3_get_summary
};

static inline HTMLTable *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTable, element.node);
}

static HRESULT HTMLTable_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable, riid)) {
        TRACE("(%p)->(IID_IHTMLTable %p)\n", This, ppv);
        *ppv = &This->IHTMLTable_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable2, riid)) {
        TRACE("(%p)->(IID_IHTMLTable2 %p)\n", This, ppv);
        *ppv = &This->IHTMLTable2_iface;
    }else if(IsEqualGUID(&IID_IHTMLTable3, riid)) {
        TRACE("(%p)->(IID_IHTMLTable3 %p)\n", This, ppv);
        *ppv = &This->IHTMLTable3_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTable_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    if(This->nstable)
        note_cc_edge((nsISupports*)This->nstable, "This->nstable", cb);
}

static void HTMLTable_unlink(HTMLDOMNode *iface)
{
    HTMLTable *This = impl_from_HTMLDOMNode(iface);

    if(This->nstable) {
        nsIDOMHTMLTableElement *nstable = This->nstable;

        This->nstable = NULL;
        nsIDOMHTMLTableElement_Release(nstable);
    }
}

static const cpc_entry_t HTMLTable_cpc[] = {
    {&DIID_HTMLTableEvents},
    HTMLELEMENT_CPC,
    {NULL}
};

static const NodeImplVtbl HTMLTableImplVtbl = {
    HTMLTable_QI,
    HTMLElement_destructor,
    HTMLTable_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTable_traverse,
    HTMLTable_unlink
};

static const tid_t HTMLTable_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTable_tid,
    IHTMLTable2_tid,
    IHTMLTable3_tid,
    0
};

static dispex_static_data_t HTMLTable_dispex = {
    NULL,
    DispHTMLTable_tid,
    NULL,
    HTMLTable_iface_tids
};

HRESULT HTMLTable_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLTable *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLTable));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->element.node.vtbl = &HTMLTableImplVtbl;
    ret->IHTMLTable_iface.lpVtbl = &HTMLTableVtbl;
    ret->IHTMLTable2_iface.lpVtbl = &HTMLTable2Vtbl;
    ret->IHTMLTable3_iface.lpVtbl = &HTMLTable3Vtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTable_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableElement, (void**)&ret->nstable);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
