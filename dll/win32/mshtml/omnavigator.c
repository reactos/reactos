/*
 * Copyright 2008,2009 Jacek Caban for CodeWeavers
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
#include "mshtmdid.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct HTMLPluginsCollection HTMLPluginsCollection;
typedef struct HTMLMimeTypesCollection HTMLMimeTypesCollection;

typedef struct {
    DispatchEx dispex;
    IOmNavigator IOmNavigator_iface;

    HTMLPluginsCollection *plugins;
    HTMLMimeTypesCollection *mime_types;
} OmNavigator;

typedef struct {
    DispatchEx dispex;
    IHTMLDOMImplementation IHTMLDOMImplementation_iface;
    IHTMLDOMImplementation2 IHTMLDOMImplementation2_iface;

    nsIDOMDOMImplementation *implementation;
    HTMLDocumentNode *doc;
} HTMLDOMImplementation;

static inline HTMLDOMImplementation *impl_from_IHTMLDOMImplementation(IHTMLDOMImplementation *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, IHTMLDOMImplementation_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMImplementation, IHTMLDOMImplementation,
                      impl_from_IHTMLDOMImplementation(iface)->dispex)

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

static inline HTMLDOMImplementation *impl_from_IHTMLDOMImplementation2(IHTMLDOMImplementation2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, IHTMLDOMImplementation2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMImplementation2, IHTMLDOMImplementation2,
                      impl_from_IHTMLDOMImplementation2(iface)->dispex)

static HRESULT WINAPI HTMLDOMImplementation2_createDocumentType(IHTMLDOMImplementation2 *iface, BSTR name,
        VARIANT *public_id, VARIANT *system_id, IDOMDocumentType **new_type)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    FIXME("(%p)->(%s %s %s %p)\n", This, debugstr_w(name), debugstr_variant(public_id),
          debugstr_variant(system_id), new_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMImplementation2_createDocument(IHTMLDOMImplementation2 *iface, VARIANT *ns,
        VARIANT *tag_name, IDOMDocumentType *document_type, IHTMLDocument7 **new_document)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    FIXME("(%p)->(%s %s %p %p)\n", This, debugstr_variant(ns), debugstr_variant(tag_name),
          document_type, new_document);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMImplementation2_createHTMLDocument(IHTMLDOMImplementation2 *iface, BSTR title,
        IHTMLDocument7 **new_document)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);
    HTMLDocumentNode *new_document_node;
    nsIDOMDocument *doc;
    nsAString title_str;
    nsresult nsres;
    HRESULT hres;

    FIXME("(%p)->(%s %p)\n", This, debugstr_w(title), new_document);

    if(!This->doc || !This->doc->browser)
        return E_UNEXPECTED;

    nsAString_InitDepend(&title_str, title);
    nsres = nsIDOMDOMImplementation_CreateHTMLDocument(This->implementation, &title_str, &doc);
    nsAString_Finish(&title_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateHTMLDocument failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = create_document_node(doc, This->doc->browser, NULL, This->doc->script_global,
                                dispex_compat_mode(&This->dispex), &new_document_node);
    nsIDOMDocument_Release(doc);
    if(FAILED(hres))
        return hres;

    *new_document = &new_document_node->IHTMLDocument7_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMImplementation2_hasFeature(IHTMLDOMImplementation2 *iface, BSTR feature,
        VARIANT version, VARIANT_BOOL *pfHasFeature)
{
    HTMLDOMImplementation *This = impl_from_IHTMLDOMImplementation2(iface);

    FIXME("(%p)->(%s %s %p) returning false\n", This, debugstr_w(feature), debugstr_variant(&version), pfHasFeature);

    *pfHasFeature = VARIANT_FALSE;
    return S_OK;
}

static const IHTMLDOMImplementation2Vtbl HTMLDOMImplementation2Vtbl = {
    HTMLDOMImplementation2_QueryInterface,
    HTMLDOMImplementation2_AddRef,
    HTMLDOMImplementation2_Release,
    HTMLDOMImplementation2_GetTypeInfoCount,
    HTMLDOMImplementation2_GetTypeInfo,
    HTMLDOMImplementation2_GetIDsOfNames,
    HTMLDOMImplementation2_Invoke,
    HTMLDOMImplementation2_createDocumentType,
    HTMLDOMImplementation2_createDocument,
    HTMLDOMImplementation2_createHTMLDocument,
    HTMLDOMImplementation2_hasFeature
};

static inline HTMLDOMImplementation *HTMLDOMImplementation_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMImplementation, dispex);
}

static void *HTMLDOMImplementation_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDOMImplementation *This = HTMLDOMImplementation_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLDOMImplementation, riid))
        return &This->IHTMLDOMImplementation_iface;
    if(IsEqualGUID(&IID_IHTMLDOMImplementation2, riid))
        return &This->IHTMLDOMImplementation2_iface;

    return NULL;
}

static void HTMLDOMImplementation_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDOMImplementation *This = HTMLDOMImplementation_from_DispatchEx(dispex);
    if(This->implementation)
        note_cc_edge((nsISupports*)This->implementation, "implementation", cb);
}

static void HTMLDOMImplementation_unlink(DispatchEx *dispex)
{
    HTMLDOMImplementation *This = HTMLDOMImplementation_from_DispatchEx(dispex);
    unlink_ref(&This->implementation);
}

static void HTMLDOMImplementation_destructor(DispatchEx *dispex)
{
    HTMLDOMImplementation *This = HTMLDOMImplementation_from_DispatchEx(dispex);
    assert(!This->doc);
    free(This);
}

static const dispex_static_data_vtbl_t DOMImplementation_dispex_vtbl = {
    .query_interface  = HTMLDOMImplementation_query_interface,
    .destructor       = HTMLDOMImplementation_destructor,
    .traverse         = HTMLDOMImplementation_traverse,
    .unlink           = HTMLDOMImplementation_unlink
};

static void HTMLDOMImplementation_init_dispex_info(dispex_data_t *info, compat_mode_t compat_mode)
{
    if(compat_mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(info, IHTMLDOMImplementation2_tid, NULL);
}

static const tid_t HTMLDOMImplementation_iface_tids[] = {
    IHTMLDOMImplementation_tid,
    0
};
dispex_static_data_t DOMImplementation_dispex = {
    .id         = PROT_DOMImplementation,
    .vtbl       = &DOMImplementation_dispex_vtbl,
    .disp_tid   = DispHTMLDOMImplementation_tid,
    .iface_tids = HTMLDOMImplementation_iface_tids,
    .init_info  = HTMLDOMImplementation_init_dispex_info,
};

HRESULT create_dom_implementation(HTMLDocumentNode *doc_node, IHTMLDOMImplementation **ret)
{
    HTMLDOMImplementation *dom_implementation;
    nsresult nsres;

    if(!doc_node->browser)
        return E_UNEXPECTED;

    dom_implementation = calloc(1, sizeof(*dom_implementation));
    if(!dom_implementation)
        return E_OUTOFMEMORY;

    dom_implementation->IHTMLDOMImplementation_iface.lpVtbl = &HTMLDOMImplementationVtbl;
    dom_implementation->IHTMLDOMImplementation2_iface.lpVtbl = &HTMLDOMImplementation2Vtbl;
    dom_implementation->doc = doc_node;

    init_dispatch(&dom_implementation->dispex, &DOMImplementation_dispex, doc_node->script_global, doc_node->document_mode);

    nsres = nsIDOMDocument_GetImplementation(doc_node->dom_document, &dom_implementation->implementation);
    if(NS_FAILED(nsres)) {
        ERR("GetDOMImplementation failed: %08lx\n", nsres);
        IHTMLDOMImplementation_Release(&dom_implementation->IHTMLDOMImplementation_iface);
        return E_FAIL;
    }

    *ret = &dom_implementation->IHTMLDOMImplementation_iface;
    return S_OK;
}

void detach_dom_implementation(IHTMLDOMImplementation *iface)
{
    HTMLDOMImplementation *dom_implementation = impl_from_IHTMLDOMImplementation(iface);
    dom_implementation->doc = NULL;
}

typedef struct {
    DispatchEx dispex;
    IHTMLScreen IHTMLScreen_iface;
} HTMLScreen;

static inline HTMLScreen *impl_from_IHTMLScreen(IHTMLScreen *iface)
{
    return CONTAINING_RECORD(iface, HTMLScreen, IHTMLScreen_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLScreen, IHTMLScreen, impl_from_IHTMLScreen(iface)->dispex)

static HRESULT WINAPI HTMLScreen_get_colorDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    HDC hdc = GetDC(0);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_bufferDepth(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_bufferDepth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_width(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetSystemMetrics(SM_CXSCREEN);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_height(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = GetSystemMetrics(SM_CYSCREEN);
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_put_updateInterval(IHTMLScreen *iface, LONG v)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_updateInterval(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScreen_get_availHeight(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.bottom-work_area.top;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_availWidth(IHTMLScreen *iface, LONG *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    RECT work_area;

    TRACE("(%p)->(%p)\n", This, p);

    if(!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
        return E_FAIL;

    *p = work_area.right-work_area.left;
    return S_OK;
}

static HRESULT WINAPI HTMLScreen_get_fontSmoothingEnabled(IHTMLScreen *iface, VARIANT_BOOL *p)
{
    HTMLScreen *This = impl_from_IHTMLScreen(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLScreenVtbl HTMLSreenVtbl = {
    HTMLScreen_QueryInterface,
    HTMLScreen_AddRef,
    HTMLScreen_Release,
    HTMLScreen_GetTypeInfoCount,
    HTMLScreen_GetTypeInfo,
    HTMLScreen_GetIDsOfNames,
    HTMLScreen_Invoke,
    HTMLScreen_get_colorDepth,
    HTMLScreen_put_bufferDepth,
    HTMLScreen_get_bufferDepth,
    HTMLScreen_get_width,
    HTMLScreen_get_height,
    HTMLScreen_put_updateInterval,
    HTMLScreen_get_updateInterval,
    HTMLScreen_get_availHeight,
    HTMLScreen_get_availWidth,
    HTMLScreen_get_fontSmoothingEnabled
};

static inline HTMLScreen *HTMLScreen_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLScreen, dispex);
}

static void *HTMLScreen_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLScreen *This = HTMLScreen_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLScreen, riid))
        return &This->IHTMLScreen_iface;

    return NULL;
}

static void HTMLScreen_destructor(DispatchEx *dispex)
{
    HTMLScreen *This = HTMLScreen_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLScreen_dispex_vtbl = {
    .query_interface  = HTMLScreen_query_interface,
    .destructor       = HTMLScreen_destructor,
};

static const tid_t Screen_iface_tids[] = {
    IHTMLScreen_tid,
    0
};
dispex_static_data_t Screen_dispex = {
    .id         = PROT_Screen,
    .vtbl       = &HTMLScreen_dispex_vtbl,
    .disp_tid   = DispHTMLScreen_tid,
    .iface_tids = Screen_iface_tids,
};

HRESULT create_html_screen(HTMLInnerWindow *window, IHTMLScreen **ret)
{
    HTMLScreen *screen;

    screen = calloc(1, sizeof(HTMLScreen));
    if(!screen)
        return E_OUTOFMEMORY;

    screen->IHTMLScreen_iface.lpVtbl = &HTMLSreenVtbl;

    init_dispatch(&screen->dispex, &Screen_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));

    *ret = &screen->IHTMLScreen_iface;
    return S_OK;
}

static inline OmHistory *impl_from_IOmHistory(IOmHistory *iface)
{
    return CONTAINING_RECORD(iface, OmHistory, IOmHistory_iface);
}

DISPEX_IDISPATCH_IMPL(OmHistory, IOmHistory, impl_from_IOmHistory(iface)->dispex)

static HRESULT WINAPI OmHistory_get_length(IOmHistory *iface, short *p)
{
    OmHistory *This = impl_from_IOmHistory(iface);
    GeckoBrowser *browser = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window->base.outer_window)
        browser = This->window->base.outer_window->browser;

    *p = browser && browser->doc->travel_log
        ? ITravelLog_CountEntries(browser->doc->travel_log, browser->doc->browser_service)
        : 0;
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

static inline OmHistory *OmHistory_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, OmHistory, dispex);
}

static void *OmHistory_query_interface(DispatchEx *dispex, REFIID riid)
{
    OmHistory *This = OmHistory_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IOmHistory, riid))
        return &This->IOmHistory_iface;

    return NULL;
}

static void OmHistory_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    OmHistory *This = OmHistory_from_DispatchEx(dispex);

    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void OmHistory_unlink(DispatchEx *dispex)
{
    OmHistory *This = OmHistory_from_DispatchEx(dispex);

    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void OmHistory_destructor(DispatchEx *dispex)
{
    OmHistory *This = OmHistory_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t OmHistory_dispex_vtbl = {
    .query_interface  = OmHistory_query_interface,
    .destructor       = OmHistory_destructor,
    .traverse         = OmHistory_traverse,
    .unlink           = OmHistory_unlink,
};

static const tid_t History_iface_tids[] = {
    IOmHistory_tid,
    0
};
dispex_static_data_t History_dispex = {
    .id         = PROT_History,
    .vtbl       = &OmHistory_dispex_vtbl,
    .disp_tid   = DispHTMLHistory_tid,
    .iface_tids = History_iface_tids,
};


HRESULT create_history(HTMLInnerWindow *window, OmHistory **ret)
{
    OmHistory *history;

    history = calloc(1, sizeof(*history));
    if(!history)
        return E_OUTOFMEMORY;

    init_dispatch(&history->dispex, &History_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));
    history->IOmHistory_iface.lpVtbl = &OmHistoryVtbl;

    history->window = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    *ret = history;
    return S_OK;
}

struct HTMLPluginsCollection {
    DispatchEx dispex;
    IHTMLPluginsCollection IHTMLPluginsCollection_iface;

    OmNavigator *navigator;
};

static inline HTMLPluginsCollection *impl_from_IHTMLPluginsCollection(IHTMLPluginsCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLPluginsCollection, IHTMLPluginsCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLPluginsCollection, IHTMLPluginsCollection,
                      impl_from_IHTMLPluginsCollection(iface)->dispex)

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

static inline HTMLPluginsCollection *HTMLPluginsCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLPluginsCollection, dispex);
}

static void *HTMLPluginsCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLPluginsCollection *This = HTMLPluginsCollection_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLPluginsCollection, riid))
        return &This->IHTMLPluginsCollection_iface;

    return NULL;
}

static void HTMLPluginsCollection_unlink(DispatchEx *dispex)
{
    HTMLPluginsCollection *This = HTMLPluginsCollection_from_DispatchEx(dispex);
    if(This->navigator) {
        This->navigator->plugins = NULL;
        This->navigator = NULL;
    }
}

static void HTMLPluginsCollection_destructor(DispatchEx *dispex)
{
    HTMLPluginsCollection *This = HTMLPluginsCollection_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLPluginsCollection_dispex_vtbl = {
    .query_interface  = HTMLPluginsCollection_query_interface,
    .destructor       = HTMLPluginsCollection_destructor,
    .unlink           = HTMLPluginsCollection_unlink
};

static const tid_t PluginArray_iface_tids[] = {
    IHTMLPluginsCollection_tid,
    0
};
dispex_static_data_t PluginArray_dispex = {
    .id         = PROT_PluginArray,
    .vtbl       = &HTMLPluginsCollection_dispex_vtbl,
    .disp_tid   = DispCPlugins_tid,
    .iface_tids = PluginArray_iface_tids,
};

static HRESULT create_plugins_collection(OmNavigator *navigator, HTMLPluginsCollection **ret)
{
    HTMLPluginsCollection *col;

    col = calloc(1, sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLPluginsCollection_iface.lpVtbl = &HTMLPluginsCollectionVtbl;
    col->navigator = navigator;

    init_dispatch_with_owner(&col->dispex, &PluginArray_dispex, &navigator->dispex);

    *ret = col;
    return S_OK;
}

struct HTMLMimeTypesCollection {
    DispatchEx dispex;
    IHTMLMimeTypesCollection IHTMLMimeTypesCollection_iface;

    OmNavigator *navigator;
};

static inline HTMLMimeTypesCollection *impl_from_IHTMLMimeTypesCollection(IHTMLMimeTypesCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLMimeTypesCollection, IHTMLMimeTypesCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLMimeTypesCollection, IHTMLMimeTypesCollection,
                      impl_from_IHTMLMimeTypesCollection(iface)->dispex)

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

static inline HTMLMimeTypesCollection *HTMLMimeTypesCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLMimeTypesCollection, dispex);
}

static void *HTMLMimeTypesCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLMimeTypesCollection *This = HTMLMimeTypesCollection_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLMimeTypesCollection, riid))
        return &This->IHTMLMimeTypesCollection_iface;

    return NULL;
}

static void HTMLMimeTypesCollection_unlink(DispatchEx *dispex)
{
    HTMLMimeTypesCollection *This = HTMLMimeTypesCollection_from_DispatchEx(dispex);
    if(This->navigator) {
        This->navigator->mime_types = NULL;
        This->navigator = NULL;
    }
}

static void HTMLMimeTypesCollection_destructor(DispatchEx *dispex)
{
    HTMLMimeTypesCollection *This = HTMLMimeTypesCollection_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLMimeTypesCollection_dispex_vtbl = {
    .query_interface  = HTMLMimeTypesCollection_query_interface,
    .destructor       = HTMLMimeTypesCollection_destructor,
    .unlink           = HTMLMimeTypesCollection_unlink
};

static const tid_t MimeTypeArray_iface_tids[] = {
    IHTMLMimeTypesCollection_tid,
    0
};
dispex_static_data_t MimeTypeArray_dispex = {
    .id         = PROT_MimeTypeArray,
    .vtbl       = &HTMLMimeTypesCollection_dispex_vtbl,
    .disp_tid   = IHTMLMimeTypesCollection_tid,
    .iface_tids = MimeTypeArray_iface_tids,
};

static HRESULT create_mime_types_collection(OmNavigator *navigator, HTMLMimeTypesCollection **ret)
{
    HTMLMimeTypesCollection *col;

    col = calloc(1, sizeof(*col));
    if(!col)
        return E_OUTOFMEMORY;

    col->IHTMLMimeTypesCollection_iface.lpVtbl = &HTMLMimeTypesCollectionVtbl;
    col->navigator = navigator;

    init_dispatch_with_owner(&col->dispex, &MimeTypeArray_dispex, &navigator->dispex);

    *ret = col;
    return S_OK;
}

static inline OmNavigator *impl_from_IOmNavigator(IOmNavigator *iface)
{
    return CONTAINING_RECORD(iface, OmNavigator, IOmNavigator_iface);
}

DISPEX_IDISPATCH_IMPL(OmNavigator, IOmNavigator,impl_from_IOmNavigator(iface)->dispex)

static HRESULT WINAPI OmNavigator_get_appCodeName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(L"Mozilla");
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(dispex_compat_mode(&This->dispex) == COMPAT_MODE_IE11
                        ? L"Netscape" : L"Microsoft Internet Explorer");
    if(!*p)
        return E_OUTOFMEMORY;

    return S_OK;
}

/* undocumented, added in IE8 */
extern HRESULT WINAPI MapBrowserEmulationModeToUserAgent(const void*,WCHAR**);

