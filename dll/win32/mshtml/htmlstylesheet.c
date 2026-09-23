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
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLStyleSheet {
    DispatchEx dispex;
    IHTMLStyleSheet IHTMLStyleSheet_iface;
    IHTMLStyleSheet4 IHTMLStyleSheet4_iface;

    nsIDOMCSSStyleSheet *nsstylesheet;
};

struct HTMLStyleSheetsCollection {
    DispatchEx dispex;
    IHTMLStyleSheetsCollection IHTMLStyleSheetsCollection_iface;

    nsIDOMStyleSheetList *nslist;
};

typedef struct {
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    ULONG iter;
    HTMLStyleSheetsCollection *col;
} HTMLStyleSheetsCollectionEnum;

struct HTMLStyleSheetRulesCollection {
    DispatchEx dispex;
    IHTMLStyleSheetRulesCollection IHTMLStyleSheetRulesCollection_iface;

    nsIDOMCSSRuleList *nslist;
};

struct HTMLStyleSheetRule {
    DispatchEx dispex;
    IHTMLStyleSheetRule IHTMLStyleSheetRule_iface;

    nsIDOMCSSRule *nsstylesheetrule;
};

static inline HTMLStyleSheetRule *impl_from_IHTMLStyleSheetRule(IHTMLStyleSheetRule *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRule, IHTMLStyleSheetRule_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyleSheetRule, IHTMLStyleSheetRule,
                      impl_from_IHTMLStyleSheetRule(iface)->dispex)

static HRESULT WINAPI HTMLStyleSheetRule_put_selectorText(IHTMLStyleSheetRule *iface, BSTR v)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_selectorText(IHTMLStyleSheetRule *iface, BSTR *p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_style(IHTMLStyleSheetRule *iface, IHTMLRuleStyle **p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheetRule_get_readOnly(IHTMLStyleSheetRule *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheetRule *This = impl_from_IHTMLStyleSheetRule(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyleSheetRuleVtbl HTMLStyleSheetRuleVtbl = {
    HTMLStyleSheetRule_QueryInterface,
    HTMLStyleSheetRule_AddRef,
    HTMLStyleSheetRule_Release,
    HTMLStyleSheetRule_GetTypeInfoCount,
    HTMLStyleSheetRule_GetTypeInfo,
    HTMLStyleSheetRule_GetIDsOfNames,
    HTMLStyleSheetRule_Invoke,
    HTMLStyleSheetRule_put_selectorText,
    HTMLStyleSheetRule_get_selectorText,
    HTMLStyleSheetRule_get_style,
    HTMLStyleSheetRule_get_readOnly
};

static inline HTMLStyleSheetRule *HTMLStyleSheetRule_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRule, dispex);
}

static void *HTMLStyleSheetRule_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStyleSheetRule *This = HTMLStyleSheetRule_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStyleSheetRule, riid))
        return &This->IHTMLStyleSheetRule_iface;

    return NULL;
}

static void HTMLStyleSheetRule_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyleSheetRule *This = HTMLStyleSheetRule_from_DispatchEx(dispex);
    if(This->nsstylesheetrule)
        note_cc_edge((nsISupports*)This->nsstylesheetrule, "nsstylesheetrule", cb);
}

static void HTMLStyleSheetRule_unlink(DispatchEx *dispex)
{
    HTMLStyleSheetRule *This = HTMLStyleSheetRule_from_DispatchEx(dispex);
    unlink_ref(&This->nsstylesheetrule);
}

static void HTMLStyleSheetRule_destructor(DispatchEx *dispex)
{
    HTMLStyleSheetRule *This = HTMLStyleSheetRule_from_DispatchEx(dispex);
    free(This);
}

dispex_static_data_t CSSRule_dispex = {
    .id = PROT_CSSRule,
};

static const dispex_static_data_vtbl_t CSSStyleRule_dispex_vtbl = {
    .query_interface  = HTMLStyleSheetRule_query_interface,
    .destructor       = HTMLStyleSheetRule_destructor,
    .traverse         = HTMLStyleSheetRule_traverse,
    .unlink           = HTMLStyleSheetRule_unlink
};

static const tid_t CSSStyleRule_iface_tids[] = {
    IHTMLStyleSheetRule_tid,
    0
};
dispex_static_data_t CSSStyleRule_dispex = {
    .id           = PROT_CSSStyleRule,
    .prototype_id = PROT_CSSRule,
    .vtbl         = &CSSStyleRule_dispex_vtbl,
    .disp_tid     = DispHTMLStyleSheetRule_tid,
    .iface_tids   = CSSStyleRule_iface_tids,
};

