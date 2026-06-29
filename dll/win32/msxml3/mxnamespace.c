/*
 *    IMXNamespaceManager implementation
 *
 * Copyright 2011-2012 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_dispex.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct ns
{
    BSTR prefix;
    BSTR uri;
};

struct nscontext
{
    struct list entry;

    struct ns *ns;
    int   count;
    int   max_alloc;
};

#define DEFAULT_PREFIX_ALLOC_COUNT 16

static const WCHAR xmlW[] = {'x','m','l',0};
static const WCHAR xmluriW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g',
    '/','X','M','L','/','1','9','9','8','/','n','a','m','e','s','p','a','c','e',0};

typedef struct
{
    DispatchEx dispex;
    IMXNamespaceManager   IMXNamespaceManager_iface;
    IVBMXNamespaceManager IVBMXNamespaceManager_iface;
    LONG ref;

    struct list ctxts;

    VARIANT_BOOL override;
} namespacemanager;

static inline namespacemanager *impl_from_IMXNamespaceManager( IMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IMXNamespaceManager_iface);
}

static inline namespacemanager *impl_from_IVBMXNamespaceManager( IVBMXNamespaceManager *iface )
{
    return CONTAINING_RECORD(iface, namespacemanager, IVBMXNamespaceManager_iface);
}

static HRESULT declare_prefix(namespacemanager *This, const WCHAR *prefix, const WCHAR *uri)
{
    struct nscontext *ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    static const WCHAR emptyW[] = {0};
    struct ns *ns;
    int i;

    if (ctxt->count == ctxt->max_alloc)
    {
        ctxt->max_alloc *= 2;
        ctxt->ns = realloc(ctxt->ns, ctxt->max_alloc * sizeof(*ctxt->ns));
    }

    if (!prefix) prefix = emptyW;

    ns = NULL;
    for (i = 0; i < ctxt->count; i++)
        if (!wcscmp(ctxt->ns[i].prefix, prefix))
        {
            ns = &ctxt->ns[i];
            break;
        }

    if (ns)
    {
        if (This->override == VARIANT_TRUE)
        {
            SysFreeString(ns->uri);
            ns->uri = SysAllocString(uri);
            return S_FALSE;
        }
        else
            return E_FAIL;
    }
    else
    {
        ctxt->ns[ctxt->count].prefix = SysAllocString(prefix);
        ctxt->ns[ctxt->count].uri = SysAllocString(uri);
        ctxt->count++;
    }

    return S_OK;
}

/* returned stored pointer, caller needs to copy it */
static HRESULT get_declared_prefix_idx(const struct nscontext *ctxt, LONG index, BSTR *prefix)
{
    *prefix = NULL;

    if (index >= ctxt->count || index < 0) return E_FAIL;

    if (index > 0) index = ctxt->count - index;
    *prefix = ctxt->ns[index].prefix;

    return S_OK;
}

/* returned stored pointer, caller needs to copy it */
static HRESULT get_declared_prefix_uri(const struct list *ctxts, const WCHAR *uri, BSTR *prefix)
{
    struct nscontext *ctxt;

    LIST_FOR_EACH_ENTRY(ctxt, ctxts, struct nscontext, entry)
    {
        int i;
        for (i = 0; i < ctxt->count; i++)
            if (!wcscmp(ctxt->ns[i].uri, uri))
            {
                *prefix = ctxt->ns[i].prefix;
                return S_OK;
            }
    }

    *prefix = NULL;
    return E_FAIL;
}

static HRESULT get_uri_from_prefix(const struct nscontext *ctxt, const WCHAR *prefix, BSTR *uri)
{
    int i;

    for (i = 0; i < ctxt->count; i++)
        if (!wcscmp(ctxt->ns[i].prefix, prefix))
        {
            *uri = ctxt->ns[i].uri;
            return S_OK;
        }

    *uri = NULL;
    return S_FALSE;
}

static struct nscontext* alloc_ns_context(void)
{
    struct nscontext *ctxt;

    ctxt = malloc(sizeof(*ctxt));
    if (!ctxt) return NULL;

    ctxt->count = 0;
    ctxt->max_alloc = DEFAULT_PREFIX_ALLOC_COUNT;
    ctxt->ns = malloc(ctxt->max_alloc * sizeof(*ctxt->ns));
    if (!ctxt->ns)
    {
        free(ctxt);
        return NULL;
    }