/* Retrieves allocated user agent via CoTaskMemAlloc */
static HRESULT get_user_agent(OmNavigator *navigator, WCHAR **user_agent)
{
    DWORD version = get_compat_mode_version(dispex_compat_mode(&navigator->dispex));

    return MapBrowserEmulationModeToUserAgent(&version, user_agent);
}

static HRESULT WINAPI OmNavigator_get_appVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    WCHAR *user_agent;
    unsigned len;
    HRESULT hres;
    const unsigned skip_prefix = strlen("Mozilla/");

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_user_agent(This, &user_agent);
    if(FAILED(hres))
        return hres;
    len = wcslen(user_agent);

    if(len < skip_prefix) {
        CoTaskMemFree(user_agent);
        *p = NULL;
        return S_OK;
    }

    *p = SysAllocStringLen(user_agent + skip_prefix, len - skip_prefix);
    CoTaskMemFree(user_agent);
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI OmNavigator_get_userAgent(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);
    WCHAR *user_agent;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_user_agent(This, &user_agent);
    if(FAILED(hres))
        return hres;

    *p = SysAllocString(user_agent);
    CoTaskMemFree(user_agent);
    return *p ? S_OK : E_OUTOFMEMORY;
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

    TRACE("(%p)->(%p)\n", This, String);

    return dispex_to_string(&This->dispex, String);
}