static HRESULT create_style_sheet_rule(nsIDOMCSSRule *nsstylesheetrule, DispatchEx *owner,
                                       IHTMLStyleSheetRule **ret)
{
    HTMLStyleSheetRule *rule;
    nsresult nsres;

    if(!(rule = malloc(sizeof(*rule))))
        return E_OUTOFMEMORY;

    rule->IHTMLStyleSheetRule_iface.lpVtbl = &HTMLStyleSheetRuleVtbl;
    rule->nsstylesheetrule = NULL;

    init_dispatch_with_owner(&rule->dispex, &CSSStyleRule_dispex, owner);

    if (nsstylesheetrule)
    {
        nsres = nsIDOMCSSRule_QueryInterface(nsstylesheetrule, &IID_nsIDOMCSSRule,
                (void **)&rule->nsstylesheetrule);
        if (NS_FAILED(nsres))
            ERR("Could not get nsIDOMCSSRule interface: %08lx\n", nsres);
    }

    *ret = &rule->IHTMLStyleSheetRule_iface;
    return S_OK;
}

static inline HTMLStyleSheetRulesCollection *impl_from_IHTMLStyleSheetRulesCollection(IHTMLStyleSheetRulesCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRulesCollection, IHTMLStyleSheetRulesCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyleSheetRulesCollection, IHTMLStyleSheetRulesCollection,
                      impl_from_IHTMLStyleSheetRulesCollection(iface)->dispex)

static HRESULT WINAPI HTMLStyleSheetRulesCollection_get_length(IHTMLStyleSheetRulesCollection *iface,
        LONG *p)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    UINT32 len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nslist) {
        nsresult nsres;

        nsres = nsIDOMCSSRuleList_GetLength(This->nslist, &len);
        if(NS_FAILED(nsres))
            ERR("GetLength failed: %08lx\n", nsres);
    }

    *p = len;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetRulesCollection_item(IHTMLStyleSheetRulesCollection *iface,
        LONG index, IHTMLStyleSheetRule **p)
{
    HTMLStyleSheetRulesCollection *This = impl_from_IHTMLStyleSheetRulesCollection(iface);
    nsIDOMCSSRule *nsstylesheetrule;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%ld %p)\n", This, index, p);

    nsres = nsIDOMCSSRuleList_Item(This->nslist, index, &nsstylesheetrule);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);
    if(!nsstylesheetrule)
        return E_INVALIDARG;

    hres = create_style_sheet_rule(nsstylesheetrule, &This->dispex, p);
    nsIDOMCSSRule_Release(nsstylesheetrule);
    return hres;
}

static const IHTMLStyleSheetRulesCollectionVtbl HTMLStyleSheetRulesCollectionVtbl = {
    HTMLStyleSheetRulesCollection_QueryInterface,
    HTMLStyleSheetRulesCollection_AddRef,
    HTMLStyleSheetRulesCollection_Release,
    HTMLStyleSheetRulesCollection_GetTypeInfoCount,
    HTMLStyleSheetRulesCollection_GetTypeInfo,
    HTMLStyleSheetRulesCollection_GetIDsOfNames,
    HTMLStyleSheetRulesCollection_Invoke,
    HTMLStyleSheetRulesCollection_get_length,
    HTMLStyleSheetRulesCollection_item
};

static inline HTMLStyleSheetRulesCollection *HTMLStyleSheetRulesCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetRulesCollection, dispex);
}

static void *HTMLStyleSheetRulesCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStyleSheetRulesCollection, riid))
        return &This->IHTMLStyleSheetRulesCollection_iface;

    return NULL;
}

static void HTMLStyleSheetRulesCollection_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);
    if(This->nslist)
        note_cc_edge((nsISupports*)This->nslist, "nslist", cb);
}

static void HTMLStyleSheetRulesCollection_unlink(DispatchEx *dispex)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);
    unlink_ref(&This->nslist);
}

static void HTMLStyleSheetRulesCollection_destructor(DispatchEx *dispex)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);
    free(This);
}

