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
    IHTMLSelectionObject IHTMLSelectionObject_iface;

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

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectionObject_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectionObject_iface;
    }else if(IsEqualGUID(&IID_IHTMLSelectionObject, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectionObject_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
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
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLSelectionObject_GetTypeInfoCount(IHTMLSelectionObject *iface, UINT *pctinfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject_GetTypeInfo(IHTMLSelectionObject *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject_GetIDsOfNames(IHTMLSelectionObject *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectionObject_Invoke(IHTMLSelectionObject *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectionObject *This = impl_from_IHTMLSelectionObject(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
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

HRESULT HTMLSelectionObject_Create(HTMLDocumentNode *doc, nsISelection *nsselection, IHTMLSelectionObject **ret)
{
    HTMLSelectionObject *selection;

    selection = heap_alloc(sizeof(HTMLSelectionObject));
    if(!selection)
        return E_OUTOFMEMORY;

    selection->IHTMLSelectionObject_iface.lpVtbl = &HTMLSelectionObjectVtbl;
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