static HRESULT WINAPI OmNavigator_get_cpuClass(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

#ifdef _WIN64
    *p = SysAllocString(L"x64");
#else
    *p = SysAllocString(L"x86");
#endif
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT get_language_string(LCID lcid, BSTR *p)
{
    BSTR ret;
    int len;

    len = LCIDToLocaleName(lcid, NULL, 0, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %lu\n", GetLastError());
        return E_FAIL;
    }

    ret = SysAllocStringLen(NULL, len-1);
    if(!ret)
        return E_OUTOFMEMORY;

    len = LCIDToLocaleName(lcid, ret, len, 0);
    if(!len) {
        WARN("LCIDToLocaleName failed: %lu\n", GetLastError());
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

    TRACE("(%p)->(%p)\n", This, p);

#ifdef _WIN64
    *p = SysAllocString(L"Win64");
#else
    *p = SysAllocString(L"Win32");
#endif
    return S_OK;
}

static HRESULT WINAPI OmNavigator_get_appMinorVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = impl_from_IOmNavigator(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* NOTE: MSIE returns "0" or values like ";SP2;". Returning "0" should be enough. */
    *p = SysAllocString(L"0");
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

static inline OmNavigator *OmNavigator_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, OmNavigator, dispex);
}

static void *OmNavigator_query_interface(DispatchEx *dispex, REFIID riid)
{
    OmNavigator *This = OmNavigator_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IOmNavigator, riid))
        return &This->IOmNavigator_iface;

    return NULL;
}