static HRESULT HTMLStyleSheetRulesCollection_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);
    const WCHAR *ptr;
    UINT32 len = 0;
    DWORD idx = 0;

    for(ptr = name; *ptr && is_digit(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    nsIDOMCSSRuleList_GetLength(This->nslist, &len);
    if(idx >= len)
        return DISP_E_UNKNOWNNAME;

    *dispid = MSHTML_DISPID_CUSTOM_MIN + idx;
    TRACE("ret %lx\n", *dispid);
    return S_OK;
}

static HRESULT HTMLStyleSheetRulesCollection_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLStyleSheetRulesCollection *This = HTMLStyleSheetRulesCollection_from_DispatchEx(dispex);

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        IHTMLStyleSheetRule *stylesheetrule;
        nsIDOMCSSRule *nsstylesheetrule;
        nsresult nsres;
        HRESULT hres;

        nsres = nsIDOMCSSRuleList_Item(This->nslist, id - MSHTML_DISPID_CUSTOM_MIN, &nsstylesheetrule);
        if(NS_FAILED(nsres))
            return DISP_E_MEMBERNOTFOUND;
        if(!nsstylesheetrule) {
            V_VT(res) = VT_EMPTY;
            return S_OK;
        }

        hres = create_style_sheet_rule(nsstylesheetrule, &This->dispex, &stylesheetrule);
        nsIDOMCSSRule_Release(nsstylesheetrule);
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)stylesheetrule;
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const dispex_static_data_vtbl_t MSCSSRuleList_dispex_vtbl = {
    .query_interface  = HTMLStyleSheetRulesCollection_query_interface,
    .destructor       = HTMLStyleSheetRulesCollection_destructor,
    .traverse         = HTMLStyleSheetRulesCollection_traverse,
    .unlink           = HTMLStyleSheetRulesCollection_unlink,
    .get_dispid       = HTMLStyleSheetRulesCollection_get_dispid,
    .get_prop_desc    = dispex_index_prop_desc,
    .invoke           = HTMLStyleSheetRulesCollection_invoke
};
static const tid_t MSCSSRuleList_iface_tids[] = {
    IHTMLStyleSheetRulesCollection_tid,
    0
};
dispex_static_data_t MSCSSRuleList_dispex = {
    .id         = PROT_MSCSSRuleList,
    .vtbl       = &MSCSSRuleList_dispex_vtbl,
    .disp_tid   = DispHTMLStyleSheetRulesCollection_tid,
    .iface_tids = MSCSSRuleList_iface_tids,
};

static HRESULT create_style_sheet_rules_collection(nsIDOMCSSRuleList *nslist, DispatchEx *owner,
                                                   IHTMLStyleSheetRulesCollection **ret)
{
    HTMLStyleSheetRulesCollection *collection;

    if(!(collection = malloc(sizeof(*collection))))
        return E_OUTOFMEMORY;

    collection->IHTMLStyleSheetRulesCollection_iface.lpVtbl = &HTMLStyleSheetRulesCollectionVtbl;
    collection->nslist = nslist;

    init_dispatch_with_owner(&collection->dispex, &MSCSSRuleList_dispex, owner);

    if(nslist)
        nsIDOMCSSRuleList_AddRef(nslist);

    *ret = &collection->IHTMLStyleSheetRulesCollection_iface;
    return S_OK;
}

static inline HTMLStyleSheetsCollectionEnum *HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetsCollectionEnum, IEnumVARIANT_iface);
}

static HRESULT WINAPI HTMLStyleSheetsCollectionEnum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else if(IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyleSheetsCollectionEnum_AddRef(IEnumVARIANT *iface)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyleSheetsCollectionEnum_Release(IEnumVARIANT *iface)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IHTMLStyleSheetsCollection_Release(&This->col->IHTMLStyleSheetsCollection_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyleSheetsCollectionEnum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);
    VARIANT index;
    HRESULT hres;
    ULONG num, i;
    UINT32 len;

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

    nsIDOMStyleSheetList_GetLength(This->col->nslist, &len);
    num = min(len - This->iter, celt);
    V_VT(&index) = VT_I4;

    for(i = 0; i < num; i++) {
        V_I4(&index) = This->iter + i;
        hres = IHTMLStyleSheetsCollection_item(&This->col->IHTMLStyleSheetsCollection_iface, &index, &rgVar[i]);
        if(FAILED(hres)) {
            while(i--)
                VariantClear(&rgVar[i]);
            return hres;
        }
    }

    This->iter += num;
    if(pCeltFetched)
        *pCeltFetched = num;
    return num == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI HTMLStyleSheetsCollectionEnum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);
    UINT32 len;

    TRACE("(%p)->(%lu)\n", This, celt);

    nsIDOMStyleSheetList_GetLength(This->col->nslist, &len);
    if(This->iter + celt > len) {
        This->iter = len;
        return S_FALSE;
    }

    This->iter += celt;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollectionEnum_Reset(IEnumVARIANT *iface)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);

    TRACE("(%p)->()\n", This);

    This->iter = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollectionEnum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    HTMLStyleSheetsCollectionEnum *This = HTMLStyleSheetsCollectionEnum_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl HTMLStyleSheetsCollectionEnumVtbl = {
    HTMLStyleSheetsCollectionEnum_QueryInterface,
    HTMLStyleSheetsCollectionEnum_AddRef,
    HTMLStyleSheetsCollectionEnum_Release,
    HTMLStyleSheetsCollectionEnum_Next,
    HTMLStyleSheetsCollectionEnum_Skip,
    HTMLStyleSheetsCollectionEnum_Reset,
    HTMLStyleSheetsCollectionEnum_Clone
};