    /* first allocated prefix is always 'xml' */
    ctxt->ns[0].prefix = SysAllocString(xmlW);
    ctxt->ns[0].uri = SysAllocString(xmluriW);
    ctxt->count++;
    if (!ctxt->ns[0].prefix || !ctxt->ns[0].uri)
    {
        free(ctxt->ns);
        free(ctxt);
        return NULL;
    }

    return ctxt;
}

static void free_ns_context(struct nscontext *ctxt)
{
    int i;

    for (i = 0; i < ctxt->count; i++)
    {
        SysFreeString(ctxt->ns[i].prefix);
        SysFreeString(ctxt->ns[i].uri);
    }

    free(ctxt->ns);
    free(ctxt);
}

static HRESULT WINAPI namespacemanager_QueryInterface(IMXNamespaceManager *iface, REFIID riid, void **ppvObject)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_QueryInterface(&This->IVBMXNamespaceManager_iface, riid, ppvObject);
}

static ULONG WINAPI namespacemanager_AddRef(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_AddRef(&This->IVBMXNamespaceManager_iface);
}

static ULONG WINAPI namespacemanager_Release(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_Release(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_putAllowOverride(IMXNamespaceManager *iface,
    VARIANT_BOOL override)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_put_allowOverride(&This->IVBMXNamespaceManager_iface, override);
}

static HRESULT WINAPI namespacemanager_getAllowOverride(IMXNamespaceManager *iface,
    VARIANT_BOOL *override)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_get_allowOverride(&This->IVBMXNamespaceManager_iface, override);
}