static void OmNavigator_unlink(DispatchEx *dispex)
{
    OmNavigator *This = OmNavigator_from_DispatchEx(dispex);
    if(This->plugins) {
        This->plugins->navigator = NULL;
        This->plugins = NULL;
    }
    if(This->mime_types) {
        This->mime_types->navigator = NULL;
        This->mime_types = NULL;
    }
}

static void OmNavigator_destructor(DispatchEx *dispex)
{
    OmNavigator *This = OmNavigator_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t Navigator_dispex_vtbl = {
    .query_interface  = OmNavigator_query_interface,
    .destructor       = OmNavigator_destructor,
    .unlink           = OmNavigator_unlink
};

static const tid_t Navigator_iface_tids[] = {
    IOmNavigator_tid,
    0
};
dispex_static_data_t Navigator_dispex = {
    .id         = PROT_Navigator,
    .vtbl       = &Navigator_dispex_vtbl,
    .disp_tid   = DispHTMLNavigator_tid,
    .iface_tids = Navigator_iface_tids,
};

HRESULT create_navigator(HTMLInnerWindow *script_global, IOmNavigator **navigator)
{
    OmNavigator *ret;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IOmNavigator_iface.lpVtbl = &OmNavigatorVtbl;

    init_dispatch(&ret->dispex, &Navigator_dispex, script_global,
                  dispex_compat_mode(&script_global->event_target.dispex));

    *navigator = &ret->IOmNavigator_iface;
    return S_OK;
}

typedef struct {
    DispatchEx dispex;
    IHTMLPerformanceTiming IHTMLPerformanceTiming_iface;

    HTMLInnerWindow *window;
} HTMLPerformanceTiming;

static inline HTMLPerformanceTiming *impl_from_IHTMLPerformanceTiming(IHTMLPerformanceTiming *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceTiming, IHTMLPerformanceTiming_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLPerformanceTiming, IHTMLPerformanceTiming,
                      impl_from_IHTMLPerformanceTiming(iface)->dispex)

static ULONGLONG get_fetch_time(HTMLPerformanceTiming *This)
{
    HTMLInnerWindow *window = This->window;

    /* If there's no prior doc unloaded and no redirects, fetch time == navigationStart time */
    if(!window->unload_event_end_time && !window->redirect_time)
        return window->navigation_start_time;

    if(window->dns_lookup_time)
        return window->dns_lookup_time;
    if(window->connect_time)
        return window->connect_time;
    if(window->request_time)
        return window->request_time;
    if(window->unload_event_end_time)
        return window->unload_event_end_time;

    return window->redirect_time;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_navigationStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->navigation_start_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_unloadEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->unload_event_start_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_unloadEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->unload_event_end_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_redirectStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->redirect_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_redirectEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->redirect_time ? get_fetch_time(This) : 0;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_fetchStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domainLookupStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->dns_lookup_time ? This->window->dns_lookup_time : get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domainLookupEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->connect_time    ? This->window->connect_time    :
         This->window->dns_lookup_time ? This->window->dns_lookup_time : get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_connectStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->connect_time    ? This->window->connect_time    :
         This->window->dns_lookup_time ? This->window->dns_lookup_time : get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_connectEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->request_time    ? This->window->request_time    :
         This->window->connect_time    ? This->window->connect_time    :
         This->window->dns_lookup_time ? This->window->dns_lookup_time : get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_requestStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->request_time    ? This->window->request_time    :
         This->window->connect_time    ? This->window->connect_time    :
         This->window->dns_lookup_time ? This->window->dns_lookup_time : get_fetch_time(This);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_responseStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->response_start_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_responseEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->response_end_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domLoading(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* Make sure this is after responseEnd, when the Gecko parser starts */
    *p = This->window->response_end_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domInteractive(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->dom_interactive_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domContentLoadedEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->dom_content_loaded_event_start_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domContentLoadedEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->dom_content_loaded_event_end_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_domComplete(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->dom_complete_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_loadEventStart(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->load_event_start_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_loadEventEnd(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->load_event_end_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_get_msFirstPaint(IHTMLPerformanceTiming *iface, ULONGLONG *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->first_paint_time;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceTiming_toString(IHTMLPerformanceTiming *iface, BSTR *string)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);

    TRACE("(%p)->(%p)\n", This, string);

    return dispex_to_string(&This->dispex, string);
}

static HRESULT WINAPI HTMLPerformanceTiming_toJSON(IHTMLPerformanceTiming *iface, VARIANT *p)
{
    HTMLPerformanceTiming *This = impl_from_IHTMLPerformanceTiming(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLPerformanceTimingVtbl HTMLPerformanceTimingVtbl = {
    HTMLPerformanceTiming_QueryInterface,
    HTMLPerformanceTiming_AddRef,
    HTMLPerformanceTiming_Release,
    HTMLPerformanceTiming_GetTypeInfoCount,
    HTMLPerformanceTiming_GetTypeInfo,
    HTMLPerformanceTiming_GetIDsOfNames,
    HTMLPerformanceTiming_Invoke,
    HTMLPerformanceTiming_get_navigationStart,
    HTMLPerformanceTiming_get_unloadEventStart,
    HTMLPerformanceTiming_get_unloadEventEnd,
    HTMLPerformanceTiming_get_redirectStart,
    HTMLPerformanceTiming_get_redirectEnd,
    HTMLPerformanceTiming_get_fetchStart,
    HTMLPerformanceTiming_get_domainLookupStart,
    HTMLPerformanceTiming_get_domainLookupEnd,
    HTMLPerformanceTiming_get_connectStart,
    HTMLPerformanceTiming_get_connectEnd,
    HTMLPerformanceTiming_get_requestStart,
    HTMLPerformanceTiming_get_responseStart,
    HTMLPerformanceTiming_get_responseEnd,
    HTMLPerformanceTiming_get_domLoading,
    HTMLPerformanceTiming_get_domInteractive,
    HTMLPerformanceTiming_get_domContentLoadedEventStart,
    HTMLPerformanceTiming_get_domContentLoadedEventEnd,
    HTMLPerformanceTiming_get_domComplete,
    HTMLPerformanceTiming_get_loadEventStart,
    HTMLPerformanceTiming_get_loadEventEnd,
    HTMLPerformanceTiming_get_msFirstPaint,
    HTMLPerformanceTiming_toString,
    HTMLPerformanceTiming_toJSON
};

static inline HTMLPerformanceTiming *HTMLPerformanceTiming_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceTiming, dispex);
}

static void *HTMLPerformanceTiming_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLPerformanceTiming *This = HTMLPerformanceTiming_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLPerformanceTiming, riid))
        return &This->IHTMLPerformanceTiming_iface;

    return NULL;
}

static void HTMLPerformanceTiming_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLPerformanceTiming *This = HTMLPerformanceTiming_from_DispatchEx(dispex);
    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void HTMLPerformanceTiming_unlink(DispatchEx *dispex)
{
    HTMLPerformanceTiming *This = HTMLPerformanceTiming_from_DispatchEx(dispex);
    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void HTMLPerformanceTiming_destructor(DispatchEx *dispex)
{
    HTMLPerformanceTiming *This = HTMLPerformanceTiming_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLPerformanceTiming_dispex_vtbl = {
    .query_interface  = HTMLPerformanceTiming_query_interface,
    .destructor       = HTMLPerformanceTiming_destructor,
    .traverse         = HTMLPerformanceTiming_traverse,
    .unlink           = HTMLPerformanceTiming_unlink
};

static const tid_t PerformanceTiming_iface_tids[] = {
    IHTMLPerformanceTiming_tid,
    0
};
dispex_static_data_t PerformanceTiming_dispex = {
    .id         = PROT_PerformanceTiming,
    .vtbl       = &HTMLPerformanceTiming_dispex_vtbl,
    .disp_tid   = IHTMLPerformanceTiming_tid,
    .iface_tids = PerformanceTiming_iface_tids,
};

typedef struct {
    DispatchEx dispex;
    IHTMLPerformanceNavigation IHTMLPerformanceNavigation_iface;

    HTMLInnerWindow *window;
} HTMLPerformanceNavigation;

static inline HTMLPerformanceNavigation *impl_from_IHTMLPerformanceNavigation(IHTMLPerformanceNavigation *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceNavigation, IHTMLPerformanceNavigation_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLPerformanceNavigation, IHTMLPerformanceNavigation,
                      impl_from_IHTMLPerformanceNavigation(iface)->dispex)

static HRESULT WINAPI HTMLPerformanceNavigation_get_type(IHTMLPerformanceNavigation *iface, ULONG *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->navigation_type;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceNavigation_get_redirectCount(IHTMLPerformanceNavigation *iface, ULONG *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->window->redirect_count;
    return S_OK;
}

static HRESULT WINAPI HTMLPerformanceNavigation_toString(IHTMLPerformanceNavigation *iface, BSTR *string)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);

    TRACE("(%p)->(%p)\n", This, string);

    return dispex_to_string(&This->dispex, string);
}

static HRESULT WINAPI HTMLPerformanceNavigation_toJSON(IHTMLPerformanceNavigation *iface, VARIANT *p)
{
    HTMLPerformanceNavigation *This = impl_from_IHTMLPerformanceNavigation(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLPerformanceNavigationVtbl HTMLPerformanceNavigationVtbl = {
    HTMLPerformanceNavigation_QueryInterface,
    HTMLPerformanceNavigation_AddRef,
    HTMLPerformanceNavigation_Release,
    HTMLPerformanceNavigation_GetTypeInfoCount,
    HTMLPerformanceNavigation_GetTypeInfo,
    HTMLPerformanceNavigation_GetIDsOfNames,
    HTMLPerformanceNavigation_Invoke,
    HTMLPerformanceNavigation_get_type,
    HTMLPerformanceNavigation_get_redirectCount,
    HTMLPerformanceNavigation_toString,
    HTMLPerformanceNavigation_toJSON
};

static inline HTMLPerformanceNavigation *HTMLPerformanceNavigation_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformanceNavigation, dispex);
}

static void *HTMLPerformanceNavigation_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLPerformanceNavigation *This = HTMLPerformanceNavigation_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLPerformanceNavigation, riid))
        return &This->IHTMLPerformanceNavigation_iface;

    return NULL;
}

static void HTMLPerformanceNavigation_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLPerformanceNavigation *This = HTMLPerformanceNavigation_from_DispatchEx(dispex);
    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void HTMLPerformanceNavigation_unlink(DispatchEx *dispex)
{
    HTMLPerformanceNavigation *This = HTMLPerformanceNavigation_from_DispatchEx(dispex);
    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void HTMLPerformanceNavigation_destructor(DispatchEx *dispex)
{
    HTMLPerformanceNavigation *This = HTMLPerformanceNavigation_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLPerformanceNavigation_dispex_vtbl = {
    .query_interface  = HTMLPerformanceNavigation_query_interface,
    .destructor       = HTMLPerformanceNavigation_destructor,
    .traverse         = HTMLPerformanceNavigation_traverse,
    .unlink           = HTMLPerformanceNavigation_unlink
};

static const tid_t PerformanceNavigation_iface_tids[] = {
    IHTMLPerformanceNavigation_tid,
    0
};
dispex_static_data_t PerformanceNavigation_dispex = {
    .id         = PROT_PerformanceNavigation,
    .vtbl       = &HTMLPerformanceNavigation_dispex_vtbl,
    .disp_tid   = IHTMLPerformanceNavigation_tid,
    .iface_tids = PerformanceNavigation_iface_tids,
};

typedef struct {
    DispatchEx dispex;
    IHTMLPerformance IHTMLPerformance_iface;

    HTMLInnerWindow *window;
    IHTMLPerformanceNavigation *navigation;
    IHTMLPerformanceTiming *timing;
} HTMLPerformance;

static inline HTMLPerformance *impl_from_IHTMLPerformance(IHTMLPerformance *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformance, IHTMLPerformance_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLPerformance, IHTMLPerformance, impl_from_IHTMLPerformance(iface)->dispex)

static HRESULT WINAPI HTMLPerformance_get_navigation(IHTMLPerformance *iface,
                                                     IHTMLPerformanceNavigation **p)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->navigation) {
        HTMLPerformanceNavigation *navigation;

        navigation = calloc(1, sizeof(*navigation));
        if(!navigation)
            return E_OUTOFMEMORY;

        navigation->IHTMLPerformanceNavigation_iface.lpVtbl = &HTMLPerformanceNavigationVtbl;
        navigation->window = This->window;
        IHTMLWindow2_AddRef(&This->window->base.IHTMLWindow2_iface);

        init_dispatch(&navigation->dispex, &PerformanceNavigation_dispex, This->window,
                      dispex_compat_mode(&This->dispex));

        This->navigation = &navigation->IHTMLPerformanceNavigation_iface;
    }

    IHTMLPerformanceNavigation_AddRef(*p = This->navigation);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformance_get_timing(IHTMLPerformance *iface, IHTMLPerformanceTiming **p)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->timing) {
        HTMLPerformanceTiming *timing;

        timing = calloc(1, sizeof(*timing));
        if(!timing)
            return E_OUTOFMEMORY;

        timing->IHTMLPerformanceTiming_iface.lpVtbl = &HTMLPerformanceTimingVtbl;
        timing->window = This->window;
        IHTMLWindow2_AddRef(&This->window->base.IHTMLWindow2_iface);

        init_dispatch(&timing->dispex, &PerformanceTiming_dispex, This->window,
                      dispex_compat_mode(&This->dispex));

        This->timing = &timing->IHTMLPerformanceTiming_iface;
    }

    IHTMLPerformanceTiming_AddRef(*p = This->timing);
    return S_OK;
}