static inline HTMLStyleSheetsCollection *impl_from_IHTMLStyleSheetsCollection(IHTMLStyleSheetsCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetsCollection, IHTMLStyleSheetsCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyleSheetsCollection, IHTMLStyleSheetsCollection,
                      impl_from_IHTMLStyleSheetsCollection(iface)->dispex)

static HRESULT WINAPI HTMLStyleSheetsCollection_get_length(IHTMLStyleSheetsCollection *iface,
        LONG *p)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    UINT32 len = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nslist)
        nsIDOMStyleSheetList_GetLength(This->nslist, &len);

    *p = len;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_get__newEnum(IHTMLStyleSheetsCollection *iface,
        IUnknown **p)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);
    HTMLStyleSheetsCollectionEnum *ret;

    TRACE("(%p)->(%p)\n", This, p);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumVARIANT_iface.lpVtbl = &HTMLStyleSheetsCollectionEnumVtbl;
    ret->ref = 1;
    ret->iter = 0;

    HTMLStyleSheetsCollection_AddRef(&This->IHTMLStyleSheetsCollection_iface);
    ret->col = This;

    *p = (IUnknown*)&ret->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheetsCollection_item(IHTMLStyleSheetsCollection *iface,
        VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLStyleSheetsCollection *This = impl_from_IHTMLStyleSheetsCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(pvarIndex), pvarResult);

    switch(V_VT(pvarIndex)) {
    case VT_I4: {
        nsIDOMStyleSheet *nsstylesheet;
        IHTMLStyleSheet *stylesheet;
        nsresult nsres;
        HRESULT hres;

        TRACE("index=%ld\n", V_I4(pvarIndex));

        nsres = nsIDOMStyleSheetList_Item(This->nslist, V_I4(pvarIndex), &nsstylesheet);
        if(NS_FAILED(nsres) || !nsstylesheet) {
            WARN("Item failed: %08lx\n", nsres);
            V_VT(pvarResult) = VT_EMPTY;
            return E_INVALIDARG;
        }

        hres = create_style_sheet(nsstylesheet, &This->dispex, &stylesheet);
        nsIDOMStyleSheet_Release(nsstylesheet);
        if(FAILED(hres))
            return hres;

        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = (IDispatch*)stylesheet;
        return S_OK;
    }

    case VT_BSTR:
        FIXME("id=%s not implemented\n", debugstr_w(V_BSTR(pvarResult)));
        return E_NOTIMPL;

    default:
        WARN("Invalid index %s\n", debugstr_variant(pvarIndex));
    }

    return E_INVALIDARG;
}

static const IHTMLStyleSheetsCollectionVtbl HTMLStyleSheetsCollectionVtbl = {
    HTMLStyleSheetsCollection_QueryInterface,
    HTMLStyleSheetsCollection_AddRef,
    HTMLStyleSheetsCollection_Release,
    HTMLStyleSheetsCollection_GetTypeInfoCount,
    HTMLStyleSheetsCollection_GetTypeInfo,
    HTMLStyleSheetsCollection_GetIDsOfNames,
    HTMLStyleSheetsCollection_Invoke,
    HTMLStyleSheetsCollection_get_length,
    HTMLStyleSheetsCollection_get__newEnum,
    HTMLStyleSheetsCollection_item
};

static inline HTMLStyleSheetsCollection *HTMLStyleSheetsCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheetsCollection, dispex);
}

