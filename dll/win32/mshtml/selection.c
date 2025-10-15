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

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    DispatchEx dispex;
    IHTMLSelectionObject IHTMLSelectionObject_iface;
    IHTMLSelectionObject2 IHTMLSelectionObject2_iface;

    LONG ref;

    nsISelection *nsselection;
    HTMLDocumentNode *doc;

    struct list entry;
} HTMLSelectionObject;

static inline HTMLSelectionObject *impl_from_IHTMLSelectionObject(IHTMLSelectionObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectionObject, IHTMLSelectionObject_iface);
}

static HRESULT WINAPI HTMLSelectionObject_QueryInterface(IHTMLSelectionObject *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLSelectionObject_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLSelectionObject_iface;
    }else if(IsEqualGUID(&IID_IHTMLSelectionObject, riid)) {
        *ppv = &This->IHTMLSelectionObject_iface;
    }else if(IsEqualGUID(&IID_IHTMLSelectionObject2, riid)) {
        *ppv = &This->IHTMLSelectionObject2_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLSelectionObject_AddRef(IHTMLSelectionObject *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLSelectionObject_Release(IHTMLSelectionObject *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsselection)
            nsISelection_Release(This->nsselection);
        if(This->doc)
            list_remove(&This->entry);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLSelectionObject_GetTypeInfoCount(IHTMLSelectionObject *iface, UINT *pctinfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);

    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLSelectionObject_GetTypeInfo(IHTMLSelectionObject *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLSelectionObject_GetIDsOfNames(IHTMLSelectionObject *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLSelectionObject_Invoke(IHTMLSelectionObject *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);


    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLSelectionObject_createRange(IHTMLSelectionObject *iface, IDispatch **range)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    IHTMLTxtRange *range_obj = NULL;
    nsIDOMRange *nsrange = NULL;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, range);

    if(This->nsselection) {
        LONG nsrange_cnt = 0;
        nsresult nsres;

        nsISelection_GetRangeCount(This->nsselection, &nsrange_cnt);
        if(!nsrange_cnt) {
            nsIDOMHTMLElement *nsbody = NULL;

            TRACE("nsrange_cnt = 0\n");

            if(!This->doc->nsdoc) {
                WARN("nsdoc is NULL\n");
                return E_UNEXPECTED;
            }

            nsres = nsIDOMHTMLDocument_GetBody(This->doc->nsdoc, &nsbody);
            if(NS_FAILED(nsres) || !nsbody) {
                ERR("Could not get body: %08x\n", nsres);
                return E_FAIL;
            }

            nsres = nsISelection_Collapse(This->nsselection, (nsIDOMNode*)nsbody, 0);
            nsIDOMHTMLElement_Release(nsbody);
            if(NS_FAILED(nsres))
                ERR("Collapse failed: %08x\n", nsres);
        }else if(nsrange_cnt > 1) {
            FIXME("range_cnt = %d\n", nsrange_cnt);
        }

        nsres = nsISelection_GetRangeAt(This->nsselection, 0, &nsrange);
        if(NS_FAILED(nsres))
            ERR("GetRangeAt failed: %08x\n", nsres);
    }

    hres = HTMLTxtRange_Create(This->doc, nsrange, &range_obj);

    if (nsrange) nsIDOMRange_Release(nsrange);
    *range = (IDispatch*)range_obj;
    return hres;
}

static HRESULT WINAPI HTMLSelectionObject_empty(IHTMLSelectionObject *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject_clear(IHTMLSelectionObject *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject_get_type(IHTMLSelectionObject *iface, BSTR *p)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    cpp_bool collapsed = TRUE;

    static const WCHAR wszNone[] = {'N','o','n','e',0};
    static const WCHAR wszText[] = {'T','e','x','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsselection)
        nsISelection_GetIsCollapsed(This->nsselection, &collapsed);

    *p = SysAllocString(collapsed ? wszNone : wszText); /* FIXME: control */
    TRACE("ret %s\n", debugstr_w(*p));
    return S_OK;
}

static const IHTMLSelectionObjectVtbl HTMLSelectionObjectVtbl = {
    HTMLSelectionObject_QueryInterface,
    HTMLSelectionObject_AddRef,
    HTMLSelectionObject_Release,
    HTMLSelectionObject_GetTypeInfoCount,
    HTMLSelectionObject_GetTypeInfo,
    HTMLSelectionObject_GetIDsOfNames,
    HTMLSelectionObject_Invoke,
    HTMLSelectionObject_createRange,
    HTMLSelectionObject_empty,
    HTMLSelectionObject_clear,
    HTMLSelectionObject_get_type
};

static inline HTMLSelectionObject *impl_from_IHTMLSelectionObject2(IHTMLSelectionObject2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectionObject, IHTMLSelectionObject2_iface);
}

static HRESULT WINAPI HTMLSelectionObject2_QueryInterface(IHTMLSelectionObject2 *iface, REFIID riid, void **ppv)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IHTMLSelectionObject_QueryInterface(&This->IHTMLSelectionObject_iface, riid, ppv);
}

static ULONG WINAPI HTMLSelectionObject2_AddRef(IHTMLSelectionObject2 *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IHTMLSelectionObject_AddRef(&This->IHTMLSelectionObject_iface);
}

static ULONG WINAPI HTMLSelectionObject2_Release(IHTMLSelectionObject2 *iface)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IHTMLSelectionObject_Release(&This->IHTMLSelectionObject_iface);
}

static HRESULT WINAPI HTMLSelectionObject2_GetTypeInfoCount(IHTMLSelectionObject2 *iface, UINT *pctinfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLSelectionObject2_GetTypeInfo(IHTMLSelectionObject2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLSelectionObject2_GetIDsOfNames(IHTMLSelectionObject2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLSelectionObject2_Invoke(IHTMLSelectionObject2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLSelectionObject2_createRangeCollection(IHTMLSelectionObject2 *iface, IDispatch **rangeCollection)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);
    FIXME("(%p)->(%p)\n", This, rangeCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject2_get_typeDetail(IHTMLSelectionObject2 *iface, BSTR *p)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLSelectionObject2Vtbl HTMLSelectionObject2Vtbl = {
    HTMLSelectionObject2_QueryInterface,
    HTMLSelectionObject2_AddRef,
    HTMLSelectionObject2_Release,
    HTMLSelectionObject2_GetTypeInfoCount,
    HTMLSelectionObject2_GetTypeInfo,
    HTMLSelectionObject2_GetIDsOfNames,
    HTMLSelectionObject2_Invoke,
    HTMLSelectionObject2_createRangeCollection,
    HTMLSelectionObject2_get_typeDetail
};

static const tid_t HTMLSelectionObject_iface_tids[] = {
    IHTMLSelectionObject_tid,
    IHTMLSelectionObject2_tid,
    0
};
static dispex_static_data_t HTMLSelectionObject_dispex = {
    NULL,
    IHTMLSelectionObject_tid, /* FIXME: We have a test for that, but it doesn't expose IHTMLSelectionObject2 iface. */
    NULL,
    HTMLSelectionObject_iface_tids
};

HRESULT HTMLSelectionObject_Create(HTMLDocumentNode *doc, nsISelection *nsselection, IHTMLSelectionObject **ret)
{
    HTMLSelectionObject *selection;

    selection = heap_alloc(sizeof(HTMLSelectionObject));
    if(!selection)
        return E_OUTOFMEMORY;

    init_dispex(&selection->dispex, (IUnknown*)&selection->IHTMLSelectionObject_iface, &HTMLSelectionObject_dispex);

    selection->IHTMLSelectionObject_iface.lpVtbl = &HTMLSelectionObjectVtbl;
    selection->IHTMLSelectionObject2_iface.lpVtbl = &HTMLSelectionObject2Vtbl;
    selection->ref = 1;
    selection->nsselection = nsselection; /* We shouldn't call AddRef here */

    selection->doc = doc;
    list_add_head(&doc->selection_list, &selection->entry);

    *ret = &selection->IHTMLSelectionObject_iface;
    return S_OK;
}

void detach_selection(HTMLDocumentNode *This)
{
    HTMLSelectionObject *iter;

    LIST_FOR_EACH_ENTRY(iter, &This->selection_list, HTMLSelectionObject, entry) {
        iter->doc = NULL;
    }
}