static HRESULT WINAPI HTMLPerformance_toString(IHTMLPerformance *iface, BSTR *string)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);

    TRACE("(%p)->(%p)\n", This, string);

    return dispex_to_string(&This->dispex, string);
}

static HRESULT WINAPI HTMLPerformance_toJSON(IHTMLPerformance *iface, VARIANT *var)
{
    HTMLPerformance *This = impl_from_IHTMLPerformance(iface);
    FIXME("(%p)->(%p)\n", This, var);
    return E_NOTIMPL;
}

static const IHTMLPerformanceVtbl HTMLPerformanceVtbl = {
    HTMLPerformance_QueryInterface,
    HTMLPerformance_AddRef,
    HTMLPerformance_Release,
    HTMLPerformance_GetTypeInfoCount,
    HTMLPerformance_GetTypeInfo,
    HTMLPerformance_GetIDsOfNames,
    HTMLPerformance_Invoke,
    HTMLPerformance_get_navigation,
    HTMLPerformance_get_timing,
    HTMLPerformance_toString,
    HTMLPerformance_toJSON
};

static inline HTMLPerformance *HTMLPerformance_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLPerformance, dispex);
}

static void *HTMLPerformance_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLPerformance *This = HTMLPerformance_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLPerformance, riid))
        return &This->IHTMLPerformance_iface;

    return NULL;
}