static void *HTMLStyleSheetsCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStyleSheetsCollection, riid))
        return &This->IHTMLStyleSheetsCollection_iface;

    return NULL;
}

static void HTMLStyleSheetsCollection_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);
    if(This->nslist)
        note_cc_edge((nsISupports*)This->nslist, "nslist", cb);
}

static void HTMLStyleSheetsCollection_unlink(DispatchEx *dispex)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);
    unlink_ref(&This->nslist);
}

static void HTMLStyleSheetsCollection_destructor(DispatchEx *dispex)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);
    free(This);
}

static HRESULT HTMLStyleSheetsCollection_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);
    const WCHAR *ptr;
    UINT32 len = 0;
    DWORD idx = 0;

    for(ptr = name; *ptr && is_digit(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    nsIDOMStyleSheetList_GetLength(This->nslist, &len);
    if(idx >= len)
        return DISP_E_UNKNOWNNAME;

    *dispid = MSHTML_DISPID_CUSTOM_MIN + idx;
    TRACE("ret %lx\n", *dispid);
    return S_OK;
}

static HRESULT HTMLStyleSheetsCollection_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLStyleSheetsCollection *This = HTMLStyleSheetsCollection_from_DispatchEx(dispex);

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        nsIDOMStyleSheet *nsstylesheet;
        IHTMLStyleSheet *stylesheet;
        nsresult nsres;
        HRESULT hres;

        nsres = nsIDOMStyleSheetList_Item(This->nslist, id - MSHTML_DISPID_CUSTOM_MIN, &nsstylesheet);
        if(NS_FAILED(nsres))
            return DISP_E_MEMBERNOTFOUND;
        if(!nsstylesheet) {
            V_VT(res) = VT_EMPTY;
            return S_OK;
        }

        hres = create_style_sheet(nsstylesheet, &This->dispex, &stylesheet);
        nsIDOMStyleSheet_Release(nsstylesheet);
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)stylesheet;
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const dispex_static_data_vtbl_t StyleSheetList_dispex_vtbl = {
    .query_interface  = HTMLStyleSheetsCollection_query_interface,
    .destructor       = HTMLStyleSheetsCollection_destructor,
    .traverse         = HTMLStyleSheetsCollection_traverse,
    .unlink           = HTMLStyleSheetsCollection_unlink,
    .get_dispid       = HTMLStyleSheetsCollection_get_dispid,
    .get_prop_desc    = dispex_index_prop_desc,
    .invoke           = HTMLStyleSheetsCollection_invoke
};
static const tid_t StyleSheetList_iface_tids[] = {
    IHTMLStyleSheetsCollection_tid,
    0
};
dispex_static_data_t StyleSheetList_dispex = {
    .id         = PROT_StyleSheetList,
    .vtbl       = &StyleSheetList_dispex_vtbl,
    .disp_tid   = DispHTMLStyleSheetsCollection_tid,
    .iface_tids = StyleSheetList_iface_tids,
};

HRESULT create_style_sheet_collection(nsIDOMStyleSheetList *nslist, HTMLDocumentNode *doc,
                                      IHTMLStyleSheetsCollection **ret)
{
    HTMLStyleSheetsCollection *collection;

    if(!(collection = malloc(sizeof(HTMLStyleSheetsCollection))))
        return E_OUTOFMEMORY;

    collection->IHTMLStyleSheetsCollection_iface.lpVtbl = &HTMLStyleSheetsCollectionVtbl;

    if(nslist)
        nsIDOMStyleSheetList_AddRef(nslist);
    collection->nslist = nslist;

    init_dispatch(&collection->dispex, &StyleSheetList_dispex, doc->script_global,
                  dispex_compat_mode(&doc->node.event_target.dispex));

    *ret = &collection->IHTMLStyleSheetsCollection_iface;
    return S_OK;
}

static inline HTMLStyleSheet *impl_from_IHTMLStyleSheet(IHTMLStyleSheet *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheet, IHTMLStyleSheet_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyleSheet, IHTMLStyleSheet, impl_from_IHTMLStyleSheet(iface)->dispex)