static HRESULT WINAPI namespacemanager_reset(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_reset(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_pushContext(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_pushContext(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_pushNodeContext(IMXNamespaceManager *iface,
    IXMLDOMNode *node, VARIANT_BOOL deep)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_pushNodeContext(&This->IVBMXNamespaceManager_iface, node, deep);
}

static HRESULT WINAPI namespacemanager_popContext(IMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    return IVBMXNamespaceManager_popContext(&This->IVBMXNamespaceManager_iface);
}

static HRESULT WINAPI namespacemanager_declarePrefix(IMXNamespaceManager *iface,
    const WCHAR *prefix, const WCHAR *namespaceURI)
{
    static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};

    namespacemanager *This = impl_from_IMXNamespaceManager( iface );

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(prefix), debugstr_w(namespaceURI));

    if (prefix && (!wcscmp(prefix, xmlW) || !wcscmp(prefix, xmlnsW) || !namespaceURI))
        return E_INVALIDARG;

    return declare_prefix(This, prefix, namespaceURI);
}

static HRESULT WINAPI namespacemanager_getDeclaredPrefix(IMXNamespaceManager *iface,
    LONG index, WCHAR *prefix, int *prefix_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    struct nscontext *ctxt;
    HRESULT hr;
    BSTR prfx;

    TRACE("%p, %ld, %p, %p.\n", This, index, prefix, prefix_len);

    if (!prefix_len) return E_POINTER;

    ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    hr = get_declared_prefix_idx(ctxt, index, &prfx);
    if (hr != S_OK) return hr;

    if (prefix)
    {
        if (*prefix_len < (INT)SysStringLen(prfx)) return E_XML_BUFFERTOOSMALL;
        lstrcpyW(prefix, prfx);
    }

    *prefix_len = SysStringLen(prfx);

    return S_OK;
}

static HRESULT WINAPI namespacemanager_getPrefix(IMXNamespaceManager *iface,
    const WCHAR *uri, LONG index, WCHAR *prefix, int *prefix_len)
{
    namespacemanager *manager = impl_from_IMXNamespaceManager(iface);
    HRESULT hr;
    BSTR prfx;

    TRACE("%p, %s, %ld, %p, %p.\n", iface, debugstr_w(uri), index, prefix, prefix_len);

    if (!uri || !*uri || !prefix_len) return E_INVALIDARG;

    hr = get_declared_prefix_uri(&manager->ctxts, uri, &prfx);
    if (hr == S_OK)
    {
        /* TODO: figure out what index argument is for */
        if (index) return E_FAIL;

        if (prefix)
        {
            if (*prefix_len < (INT)SysStringLen(prfx)) return E_XML_BUFFERTOOSMALL;
            lstrcpyW(prefix, prfx);
        }

        *prefix_len = SysStringLen(prfx);
        TRACE("prefix=%s\n", debugstr_w(prfx));
    }

    return hr;
}

static HRESULT WINAPI namespacemanager_getURI(IMXNamespaceManager *iface,
    const WCHAR *prefix, IXMLDOMNode *node, WCHAR *uri, int *uri_len)
{
    namespacemanager *This = impl_from_IMXNamespaceManager( iface );
    struct nscontext *ctxt;
    HRESULT hr;
    BSTR urib;

    TRACE("(%p)->(%s %p %p %p)\n", This, debugstr_w(prefix), node, uri, uri_len);

    if (!prefix) return E_INVALIDARG;
    if (!uri_len) return E_POINTER;

    if (node)
    {
        FIXME("namespaces from DOM node not supported\n");
        return E_NOTIMPL;
    }

    ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    hr = get_uri_from_prefix(ctxt, prefix, &urib);
    if (hr == S_OK)
    {
        if (uri)
        {
           if (*uri_len < (INT)SysStringLen(urib)) return E_XML_BUFFERTOOSMALL;
           lstrcpyW(uri, urib);
        }
    }
    else
        if (uri) *uri = 0;

    *uri_len = SysStringLen(urib);

    return hr;
}

static const struct IMXNamespaceManagerVtbl MXNamespaceManagerVtbl =
{
    namespacemanager_QueryInterface,
    namespacemanager_AddRef,
    namespacemanager_Release,
    namespacemanager_putAllowOverride,
    namespacemanager_getAllowOverride,
    namespacemanager_reset,
    namespacemanager_pushContext,
    namespacemanager_pushNodeContext,
    namespacemanager_popContext,
    namespacemanager_declarePrefix,
    namespacemanager_getDeclaredPrefix,
    namespacemanager_getPrefix,
    namespacemanager_getURI
};

static HRESULT WINAPI vbnamespacemanager_QueryInterface(IVBMXNamespaceManager *iface, REFIID riid, void **obj)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_IMXNamespaceManager) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *obj = &This->IMXNamespaceManager_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IVBMXNamespaceManager) ||
              IsEqualGUID( riid, &IID_IDispatch) )
    {
        *obj = &This->IVBMXNamespaceManager_iface;
    }
    else if (dispex_query_interface(&This->dispex, riid, obj))
    {
        return *obj ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IVBMXNamespaceManager_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI vbnamespacemanager_AddRef(IVBMXNamespaceManager *iface)
{
    namespacemanager *manager = impl_from_IVBMXNamespaceManager(iface);
    ULONG ref = InterlockedIncrement(&manager->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI vbnamespacemanager_Release(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        struct nscontext *ctxt, *ctxt2;

        LIST_FOR_EACH_ENTRY_SAFE(ctxt, ctxt2, &This->ctxts, struct nscontext, entry)
        {
            list_remove(&ctxt->entry);
            free_ns_context(ctxt);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI vbnamespacemanager_GetTypeInfoCount(IVBMXNamespaceManager *iface, UINT *pctinfo)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI vbnamespacemanager_GetTypeInfo(IVBMXNamespaceManager *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI vbnamespacemanager_GetIDsOfNames(IVBMXNamespaceManager *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI vbnamespacemanager_Invoke(IVBMXNamespaceManager *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI vbnamespacemanager_put_allowOverride(IVBMXNamespaceManager *iface,
    VARIANT_BOOL override)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );

    TRACE("(%p)->(%d)\n", This, override);
    This->override = override;

    return S_OK;
}

static HRESULT WINAPI vbnamespacemanager_get_allowOverride(IVBMXNamespaceManager *iface,
    VARIANT_BOOL *override)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );

    TRACE("(%p)->(%p)\n", This, override);

    if (!override) return E_POINTER;
    *override = This->override;

    return S_OK;
}

static HRESULT WINAPI vbnamespacemanager_reset(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_pushContext(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    struct nscontext *ctxt;

    TRACE("(%p)\n", This);

    ctxt = alloc_ns_context();
    if (!ctxt) return E_OUTOFMEMORY;

    list_add_head(&This->ctxts, &ctxt->entry);

    return S_OK;
}

static HRESULT WINAPI vbnamespacemanager_pushNodeContext(IVBMXNamespaceManager *iface,
    IXMLDOMNode *node, VARIANT_BOOL deep)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%p %d): stub\n", This, node, deep);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_popContext(IVBMXNamespaceManager *iface)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    const struct list *next;
    struct nscontext *ctxt;

    TRACE("(%p)\n", This);

    next = list_next(&This->ctxts, list_head(&This->ctxts));
    if (!next) return E_FAIL;

    ctxt = LIST_ENTRY(list_head(&This->ctxts), struct nscontext, entry);
    list_remove(list_head(&This->ctxts));

    free_ns_context(ctxt);

    return S_OK;
}

static HRESULT WINAPI vbnamespacemanager_declarePrefix(IVBMXNamespaceManager *iface,
    BSTR prefix, BSTR namespaceURI)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    return IMXNamespaceManager_declarePrefix(&This->IMXNamespaceManager_iface, prefix, namespaceURI);
}

static HRESULT WINAPI vbnamespacemanager_getDeclaredPrefixes(IVBMXNamespaceManager *iface,
    IMXNamespacePrefixes** prefixes)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%p): stub\n", This, prefixes);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getPrefixes(IVBMXNamespaceManager *iface,
    BSTR namespaceURI, IMXNamespacePrefixes** prefixes)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(namespaceURI), prefixes);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getURI(IVBMXNamespaceManager *iface,
    BSTR prefix, VARIANT* uri)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(prefix), uri);
    return E_NOTIMPL;
}

