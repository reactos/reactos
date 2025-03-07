/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#ifdef __REACTOS__
/* HACK This is a Vista+ API */
static INT WINAPI LCIDToLocaleName_( LCID lcid, LPWSTR name, INT count, DWORD flags )
{
    if (flags) FIXME( "unsupported flags %x\n", flags );

    return GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, name, count );
}
#define LCIDToLocaleName LCIDToLocaleName_
#endif

typedef struct HTMLPluginsCollection HTMLPluginsCollection;
typedef struct HTMLMimeTypesCollection HTMLMimeTypesCollection;

typedef struct {
    DispatchEx dispex;
    IOmNavigator IOmNavigator_iface;

    LONG ref;

    HTMLPluginsCollection *plugins;
    HTMLMimeTypesCollection *mime_types;
} OmNavigator;

typedef struct {
    DispatchEx dispex;
    IHTMLDOMImplementation IHTMLDOMImplementation_iface;

    LONG ref;
} HTMLDOMImplementation;

static inline HTMLDOMImplementation *impl_from_IHTMLDOMImplementation(IHTMLDOMImplementation *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, IHTMLDOMImplementation_iface);
}

static HRESULT WINAPI HTMLDOMImplementation_QueryInterface(IHTMLDOMImplementation *iface, REFIID riid, void **ppv)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IHTMLDOMImplementation, riid)) {
        *ppv = &This->IHTMLDOMImplementation_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLDOMImplementation_AddRef(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDOMImplementation_Release(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDOMImplementation_GetTypeInfoCount(IHTMLDOMImplementation *iface, UINT *pctinfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMImplementation_GetTypeInfo(IHTMLDOMImplementation *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMImplementation_GetIDsOfNames(IHTMLDOMImplementation *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMImplementation_Invoke(IHTMLDOMImplementation *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMImplementation_hasFeature(IHTMLDOMImplementation *iface, BSTR feature,
        VARIANT version, VARIANT_BOOL *pfHasFeature)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation(iface);

    FIXME("(%p)->(%s %s %p) returning false\n", This, debugstr_w(feature), debugstr_variant(&version), pfHasFeature);

    *pfHasFeature = VARIANT_FALSE;
    return S_OK;
}

static const IHTMLDOMImplementationVtbl HTMLDOMImplementationVtbl = {
    HTMLDOMImplementation_QueryInterface,
    HTMLDOMImplementation_AddRef,
    HTMLDOMImplementation_Release,
    HTMLDOMImplementation_GetTypeInfoCount,
    HTMLDOMImplementation_GetTypeInfo,
    HTMLDOMImplementation_GetIDsOfNames,
    HTMLDOMImplementation_Invoke,
    HTMLDOMImplementation_hasFeature
};

static const tid_t HTMLDOMImplementation_iface_tids[] = {
    IHTMLDOMImplementation_tid,
    0
};
static dispex_static_data_t HTMLDOMImplementation_dispex = {
    NULL,
    IHTMLDOMImplementation_tid,
    NULL,
    HTMLDOMImplementation_iface_tids
};

HRESULT create_dom_implementation(IHTMLDOMImplementation **ret)
{
    HTMLDOMImplementation *dom_implementation;

    dom_implementation = heap_alloc_zero(sizeof(*dom_implementation));
    if(!dom_implementation)
        return E_OUTOFMEMORY;

    dom_implementation->IHTMLDOMImplementation_iface.lpVtbl = &HTMLDOMImplementationVtbl;
    dom_implementation->ref = 1;

    init_dispex(&dom_implementation->dispex, (IUnknown*)&dom_implementation->IHTMLDOMImplementation_iface,
            &HTMLDOMImplementation_dispex);

    *ret = &dom_implementation->IHTMLDOMImplementation_iface;
    return S_OK;
}

static inline OmHistory *impl_from_IOmHistory(IOmHistory *iface)
{
    return CONTAINING_RECORD(iface, OmHistory, IOmHistory_iface);
}

static HRESULT WINAPI OmHistory_QueryInterface(IOmHistory *iface, REFIID riid, void **ppv)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOmHistory_iface;
    }else if(IsEqualGUID(&IID_IOmHistory, riid)) {
        *ppv = &This->IOmHistory_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OmHistory_AddRef(IOmHistory *iface)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OmHistory_Release(IOmHistory *iface)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI OmHistory_GetTypeInfoCount(IOmHistory *iface, UINT *pctinfo)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_GetTypeInfo(IOmHistory *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI OmHistory_GetIDsOfNames(IOmHistory *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI OmHistory_Invoke(IOmHistory *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI OmHistory_get_length(IOmHistory *iface, short *p)
{
    OmHistory *This = impl_from_IOmHistory(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window || !This->window->base.outer_window->doc_obj
            || !This->window->base.outer_window->doc_obj->travel_log) {
        *p = 0;
    }else {
        *p = ITravelLog_CountEntries(This->window->base.outer_window->doc_obj->travel_log,
                This->window->base.outer_window->doc_obj->browser_service);
    }
    return S_OK;
}

static HRESULT WINAPI OmHistory_back(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_forward(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static HRESULT WINAPI OmHistory_go(IOmHistory *iface, VARIANT *pvargdistance)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(pvargdistance));
    return E_NOTIMPL;
}

static const IOmHistoryVtbl OmHistoryVtbl = {
    OmHistory_QueryInterface,
    OmHistory_AddRef,
    OmHistory_Release,
    OmHistory_GetTypeInfoCount,
    OmHistory_GetTypeInfo,
    OmHistory_GetIDsOfNames,
    OmHistory_Invoke,
    OmHistory_get_length,
    OmHistory_back,
    OmHistory_forward,
    OmHistory_go
};

static const tid_t OmHistory_iface_tids[] = {
    IOmHistory_tid,
    0
};
static dispex_static_data_t OmHistory_dispex = {
    NULL,
    DispHTMLHistory_tid,
    NULL,
    OmHistory_iface_tids
};


HRESULT create_history(HTMLInnerWindow *window, OmHistory **ret)
{
    OmHistory *history;

    history = heap_alloc_zero(sizeof(*history));
    if(!history)
        return E_OUTOFMEMORY;

    init_dispex(&history->dispex, (IUnknown*)&history->IOmHistory_iface, &OmHistory_dispex);
    history->IOmHistory_iface.lpVtbl = &OmHistoryVtbl;
    history->ref = 1;

    history->window = window;

    *ret = history;
    return S_OK;
}

struct HTMLPluginsCollection {
    DispatchEx dispex;
    IHTMLPluginsCollection IHTMLPluginsCollection_iface;

    LONG ref;

    OmNavigator *navigator;
};

static inline HTMLPluginsCollection *impl_from_IHTMLPluginsCollection(IHTMLPluginsCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLPluginsCollection, IHTMLPluginsCollection_iface);
}

static HRESULT WINAPI HTMLPluginsCollection_QueryInterface(IHTMLPluginsCollection *iface, REFIID riid, void **ppv)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLPluginsCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLPluginsCollection, riid)) {
        *ppv = &This->IHTMLPluginsCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLPluginsCollection_AddRef(IHTMLPluginsCollection *iface)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLPluginsCollection_Release(IHTMLPluginsCollection *iface)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->navigator)
            This->navigator->plugins = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLPluginsCollection_GetTypeInfoCount(IHTMLPluginsCollection *iface, UINT *pctinfo)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLPluginsCollection_GetTypeInfo(IHTMLPluginsCollection *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLPluginsCollection_GetIDsOfNames(IHTMLPluginsCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLPluginsCollection_Invoke(IHTMLPluginsCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLPluginsCollection_get_length(IHTMLPluginsCollection *iface, LONG *p)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* IE always returns 0 here */
    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLPluginsCollection_refresh(IHTMLPluginsCollection *iface, VARIANT_BOOL reload)
{
    HTMLPluginsCollection *This = impl_from_IHTMLPluginsCollection(iface);

    TRACE("(%p)->(%x)\n", This, reload);

    /* Nothing to do here. */
    return S_OK;
}

static const IHTMLPluginsCollectionVtbl HTMLPluginsCollectionVtbl = {
    HTMLPluginsCollection_QueryInterface,
    HTMLPluginsCollection_AddRef,
    HTMLPluginsCollection_Release,
    HTMLPluginsCollection_GetTypeInfoCount,
    HTMLPluginsCollection_GetTypeInfo,
    HTMLPluginsCollection_GetIDsOfNames,
    HTMLPluginsCollection_Invoke,
    HTMLPluginsCollection_get_length,
    HTMLPluginsCollection_refresh
};

static const tid_t HTMLPluginsCollection_iface_tids[] = {
    IHTMLPluginsCollection_tid,
    0
};
static dispex_static_data_t HTMLPluginsCollection_dispex = {
    NULL,
    DispCPlugins_tid,
    NULL,
    HTMLPluginsCollection_iface_tids
};

static HRESULT create_plugins_collection(OmNavigator *navigator, HTMLPluginsCollection **ret)
{
    HTMLPluginsCollection *col;

    col = heap_alloc_zero(sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLPluginsCollection_iface.lpVtbl = &HTMLPluginsCollectionVtbl;
    col->ref = 1;
    col->navigator = navigator;

    init_dispex(&col->dispex, (IUnknown*)&col->IHTMLPluginsCollection_iface,
                &HTMLPluginsCollection_dispex);

    *ret = col;
    return S_OK;
}

struct HTMLMimeTypesCollection {
    DispatchEx dispex;
    IHTMLMimeTypesCollection IHTMLMimeTypesCollection_iface;

    LONG ref;

    OmNavigator *navigator;
};

static inline HTMLMimeTypesCollection *impl_from_IHTMLMimeTypesCollection(IHTMLMimeTypesCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLMimeTypesCollection, IHTMLMimeTypesCollection_iface);
}

static HRESULT WINAPI HTMLMimeTypesCollection_QueryInterface(IHTMLMimeTypesCollection *iface, REFIID riid, void **ppv)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLMimeTypesCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLMimeTypesCollection, riid)) {
        *ppv = &This->IHTMLMimeTypesCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLMimeTypesCollection_AddRef(IHTMLMimeTypesCollection *iface)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLMimeTypesCollection_Release(IHTMLMimeTypesCollection *iface)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->navigator)
            This->navigator->mime_types = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetTypeInfoCount(IHTMLMimeTypesCollection *iface, UINT *pctinfo)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetTypeInfo(IHTMLMimeTypesCollection *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLMimeTypesCollection_GetIDsOfNames(IHTMLMimeTypesCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLMimeTypesCollection_Invoke(IHTMLMimeTypesCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLMimeTypesCollection_get_length(IHTMLMimeTypesCollection *iface, LONG *p)
{
    HTMLMimeTypesCollection *This = impl_from_IHTMLMimeTypesCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* This is just a stub for compatibility with other browser in IE */
    *p = 0;
    return S_OK;
}

static const IHTMLMimeTypesCollectionVtbl HTMLMimeTypesCollectionVtbl = {
    HTMLMimeTypesCollection_QueryInterface,
    HTMLMimeTypesCollection_AddRef,
    HTMLMimeTypesCollection_Release,
    HTMLMimeTypesCollection_GetTypeInfoCount,
    HTMLMimeTypesCollection_GetTypeInfo,
    HTMLMimeTypesCollection_GetIDsOfNames,
    HTMLMimeTypesCollection_Invoke,
    HTMLMimeTypesCollection_get_length
};

static const tid_t HTMLMimeTypesCollection_iface_tids[] = {
    IHTMLMimeTypesCollection_tid,
    0
};
static dispex_static_data_t HTMLMimeTypesCollection_dispex = {
    NULL,
    IHTMLMimeTypesCollection_tid,
    NULL,
    HTMLMimeTypesCollection_iface_tids
};

static HRESULT create_mime_types_collection(OmNavigator *navigator, HTMLMimeTypesCollection **ret)
{
    HTMLMimeTypesCollection *col;

    col = heap_alloc_zero(sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLMimeTypesCollection_iface.lpVtbl = &HTMLMimeTypesCollectionVtbl;
    col->ref = 1;
    col->navigator = navigator;

    init_dispex(&col->dispex, (IUnknown*)&col->IHTMLMimeTypesCollection_iface,
                &HTMLMimeTypesCollection_dispex);

    *ret = col;
    return S_OK;
}

static inline OmNavigator *impl_from_IOmNavigator(IOmNavigator *iface)
{
    return CONTAINING_RECORD(iface, OmNavigator, IOmNavigator_iface);
}

static HRESULT WINAPI OmNavigator_QueryInterface(IOmNavigator *iface, REFIID riid, void **ppv)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOmNavigator_iface;
    }else if(IsEqualGUID(&IID_IOmNavigator, riid)) {
        *ppv = &This->IOmNavigator_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("Unsupported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OmNavigator_AddRef(IOmNavigator *iface)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OmNavigator_Release(IOmNavigator *iface)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->plugins)
            This->plugins->navigator = NULL;
        if(This->mime_types)
            This->mime_types->navigator = NULL;
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI OmNavigator_GetTypeInfoCount(IOmNavigator *iface, UINT *pctinfo)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_GetTypeInfo(IOmNavigator *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI OmNavigator_GetIDsOfNames(IOmNavigator *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI OmNavigator_Invoke(IOmNavigator *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI OmNavigator_get_appCodeName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    static const WCHAR mozillaW[] = {'M','o','z','i','l','l','a',0};

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(mozillaW);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    static const WCHAR app_nameW[] =
        {'M','i','c','r','o','s','o','f','t',' ',
         'I','n','t','e','r','n','e','t',' ',
         'E','x','p','l','o','r','e','r',0};

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(app_nameW);
    if(!*p)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    char user_agent[512];
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    size = sizeof(user_agent);
    hres = ObtainUserAgentString(0, user_agent, &size);
    if(FAILED(hres))
        return hres;

    if(strncmp(user_agent, "Mozilla/", 8)) {
        FIXME("Unsupported user agent\n");
        return E_FAIL;
    }

    size = MultiByteToWideChar(CP_ACP, 0, user_agent+8, -1, NULL, 0);
    *p = SysAllocStringLen(NULL, size-1);
    if(!*p)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, user_agent+8, -1, *p, size);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_userAgent(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    char user_agent[512];
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    size = sizeof(user_agent);
    hres = ObtainUserAgentString(0, user_agent, &size);
    if(FAILED(hres))
        return hres;

    size = MultiByteToWideChar(CP_ACP, 0, user_agent, -1, NULL, 0);
    *p = SysAllocStringLen(NULL, size-1);
    if(!*p)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, user_agent, -1, *p, size);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_javaEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    FIXME("(%p)->(%p) semi-stub\n", This, enabled);

    *enabled = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_taintEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_mimeTypes(IOmNavigator *iface, IHTMLMimeTypesCollection **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->mime_types) {
        HRESULT hres;

        hres = create_mime_types_collection(This, &This->mime_types);
        if(FAILED(hres))
            return hres;
    }else {
        IHTMLMimeTypesCollection_AddRef(&This->mime_types->IHTMLMimeTypesCollection_iface);
    }

    *p = &This->mime_types->IHTMLMimeTypesCollection_iface;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_plugins(IOmNavigator *iface, IHTMLPluginsCollection **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->plugins) {
        HRESULT hres;

        hres = create_plugins_collection(This, &This->plugins);
        if(FAILED(hres))
            return hres;
    }else {
        IHTMLPluginsCollection_AddRef(&This->plugins->IHTMLPluginsCollection_iface);
    }

    *p = &This->plugins->IHTMLPluginsCollection_iface;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_cookieEnabled(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    WARN("(%p)->(%p) semi-stub\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_opsProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_toString(IOmNavigator *iface, BSTR *String)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    static const WCHAR objectW[] = {'[','o','b','j','e','c','t',']',0};

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    *String = SysAllocString(objectW);
    return *String ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI OmNavigator_get_cpuClass(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    static const WCHAR cpu_classW[] =
#ifdef _WIN64
        {'x','6','4',0};
#else
        {'x','8','6',0};
#endif

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(cpu_classW);
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT get_language_string(LCID lcid, BSTR *p)
{
    BSTR ret;
    int len;

    len = LCIDToLocaleName(lcid, NULL, 0, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %u\n", GetLastError());
        return E_FAIL;
    }

    ret = SysAllocStringLen(NULL, len-1);
    if(!ret)
        return E_OUTOFMEMORY;

    len = LCIDToLocaleName(lcid, ret, len, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %u\n", GetLastError());
        SysFreeString(ret);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_systemLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(LOCALE_SYSTEM_DEFAULT, p);
}

static HRESULT WINAPI OmNavigator_get_browserLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(GetUserDefaultUILanguage(), p);
}

static HRESULT WINAPI OmNavigator_get_userLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_language_string(LOCALE_USER_DEFAULT, p);
}

static HRESULT WINAPI OmNavigator_get_platform(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

#ifdef _WIN64
    static const WCHAR platformW[] = {'W','i','n','6','4',0};
#else
    static const WCHAR platformW[] = {'W','i','n','3','2',0};
#endif

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(platformW);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appMinorVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    static const WCHAR zeroW[] = {'0',0};

    TRACE("(%p)->(%p)\n", This, p);

    /* NOTE: MSIE returns "0" or values like ";SP2;". Returning "0" should be enough. */
    *p = SysAllocString(zeroW);
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_connectionSpeed(IOmNavigator *iface, LONG *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_onLine(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    WARN("(%p)->(%p) semi-stub, returning true\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_userProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IOmNavigatorVtbl OmNavigatorVtbl = {
    OmNavigator_QueryInterface,
    OmNavigator_AddRef,
    OmNavigator_Release,
    OmNavigator_GetTypeInfoCount,
    OmNavigator_GetTypeInfo,
    OmNavigator_GetIDsOfNames,
    OmNavigator_Invoke,
    OmNavigator_get_appCodeName,
    OmNavigator_get_appName,
    OmNavigator_get_appVersion,
    OmNavigator_get_userAgent,
    OmNavigator_javaEnabled,
    OmNavigator_taintEnabled,
    OmNavigator_get_mimeTypes,
    OmNavigator_get_plugins,
    OmNavigator_get_cookieEnabled,
    OmNavigator_get_opsProfile,
    OmNavigator_toString,
    OmNavigator_get_cpuClass,
    OmNavigator_get_systemLanguage,
    OmNavigator_get_browserLanguage,
    OmNavigator_get_userLanguage,
    OmNavigator_get_platform,
    OmNavigator_get_appMinorVersion,
    OmNavigator_get_connectionSpeed,
    OmNavigator_get_onLine,
    OmNavigator_get_userProfile
};

static const tid_t OmNavigator_iface_tids[] = {
    IOmNavigator_tid,
    0
};
static dispex_static_data_t OmNavigator_dispex = {
    NULL,
    DispHTMLNavigator_tid,
    NULL,
    OmNavigator_iface_tids
};

IOmNavigator *OmNavigator_Create(void)
{
    OmNavigator *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->IOmNavigator_iface.lpVtbl = &OmNavigatorVtbl;
    ret->ref = 1;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IOmNavigator_iface, &OmNavigator_dispex);

    return &ret->IOmNavigator_iface;
}