static HRESULT WINAPI HTMLStyleSheet_put_title(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_title(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_parentStyleSheet(IHTMLStyleSheet *iface,
                                                          IHTMLStyleSheet **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_owningElement(IHTMLStyleSheet *iface, IHTMLElement **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_disabled(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_readOnly(IHTMLStyleSheet *iface, VARIANT_BOOL *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_imports(IHTMLStyleSheet *iface,
                                                 IHTMLStyleSheetsCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_href(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_href(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMCSSStyleSheet_GetHref(This->nsstylesheet, &href_str);
    return return_nsstr(nsres, &href_str, p);
}

static HRESULT WINAPI HTMLStyleSheet_get_type(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_id(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addImport(IHTMLStyleSheet *iface, BSTR bstrURL,
                                               LONG lIndex, LONG *plIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s %ld %p)\n", This, debugstr_w(bstrURL), lIndex, plIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_addRule(IHTMLStyleSheet *iface, BSTR bstrSelector,
                                             BSTR bstrStyle, LONG lIndex, LONG *plIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    const WCHAR format[] = L"%s {%s}";
    nsIDOMCSSRuleList *nslist = NULL;
    UINT32 length, new_index;
    nsAString nsstr;
    nsresult nsres;
    WCHAR *rule;
    size_t len;

    TRACE("(%p)->(%s %s %ld %p)\n", This, debugstr_w(bstrSelector), debugstr_w(bstrStyle),
          lIndex, plIndex);

    if(!bstrSelector || !bstrStyle || !bstrSelector[0] || !bstrStyle[0])
        return E_INVALIDARG;

    nsres = nsIDOMCSSStyleSheet_GetCssRules(This->nsstylesheet, &nslist);
    if(NS_FAILED(nsres))
        return E_FAIL;
    nsIDOMCSSRuleList_GetLength(nslist, &length);

    if(lIndex > length)
        lIndex = length;

    len = ARRAY_SIZE(format) - 4 /* %s twice */ + wcslen(bstrSelector) + wcslen(bstrStyle);
    if(!(rule = malloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;
    swprintf(rule, len, format, bstrSelector, bstrStyle);

    nsAString_InitDepend(&nsstr, rule);
    nsres = nsIDOMCSSStyleSheet_InsertRule(This->nsstylesheet, &nsstr, lIndex, &new_index);
    if(NS_FAILED(nsres)) WARN("failed: %08lx\n", nsres);
    nsAString_Finish(&nsstr);
    free(rule);

    *plIndex = new_index;
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLStyleSheet_removeImport(IHTMLStyleSheet *iface, LONG lIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%ld)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_removeRule(IHTMLStyleSheet *iface, LONG lIndex)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%ld)\n", This, lIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_media(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_get_media(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet_put_cssText(IHTMLStyleSheet *iface, BSTR v)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    do {
        nsres = nsIDOMCSSStyleSheet_DeleteRule(This->nsstylesheet, 0);
    }while(NS_SUCCEEDED(nsres));

    if(v && *v) {
        nsAString nsstr;
        UINT32 idx;

        /* FIXME: This won't work for multiple rules in the string. */
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMCSSStyleSheet_InsertRule(This->nsstylesheet, &nsstr, 0, &idx);
        nsAString_Finish(&nsstr);
        if(NS_FAILED(nsres)) {
            FIXME("InsertRule failed for string %s. Probably multiple rules passed.\n", debugstr_w(v));
            return E_FAIL;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyleSheet_get_cssText(IHTMLStyleSheet *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsIDOMCSSRuleList *nslist = NULL;
    nsIDOMCSSRule *nsrule;
    nsAString nsstr;
    UINT32 len;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMCSSStyleSheet_GetCssRules(This->nsstylesheet, &nslist);
    if(NS_FAILED(nsres)) {
        ERR("GetCssRules failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMCSSRuleList_GetLength(nslist, &len);
    assert(nsres == NS_OK);

    if(len) {
        nsres = nsIDOMCSSRuleList_Item(nslist, 0, &nsrule);
        if(NS_FAILED(nsres))
            ERR("Item failed: %08lx\n", nsres);
    }

    nsIDOMCSSRuleList_Release(nslist);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(!len) {
        *p = NULL;
        return S_OK;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMCSSRule_GetCssText(nsrule, &nsstr);
    nsIDOMCSSRule_Release(nsrule);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLStyleSheet_get_rules(IHTMLStyleSheet *iface,
                                               IHTMLStyleSheetRulesCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet(iface);
    nsIDOMCSSRuleList *nslist = NULL;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMCSSStyleSheet_GetCssRules(This->nsstylesheet, &nslist);
    if(NS_FAILED(nsres)) {
        ERR("GetCssRules failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = create_style_sheet_rules_collection(nslist, &This->dispex, p);
    nsIDOMCSSRuleList_Release(nslist);
    return hres;
}

static const IHTMLStyleSheetVtbl HTMLStyleSheetVtbl = {
    HTMLStyleSheet_QueryInterface,
    HTMLStyleSheet_AddRef,
    HTMLStyleSheet_Release,
    HTMLStyleSheet_GetTypeInfoCount,
    HTMLStyleSheet_GetTypeInfo,
    HTMLStyleSheet_GetIDsOfNames,
    HTMLStyleSheet_Invoke,
    HTMLStyleSheet_put_title,
    HTMLStyleSheet_get_title,
    HTMLStyleSheet_get_parentStyleSheet,
    HTMLStyleSheet_get_owningElement,
    HTMLStyleSheet_put_disabled,
    HTMLStyleSheet_get_disabled,
    HTMLStyleSheet_get_readOnly,
    HTMLStyleSheet_get_imports,
    HTMLStyleSheet_put_href,
    HTMLStyleSheet_get_href,
    HTMLStyleSheet_get_type,
    HTMLStyleSheet_get_id,
    HTMLStyleSheet_addImport,
    HTMLStyleSheet_addRule,
    HTMLStyleSheet_removeImport,
    HTMLStyleSheet_removeRule,
    HTMLStyleSheet_put_media,
    HTMLStyleSheet_get_media,
    HTMLStyleSheet_put_cssText,
    HTMLStyleSheet_get_cssText,
    HTMLStyleSheet_get_rules
};

static inline HTMLStyleSheet *impl_from_IHTMLStyleSheet4(IHTMLStyleSheet4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheet, IHTMLStyleSheet4_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyleSheet4, IHTMLStyleSheet4, impl_from_IHTMLStyleSheet4(iface)->dispex)

static HRESULT WINAPI HTMLStyleSheet4_get_type(IHTMLStyleSheet4 *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyleSheet_get_type(&This->IHTMLStyleSheet_iface, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_href(IHTMLStyleSheet4 *iface, VARIANT *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMCSSStyleSheet_GetHref(This->nsstylesheet, &href_str);
    return return_nsstr_variant(nsres, &href_str, 0, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_title(IHTMLStyleSheet4 *iface, BSTR *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_ownerNode(IHTMLStyleSheet4 *iface, IHTMLElement **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_ownerRule(IHTMLStyleSheet4 *iface, IHTMLCSSRule **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_get_cssRules(IHTMLStyleSheet4 *iface, IHTMLStyleSheetRulesCollection **p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyleSheet_get_rules(&This->IHTMLStyleSheet_iface, p);
}

static HRESULT WINAPI HTMLStyleSheet4_get_media(IHTMLStyleSheet4 *iface, VARIANT *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleSheet4_insertRule(IHTMLStyleSheet4 *iface, BSTR rule, LONG index, LONG *p)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    UINT32 new_index = 0;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(rule), index, p);

    nsAString_InitDepend(&nsstr, rule);
    nsres = nsIDOMCSSStyleSheet_InsertRule(This->nsstylesheet, &nsstr, index, &new_index);
    if(NS_FAILED(nsres)) WARN("failed: %08lx\n", nsres);
    nsAString_Finish(&nsstr);
    *p = new_index;
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLStyleSheet4_deleteRule(IHTMLStyleSheet4 *iface, LONG index)
{
    HTMLStyleSheet *This = impl_from_IHTMLStyleSheet4(iface);
    FIXME("(%p)->(%ld)\n", This, index);
    return E_NOTIMPL;
}

static const IHTMLStyleSheet4Vtbl HTMLStyleSheet4Vtbl = {
    HTMLStyleSheet4_QueryInterface,
    HTMLStyleSheet4_AddRef,
    HTMLStyleSheet4_Release,
    HTMLStyleSheet4_GetTypeInfoCount,
    HTMLStyleSheet4_GetTypeInfo,
    HTMLStyleSheet4_GetIDsOfNames,
    HTMLStyleSheet4_Invoke,
    HTMLStyleSheet4_get_type,
    HTMLStyleSheet4_get_href,
    HTMLStyleSheet4_get_title,
    HTMLStyleSheet4_get_ownerNode,
    HTMLStyleSheet4_get_ownerRule,
    HTMLStyleSheet4_get_cssRules,
    HTMLStyleSheet4_get_media,
    HTMLStyleSheet4_insertRule,
    HTMLStyleSheet4_deleteRule,
};

static inline HTMLStyleSheet *HTMLStyleSheet_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleSheet, dispex);
}

static void *HTMLStyleSheet_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStyleSheet *This = HTMLStyleSheet_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStyleSheet, riid))
        return &This->IHTMLStyleSheet_iface;
    if(IsEqualGUID(&IID_IHTMLStyleSheet4, riid))
        return &This->IHTMLStyleSheet4_iface;

    return NULL;
}

static void HTMLStyleSheet_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyleSheet *This = HTMLStyleSheet_from_DispatchEx(dispex);
    if(This->nsstylesheet)
        note_cc_edge((nsISupports*)This->nsstylesheet, "nsstylesheet", cb);
}

static void HTMLStyleSheet_unlink(DispatchEx *dispex)
{
    HTMLStyleSheet *This = HTMLStyleSheet_from_DispatchEx(dispex);
    unlink_ref(&This->nsstylesheet);
}

static void HTMLStyleSheet_destructor(DispatchEx *dispex)
{
    HTMLStyleSheet *This = HTMLStyleSheet_from_DispatchEx(dispex);
    free(This);
}

static void StyleSheet_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const DISPID stylesheet_dispids[] = {
        DISPID_IHTMLSTYLESHEET_PARENTSTYLESHEET,
        DISPID_IHTMLSTYLESHEET_DISABLED,
        DISPID_UNKNOWN
    };
    static const DISPID stylesheet4_dispids[] = {
        DISPID_IHTMLSTYLESHEET4_IE9_TYPE,
        DISPID_IHTMLSTYLESHEET4_IE9_HREF,
        DISPID_IHTMLSTYLESHEET4_IE9_TITLE,
        DISPID_IHTMLSTYLESHEET4_OWNERNODE,
        DISPID_IHTMLSTYLESHEET4_IE9_MEDIA,
        DISPID_UNKNOWN
    };
    dispex_info_add_dispids(info, IHTMLStyleSheet4_tid, stylesheet4_dispids);
    dispex_info_add_dispids(info, IHTMLStyleSheet_tid, stylesheet_dispids);
}

static void HTMLStyleSheet_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    if(mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(info, IHTMLStyleSheet4_tid, NULL);
}

dispex_static_data_t StyleSheet_dispex = {
    .id           = PROT_StyleSheet,
    .init_info    = StyleSheet_init_dispex_info,
};

static const dispex_static_data_vtbl_t CSSStyleSheet_dispex_vtbl = {
    .query_interface  = HTMLStyleSheet_query_interface,
    .destructor       = HTMLStyleSheet_destructor,
    .traverse         = HTMLStyleSheet_traverse,
    .unlink           = HTMLStyleSheet_unlink
};

static const tid_t CSSStyleSheet_iface_tids[] = {
    IHTMLStyleSheet_tid,
    0
};
dispex_static_data_t CSSStyleSheet_dispex = {
    .id           = PROT_CSSStyleSheet,
    .prototype_id = PROT_StyleSheet,
    .vtbl         = &CSSStyleSheet_dispex_vtbl,
    .disp_tid     = DispHTMLStyleSheet_tid,
    .iface_tids   = CSSStyleSheet_iface_tids,
    .init_info    = HTMLStyleSheet_init_dispex_info,
};

HRESULT create_style_sheet(nsIDOMStyleSheet *nsstylesheet, DispatchEx *owner, IHTMLStyleSheet **ret)
{
    HTMLStyleSheet *style_sheet;
    nsresult nsres;

    if(!(style_sheet = malloc(sizeof(HTMLStyleSheet))))
        return E_OUTOFMEMORY;

    style_sheet->IHTMLStyleSheet_iface.lpVtbl = &HTMLStyleSheetVtbl;
    style_sheet->IHTMLStyleSheet4_iface.lpVtbl = &HTMLStyleSheet4Vtbl;
    style_sheet->nsstylesheet = NULL;

    init_dispatch_with_owner(&style_sheet->dispex, &CSSStyleSheet_dispex, owner);

    if(nsstylesheet) {
        nsres = nsIDOMStyleSheet_QueryInterface(nsstylesheet, &IID_nsIDOMCSSStyleSheet,
                (void**)&style_sheet->nsstylesheet);
        if(NS_FAILED(nsres))
            ERR("Could not get nsICSSStyleSheet interface: %08lx\n", nsres);
    }

    *ret = &style_sheet->IHTMLStyleSheet_iface;
    return S_OK;
}