static HRESULT WINAPI vbnamespacemanager_getURIFromNode(IVBMXNamespaceManager *iface,
    BSTR prefix, IXMLDOMNode *node, VARIANT *uri)
{
    namespacemanager *This = impl_from_IVBMXNamespaceManager( iface );
    FIXME("(%p)->(%s %p %p): stub\n", This, debugstr_w(prefix), node, uri);
    return E_NOTIMPL;
}

static const struct IVBMXNamespaceManagerVtbl VBMXNamespaceManagerVtbl =
{
    vbnamespacemanager_QueryInterface,
    vbnamespacemanager_AddRef,
    vbnamespacemanager_Release,
    vbnamespacemanager_GetTypeInfoCount,
    vbnamespacemanager_GetTypeInfo,
    vbnamespacemanager_GetIDsOfNames,
    vbnamespacemanager_Invoke,
    vbnamespacemanager_put_allowOverride,
    vbnamespacemanager_get_allowOverride,
    vbnamespacemanager_reset,
    vbnamespacemanager_pushContext,
    vbnamespacemanager_pushNodeContext,
    vbnamespacemanager_popContext,
    vbnamespacemanager_declarePrefix,
    vbnamespacemanager_getDeclaredPrefixes,
    vbnamespacemanager_getPrefixes,
    vbnamespacemanager_getURI,
    vbnamespacemanager_getURIFromNode
};

static const tid_t namespacemanager_iface_tids[] = {
    IVBMXNamespaceManager_tid,
    0
};

static dispex_static_data_t namespacemanager_dispex = {
    NULL,
    IVBMXNamespaceManager_tid,
    NULL,
    namespacemanager_iface_tids
};

HRESULT MXNamespaceManager_create(void **obj)
{
    namespacemanager *This;
    struct nscontext *ctxt;

    TRACE("(%p)\n", obj);

    This = malloc(sizeof(*This));
    if( !This )
        return E_OUTOFMEMORY;

    This->IMXNamespaceManager_iface.lpVtbl = &MXNamespaceManagerVtbl;
    This->IVBMXNamespaceManager_iface.lpVtbl = &VBMXNamespaceManagerVtbl;
    This->ref = 1;
    init_dispex(&This->dispex, (IUnknown*)&This->IVBMXNamespaceManager_iface, &namespacemanager_dispex);

    list_init(&This->ctxts);
    ctxt = alloc_ns_context();
    if (!ctxt)
    {
        free(This);
        return E_OUTOFMEMORY;
    }

    list_add_head(&This->ctxts, &ctxt->entry);

    This->override = VARIANT_TRUE;

    *obj = &This->IMXNamespaceManager_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}