static void HTMLPerformance_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLPerformance *This = HTMLPerformance_from_DispatchEx(dispex);
    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
    if(This->navigation)
        note_cc_edge((nsISupports*)This->navigation, "navigation", cb);
    if(This->timing)
        note_cc_edge((nsISupports*)This->timing, "timing", cb);
}

static void HTMLPerformance_unlink(DispatchEx *dispex)
{
    HTMLPerformance *This = HTMLPerformance_from_DispatchEx(dispex);
    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
    unlink_ref(&This->navigation);
    unlink_ref(&This->timing);
}

static void HTMLPerformance_destructor(DispatchEx *dispex)
{
    HTMLPerformance *This = HTMLPerformance_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLPerformance_dispex_vtbl = {
    .query_interface  = HTMLPerformance_query_interface,
    .destructor       = HTMLPerformance_destructor,
    .traverse         = HTMLPerformance_traverse,
    .unlink           = HTMLPerformance_unlink
};

static const tid_t Performance_iface_tids[] = {
    IHTMLPerformance_tid,
    0
};
dispex_static_data_t Performance_dispex = {
    .id         = PROT_Performance,
    .vtbl       = &HTMLPerformance_dispex_vtbl,
    .disp_tid   = IHTMLPerformance_tid,
    .iface_tids = Performance_iface_tids,
};

HRESULT create_performance(HTMLInnerWindow *window, IHTMLPerformance **ret)
{
    compat_mode_t compat_mode = dispex_compat_mode(&window->event_target.dispex);
    HTMLPerformance *performance;

    performance = calloc(1, sizeof(*performance));
    if(!performance)
        return E_OUTOFMEMORY;

    performance->IHTMLPerformance_iface.lpVtbl = &HTMLPerformanceVtbl;
    performance->window = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    init_dispatch(&performance->dispex, &Performance_dispex, window, compat_mode);

    *ret = &performance->IHTMLPerformance_iface;
    return S_OK;
}

typedef struct {
    DispatchEx dispex;
    IHTMLNamespaceCollection IHTMLNamespaceCollection_iface;
} HTMLNamespaceCollection;

static inline HTMLNamespaceCollection *impl_from_IHTMLNamespaceCollection(IHTMLNamespaceCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLNamespaceCollection, IHTMLNamespaceCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLNamespaceCollection, IHTMLNamespaceCollection,
                      impl_from_IHTMLNamespaceCollection(iface)->dispex)

static HRESULT WINAPI HTMLNamespaceCollection_get_length(IHTMLNamespaceCollection *iface, LONG *p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%p) returning 0\n", This, p);
    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLNamespaceCollection_item(IHTMLNamespaceCollection *iface, VARIANT index, IDispatch **p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&index), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLNamespaceCollection_add(IHTMLNamespaceCollection *iface, BSTR namespace, BSTR urn,
                                                  VARIANT implementation_url, IDispatch **p)
{
    HTMLNamespaceCollection *This = impl_from_IHTMLNamespaceCollection(iface);
    FIXME("(%p)->(%s %s %s %p)\n", This, debugstr_w(namespace), debugstr_w(urn), debugstr_variant(&implementation_url), p);
    return E_NOTIMPL;
}

static const IHTMLNamespaceCollectionVtbl HTMLNamespaceCollectionVtbl = {
    HTMLNamespaceCollection_QueryInterface,
    HTMLNamespaceCollection_AddRef,
    HTMLNamespaceCollection_Release,
    HTMLNamespaceCollection_GetTypeInfoCount,
    HTMLNamespaceCollection_GetTypeInfo,
    HTMLNamespaceCollection_GetIDsOfNames,
    HTMLNamespaceCollection_Invoke,
    HTMLNamespaceCollection_get_length,
    HTMLNamespaceCollection_item,
    HTMLNamespaceCollection_add
};

static inline HTMLNamespaceCollection *HTMLNamespaceCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLNamespaceCollection, dispex);
}

static void *HTMLNamespaceCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLNamespaceCollection *This = HTMLNamespaceCollection_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLNamespaceCollection, riid))
        return &This->IHTMLNamespaceCollection_iface;

    return NULL;
}

static void HTMLNamespaceCollection_destructor(DispatchEx *dispex)
{
    HTMLNamespaceCollection *This = HTMLNamespaceCollection_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLNamespaceCollection_dispex_vtbl = {
    .query_interface  = HTMLNamespaceCollection_query_interface,
    .destructor       = HTMLNamespaceCollection_destructor,
};

static const tid_t MSNamespaceInfoCollection_iface_tids[] = {
    IHTMLNamespaceCollection_tid,
    0
};
dispex_static_data_t MSNamespaceInfoCollection_dispex = {
    .id              = PROT_MSNamespaceInfoCollection,
    .vtbl            = &HTMLNamespaceCollection_dispex_vtbl,
    .disp_tid        = DispHTMLNamespaceCollection_tid,
    .iface_tids      = MSNamespaceInfoCollection_iface_tids,
    .max_compat_mode = COMPAT_MODE_IE9,
};

HRESULT create_namespace_collection(HTMLDocumentNode *doc, IHTMLNamespaceCollection **ret)
{
    HTMLNamespaceCollection *namespaces;

    if (!(namespaces = calloc(1, sizeof(*namespaces))))
        return E_OUTOFMEMORY;

    namespaces->IHTMLNamespaceCollection_iface.lpVtbl = &HTMLNamespaceCollectionVtbl;
    init_dispatch(&namespaces->dispex, &MSNamespaceInfoCollection_dispex, doc->script_global,
                  dispex_compat_mode(&doc->node.event_target.dispex));
    *ret = &namespaces->IHTMLNamespaceCollection_iface;
    return S_OK;
}

struct console {
    DispatchEx dispex;
    IWineMSHTMLConsole IWineMSHTMLConsole_iface;
};

static inline struct console *impl_from_IWineMSHTMLConsole(IWineMSHTMLConsole *iface)
{
    return CONTAINING_RECORD(iface, struct console, IWineMSHTMLConsole_iface);
}

DISPEX_IDISPATCH_IMPL(console, IWineMSHTMLConsole, impl_from_IWineMSHTMLConsole(iface)->dispex)

static HRESULT WINAPI console_assert(IWineMSHTMLConsole *iface, VARIANT_BOOL *assertion, VARIANT *vararg_start)
{
    FIXME("iface %p, assertion %p, vararg_start %p stub.\n", iface, assertion, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_clear(IWineMSHTMLConsole *iface)
{
    FIXME("iface %p stub.\n", iface);

    return S_OK;
}

static HRESULT WINAPI console_count(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_debug(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_dir(IWineMSHTMLConsole *iface, VARIANT *object)
{
    FIXME("iface %p, object %p stub.\n", iface, object);

    return S_OK;
}

static HRESULT WINAPI console_dirxml(IWineMSHTMLConsole *iface, VARIANT *object)
{
    FIXME("iface %p, object %p stub.\n", iface, object);

    return S_OK;
}

static HRESULT WINAPI console_error(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_group(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_group_collapsed(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_group_end(IWineMSHTMLConsole *iface)
{
    FIXME("iface %p, stub.\n", iface);

    return S_OK;
}

static HRESULT WINAPI console_info(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_log(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_time(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_time_end(IWineMSHTMLConsole *iface, VARIANT *label)
{
    FIXME("iface %p, label %p stub.\n", iface, label);

    return S_OK;
}

static HRESULT WINAPI console_trace(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static HRESULT WINAPI console_warn(IWineMSHTMLConsole *iface, VARIANT *vararg_start)
{
    FIXME("iface %p, vararg_start %p stub.\n", iface, vararg_start);

    return S_OK;
}

static const IWineMSHTMLConsoleVtbl WineMSHTMLConsoleVtbl = {
    console_QueryInterface,
    console_AddRef,
    console_Release,
    console_GetTypeInfoCount,
    console_GetTypeInfo,
    console_GetIDsOfNames,
    console_Invoke,
    console_assert,
    console_clear,
    console_count,
    console_debug,
    console_dir,
    console_dirxml,
    console_error,
    console_group,
    console_group_collapsed,
    console_group_end,
    console_info,
    console_log,
    console_time,
    console_time_end,
    console_trace,
    console_warn,
};

static inline struct console *console_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct console, dispex);
}

static void *console_query_interface(DispatchEx *dispex, REFIID riid)
{
    struct console *console = console_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IWineMSHTMLConsole, riid))
        return &console->IWineMSHTMLConsole_iface;

    return NULL;
}

static void console_destructor(DispatchEx *dispex)
{
    struct console *console = console_from_DispatchEx(dispex);
    free(console);
}

static const dispex_static_data_vtbl_t Console_dispex_vtbl = {
    .query_interface  = console_query_interface,
    .destructor       = console_destructor,
};

static const tid_t Console_iface_tids[] = {
    IWineMSHTMLConsole_tid,
    0
};
dispex_static_data_t Console_dispex = {
    .id              = PROT_Console,
    .vtbl            = &Console_dispex_vtbl,
    .disp_tid        = IWineMSHTMLConsole_tid,
    .iface_tids      = Console_iface_tids,
    .min_compat_mode = COMPAT_MODE_IE10,
};

void create_console(HTMLInnerWindow *window, IWineMSHTMLConsole **ret)
{
    struct console *obj;

    obj = calloc(1, sizeof(*obj));
    if(!obj)
    {
        ERR("No memory.\n");
        return;
    }

    obj->IWineMSHTMLConsole_iface.lpVtbl = &WineMSHTMLConsoleVtbl;
    init_dispatch(&obj->dispex, &Console_dispex, window, dispex_compat_mode(&window->event_target.dispex));

    *ret = &obj->IWineMSHTMLConsole_iface;
}

struct media_query_list_listener {
    struct list entry;
    IDispatch *function;
};

struct media_query_list_callback;
struct media_query_list {
    DispatchEx dispex;
    IWineMSHTMLMediaQueryList IWineMSHTMLMediaQueryList_iface;
    nsIDOMMediaQueryList *nsquerylist;
    struct media_query_list_callback *callback;
    struct list listeners;
};

struct media_query_list_callback {
    nsIDOMMediaQueryListListener nsIDOMMediaQueryListListener_iface;
    struct media_query_list *media_query_list;
    LONG ref;
};

static inline struct media_query_list *impl_from_IWineMSHTMLMediaQueryList(IWineMSHTMLMediaQueryList *iface)
{
    return CONTAINING_RECORD(iface, struct media_query_list, IWineMSHTMLMediaQueryList_iface);
}

DISPEX_IDISPATCH_IMPL(media_query_list, IWineMSHTMLMediaQueryList,
                      impl_from_IWineMSHTMLMediaQueryList(iface)->dispex)

static HRESULT WINAPI media_query_list_get_media(IWineMSHTMLMediaQueryList *iface, BSTR *p)
{
    struct media_query_list *media_query_list = impl_from_IWineMSHTMLMediaQueryList(iface);
    nsAString nsstr;

    TRACE("(%p)->(%p)\n", media_query_list, p);

    nsAString_InitDepend(&nsstr, NULL);
    return return_nsstr(nsIDOMMediaQueryList_GetMedia(media_query_list->nsquerylist, &nsstr), &nsstr, p);
}

static HRESULT WINAPI media_query_list_get_matches(IWineMSHTMLMediaQueryList *iface, VARIANT_BOOL *p)
{
    struct media_query_list *media_query_list = impl_from_IWineMSHTMLMediaQueryList(iface);
    nsresult nsres;
    cpp_bool b;

    TRACE("(%p)->(%p)\n", media_query_list, p);

    nsres = nsIDOMMediaQueryList_GetMatches(media_query_list->nsquerylist, &b);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);
    *p = b ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI media_query_list_addListener(IWineMSHTMLMediaQueryList *iface, VARIANT *listener)
{
    struct media_query_list *media_query_list = impl_from_IWineMSHTMLMediaQueryList(iface);
    struct media_query_list_listener *entry;

    TRACE("(%p)->(%s)\n", media_query_list, debugstr_variant(listener));

    if(V_VT(listener) != VT_DISPATCH || !V_DISPATCH(listener))
        return S_OK;

    LIST_FOR_EACH_ENTRY(entry, &media_query_list->listeners, struct media_query_list_listener, entry)
        if(entry->function == V_DISPATCH(listener))
            return S_OK;

    if(!(entry = malloc(sizeof(*entry))))
        return E_OUTOFMEMORY;
    entry->function = V_DISPATCH(listener);
    IDispatch_AddRef(V_DISPATCH(listener));

    list_add_tail(&media_query_list->listeners, &entry->entry);
    return S_OK;
}

static HRESULT WINAPI media_query_list_removeListener(IWineMSHTMLMediaQueryList *iface, VARIANT *listener)
{
    struct media_query_list *media_query_list = impl_from_IWineMSHTMLMediaQueryList(iface);
    struct media_query_list_listener *entry;

    TRACE("(%p)->(%s)\n", media_query_list, debugstr_variant(listener));

    if(V_VT(listener) != VT_DISPATCH || !V_DISPATCH(listener))
        return S_OK;

    LIST_FOR_EACH_ENTRY(entry, &media_query_list->listeners, struct media_query_list_listener, entry) {
        if(entry->function == V_DISPATCH(listener)) {
            list_remove(&entry->entry);
            IDispatch_Release(entry->function);
            free(entry);
            break;
        }
    }

    return S_OK;
}

static const IWineMSHTMLMediaQueryListVtbl media_query_list_vtbl = {
    media_query_list_QueryInterface,
    media_query_list_AddRef,
    media_query_list_Release,
    media_query_list_GetTypeInfoCount,
    media_query_list_GetTypeInfo,
    media_query_list_GetIDsOfNames,
    media_query_list_Invoke,
    media_query_list_get_media,
    media_query_list_get_matches,
    media_query_list_addListener,
    media_query_list_removeListener
};

static inline struct media_query_list_callback *impl_from_nsIDOMMediaQueryListListener(nsIDOMMediaQueryListListener *iface)
{
    return CONTAINING_RECORD(iface, struct media_query_list_callback, nsIDOMMediaQueryListListener_iface);
}

static nsresult NSAPI media_query_list_callback_QueryInterface(nsIDOMMediaQueryListListener *iface,
        nsIIDRef riid, void **result)
{
    struct media_query_list_callback *callback = impl_from_nsIDOMMediaQueryListListener(iface);

    if(IsEqualGUID(&IID_nsISupports, riid) || IsEqualGUID(&IID_nsIDOMMediaQueryListListener, riid)) {
        *result = &callback->nsIDOMMediaQueryListListener_iface;
    }else {
        *result = NULL;
        return NS_NOINTERFACE;
    }

    nsIDOMMediaQueryListListener_AddRef(&callback->nsIDOMMediaQueryListListener_iface);
    return NS_OK;
}

static nsrefcnt NSAPI media_query_list_callback_AddRef(nsIDOMMediaQueryListListener *iface)
{
    struct media_query_list_callback *callback = impl_from_nsIDOMMediaQueryListListener(iface);
    LONG ref = InterlockedIncrement(&callback->ref);

    TRACE("(%p) ref=%ld\n", callback, ref);

    return ref;
}

static nsrefcnt NSAPI media_query_list_callback_Release(nsIDOMMediaQueryListListener *iface)
{
    struct media_query_list_callback *callback = impl_from_nsIDOMMediaQueryListListener(iface);
    LONG ref = InterlockedDecrement(&callback->ref);

    TRACE("(%p) ref=%ld\n", callback, ref);

    if(!ref)
        free(callback);
    return ref;
}

static nsresult NSAPI media_query_list_callback_HandleChange(nsIDOMMediaQueryListListener *iface, nsIDOMMediaQueryList *mql)
{
    struct media_query_list_callback *callback = impl_from_nsIDOMMediaQueryListListener(iface);
    IDispatch *listener_funcs_buf[4], **listener_funcs = listener_funcs_buf;
    struct media_query_list *media_query_list = callback->media_query_list;
    struct media_query_list_listener *listener;
    unsigned cnt, i = 0;
    VARIANT args[1], v;
    HRESULT hres;

    if(!media_query_list)
        return NS_OK;

    cnt = list_count(&media_query_list->listeners);
    if(cnt > ARRAY_SIZE(listener_funcs_buf) && !(listener_funcs = malloc(cnt * sizeof(*listener_funcs))))
        return NS_ERROR_OUT_OF_MEMORY;

    LIST_FOR_EACH_ENTRY(listener, &media_query_list->listeners, struct media_query_list_listener, entry) {
        listener_funcs[i] = listener->function;
        IDispatch_AddRef(listener_funcs[i++]);
    }

    for(i = 0; i < cnt; i++) {
        DISPPARAMS dp = { args, NULL, 1, 0 };

        V_VT(args) = VT_DISPATCH;
        V_DISPATCH(args) = (IDispatch*)&media_query_list->dispex.IWineJSDispatchHost_iface;
        V_VT(&v) = VT_EMPTY;

        TRACE("%p >>>\n", media_query_list);
        hres = call_disp_func(listener_funcs[i], &dp, &v);
        if(hres == S_OK) {
            TRACE("%p <<< %s\n", media_query_list, debugstr_variant(&v));
            VariantClear(&v);
        }else {
            WARN("%p <<< %08lx\n", media_query_list, hres);
        }
        IDispatch_Release(listener_funcs[i]);
    }

    if(listener_funcs != listener_funcs_buf)
        free(listener_funcs);
    return NS_OK;
}

static const nsIDOMMediaQueryListListenerVtbl media_query_list_callback_vtbl = {
    media_query_list_callback_QueryInterface,
    media_query_list_callback_AddRef,
    media_query_list_callback_Release,
    media_query_list_callback_HandleChange
};

static inline struct media_query_list *media_query_list_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct media_query_list, dispex);
}

static void *MediaQueryList_query_interface(DispatchEx *dispex, REFIID riid)
{
    struct media_query_list *media_query_list = media_query_list_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IWineMSHTMLMediaQueryList, riid))
        return &media_query_list->IWineMSHTMLMediaQueryList_iface;

    return NULL;
}

static void MediaQueryList_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    struct media_query_list *media_query_list = media_query_list_from_DispatchEx(dispex);
    struct media_query_list_listener *listener;

    LIST_FOR_EACH_ENTRY(listener, &media_query_list->listeners, struct media_query_list_listener, entry)
        note_cc_edge((nsISupports*)listener->function, "function", cb);
    if(media_query_list->nsquerylist)
        note_cc_edge((nsISupports*)media_query_list->nsquerylist, "nsquerylist", cb);
}

static void MediaQueryList_unlink(DispatchEx *dispex)
{
    struct media_query_list *media_query_list = media_query_list_from_DispatchEx(dispex);

    media_query_list->callback->media_query_list = NULL;
    while(!list_empty(&media_query_list->listeners)) {
        struct media_query_list_listener *listener = LIST_ENTRY(list_head(&media_query_list->listeners), struct media_query_list_listener, entry);
        list_remove(&listener->entry);
        IDispatch_Release(listener->function);
        free(listener);
    }
    unlink_ref(&media_query_list->nsquerylist);
}

static void MediaQueryList_destructor(DispatchEx *dispex)
{
    struct media_query_list *media_query_list = media_query_list_from_DispatchEx(dispex);
    nsIDOMMediaQueryListListener_Release(&media_query_list->callback->nsIDOMMediaQueryListListener_iface);
    free(media_query_list);
}

static const dispex_static_data_vtbl_t MediaQueryList_dispex_vtbl = {
    .query_interface  = MediaQueryList_query_interface,
    .destructor       = MediaQueryList_destructor,
    .traverse         = MediaQueryList_traverse,
    .unlink           = MediaQueryList_unlink
};

static const tid_t MediaQueryList_iface_tids[] = {
    IWineMSHTMLMediaQueryList_tid,
    0
};
dispex_static_data_t MediaQueryList_dispex = {
    .id              = PROT_MediaQueryList,
    .vtbl            = &MediaQueryList_dispex_vtbl,
    .disp_tid        = IWineMSHTMLMediaQueryList_tid,
    .iface_tids      = MediaQueryList_iface_tids,
    .min_compat_mode = COMPAT_MODE_IE10,
};

HRESULT create_media_query_list(HTMLInnerWindow *window, BSTR media_query, IDispatch **ret)
{
    struct media_query_list *media_query_list;
    nsISupports *nsunk;
    nsAString nsstr;
    nsresult nsres;

    if(!media_query || !media_query[0])
        return E_INVALIDARG;

    if(!(media_query_list = malloc(sizeof(*media_query_list))))
        return E_OUTOFMEMORY;

    if(!(media_query_list->callback = malloc(sizeof(*media_query_list->callback)))) {
        free(media_query_list);
        return E_OUTOFMEMORY;
    }
    media_query_list->callback->nsIDOMMediaQueryListListener_iface.lpVtbl = &media_query_list_callback_vtbl;
    media_query_list->callback->media_query_list = media_query_list;
    media_query_list->callback->ref = 1;

    nsAString_InitDepend(&nsstr, media_query);
    nsres = nsIDOMWindow_MatchMedia(window->dom_window, &nsstr, &nsunk);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        free(media_query_list->callback);
        free(media_query_list);
        return map_nsresult(nsres);
    }
    nsres = nsISupports_QueryInterface(nsunk, &IID_nsIDOMMediaQueryList, (void**)&media_query_list->nsquerylist);
    assert(NS_SUCCEEDED(nsres));
    nsISupports_Release(nsunk);

    nsres = nsIDOMMediaQueryList_SetListener(media_query_list->nsquerylist, &media_query_list->callback->nsIDOMMediaQueryListListener_iface);
    assert(NS_SUCCEEDED(nsres));

    media_query_list->IWineMSHTMLMediaQueryList_iface.lpVtbl = &media_query_list_vtbl;
    list_init(&media_query_list->listeners);
    init_dispatch(&media_query_list->dispex, &MediaQueryList_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));

    *ret = (IDispatch*)&media_query_list->IWineMSHTMLMediaQueryList_iface;
    return S_OK;
}
