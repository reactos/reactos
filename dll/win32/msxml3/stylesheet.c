/*
 *    XSLTemplate/XSLProcessor support
 *
 * Copyright 2011 Nikolay Sivov for CodeWeavers
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
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "initguid.h"
#include "asptlb.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    DispatchEx dispex;
    IXSLTemplate IXSLTemplate_iface;
    LONG ref;

    IXMLDOMNode *node;
} xsltemplate;

enum output_type
{
    PROCESSOR_OUTPUT_NOT_SET,
    PROCESSOR_OUTPUT_STREAM,        /* IStream or ISequentialStream */
    PROCESSOR_OUTPUT_PERSISTSTREAM, /* IPersistStream or IPersistStreamInit */
    PROCESSOR_OUTPUT_RESPONSE,      /* IResponse */
};

typedef struct
{
    DispatchEx dispex;
    IXSLProcessor IXSLProcessor_iface;
    LONG ref;

    xsltemplate *stylesheet;
    IXMLDOMNode *input;

    union
    {
        IUnknown *unk;
        ISequentialStream *stream;
        IPersistStream *persiststream;
        IResponse *response;
    } output;
    enum output_type output_type;
    BSTR outstr;

    struct xslprocessor_params params;
} xslprocessor;

static HRESULT XSLProcessor_create(xsltemplate*, IXSLProcessor**);

static inline xsltemplate *impl_from_IXSLTemplate( IXSLTemplate *iface )
{
    return CONTAINING_RECORD(iface, xsltemplate, IXSLTemplate_iface);
}

static inline xslprocessor *impl_from_IXSLProcessor( IXSLProcessor *iface )
{
    return CONTAINING_RECORD(iface, xslprocessor, IXSLProcessor_iface);
}

static void xslprocessor_par_free(struct xslprocessor_params *params, struct xslprocessor_par *par)
{
    params->count--;
    list_remove(&par->entry);
    SysFreeString(par->name);
    SysFreeString(par->value);
    free(par);
}

static void xsltemplate_set_node( xsltemplate *This, IXMLDOMNode *node )
{
    if (This->node) IXMLDOMNode_Release(This->node);
    This->node = node;
    if (node) IXMLDOMNode_AddRef(node);
}

static HRESULT WINAPI xsltemplate_QueryInterface(
    IXSLTemplate *iface,
    REFIID riid,
    void** ppvObject )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXSLTemplate ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI xsltemplate_AddRef( IXSLTemplate *iface )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xsltemplate_Release( IXSLTemplate *iface )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        if (This->node) IXMLDOMNode_Release( This->node );
        free( This );
    }

    return ref;
}

static HRESULT WINAPI xsltemplate_GetTypeInfoCount( IXSLTemplate *iface, UINT* pctinfo )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI xsltemplate_GetTypeInfo(
    IXSLTemplate *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI xsltemplate_GetIDsOfNames(
    IXSLTemplate *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI xsltemplate_Invoke(
    IXSLTemplate *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI xsltemplate_putref_stylesheet( IXSLTemplate *iface,
    IXMLDOMNode *node)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    TRACE("(%p)->(%p)\n", This, node);

    if (!node)
    {
        xsltemplate_set_node(This, NULL);
        return S_OK;
    }

    /* FIXME: test for document type */
    xsltemplate_set_node(This, node);

    return S_OK;
}

static HRESULT WINAPI xsltemplate_get_stylesheet( IXSLTemplate *iface,
    IXMLDOMNode **node)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    FIXME("(%p)->(%p): stub\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI xsltemplate_createProcessor( IXSLTemplate *iface,
    IXSLProcessor **processor)
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );

    TRACE("(%p)->(%p)\n", This, processor);

    if (!processor) return E_INVALIDARG;

    return XSLProcessor_create(This, processor);
}

static const struct IXSLTemplateVtbl XSLTemplateVtbl =
{
    xsltemplate_QueryInterface,
    xsltemplate_AddRef,
    xsltemplate_Release,
    xsltemplate_GetTypeInfoCount,
    xsltemplate_GetTypeInfo,
    xsltemplate_GetIDsOfNames,
    xsltemplate_Invoke,
    xsltemplate_putref_stylesheet,
    xsltemplate_get_stylesheet,
    xsltemplate_createProcessor
};

static const tid_t xsltemplate_iface_tids[] = {
    IXSLTemplate_tid,
    0
};

static dispex_static_data_t xsltemplate_dispex = {
    NULL,
    IXSLTemplate_tid,
    NULL,
    xsltemplate_iface_tids
};

HRESULT XSLTemplate_create(void **ppObj)
{
    xsltemplate *This;

    TRACE("(%p)\n", ppObj);

    This = malloc(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->IXSLTemplate_iface.lpVtbl = &XSLTemplateVtbl;
    This->ref = 1;
    This->node = NULL;
    init_dispex(&This->dispex, (IUnknown*)&This->IXSLTemplate_iface, &xsltemplate_dispex);

    *ppObj = &This->IXSLTemplate_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

/*** IXSLProcessor ***/
static HRESULT WINAPI xslprocessor_QueryInterface(
    IXSLProcessor *iface,
    REFIID riid,
    void** ppvObject )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXSLProcessor ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI xslprocessor_AddRef( IXSLProcessor *iface )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xslprocessor_Release( IXSLProcessor *iface )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        struct xslprocessor_par *par, *par2;

        if (This->input) IXMLDOMNode_Release(This->input);
        if (This->output.unk)
            IUnknown_Release(This->output.unk);
        SysFreeString(This->outstr);

        LIST_FOR_EACH_ENTRY_SAFE(par, par2, &This->params.list, struct xslprocessor_par, entry)
            xslprocessor_par_free(&This->params, par);

        IXSLTemplate_Release(&This->stylesheet->IXSLTemplate_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI xslprocessor_GetTypeInfoCount( IXSLProcessor *iface, UINT* pctinfo )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI xslprocessor_GetTypeInfo(
    IXSLProcessor *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI xslprocessor_GetIDsOfNames(
    IXSLProcessor *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI xslprocessor_Invoke(
    IXSLProcessor *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI xslprocessor_put_input( IXSLProcessor *iface, VARIANT input )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    IXMLDOMNode *input_node;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&input));

    /* try IXMLDOMNode directly first */
    if (V_VT(&input) == VT_UNKNOWN)
        hr = IUnknown_QueryInterface(V_UNKNOWN(&input), &IID_IXMLDOMNode, (void**)&input_node);
    else if (V_VT(&input) == VT_DISPATCH)
        hr = IDispatch_QueryInterface(V_DISPATCH(&input), &IID_IXMLDOMNode, (void**)&input_node);
    else
    {
        IXMLDOMDocument *doc;

        hr = dom_document_create(MSXML_DEFAULT, (void **)&doc);
        if (hr == S_OK)
        {
            VARIANT_BOOL b;

            hr = IXMLDOMDocument_load(doc, input, &b);
            if (hr == S_OK)
                hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&input_node);
            IXMLDOMDocument_Release(doc);
        }
    }

    if (hr == S_OK)
    {
        if (This->input) IXMLDOMNode_Release(This->input);
        This->input = input_node;
    }

    return hr;
}

static HRESULT WINAPI xslprocessor_get_input( IXSLProcessor *iface, VARIANT *input )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, input);
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_get_ownerTemplate(
    IXSLProcessor *iface,
    IXSLTemplate **template)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, template);
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_setStartMode(
    IXSLProcessor *iface,
    BSTR p,
    BSTR uri)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(p), debugstr_w(uri));
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_get_startMode(
    IXSLProcessor *iface,
    BSTR *p)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_get_startModeURI(
    IXSLProcessor *iface,
    BSTR *uri)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, uri);
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_put_output(
    IXSLProcessor *iface,
    VARIANT var)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    enum output_type output_type = PROCESSOR_OUTPUT_NOT_SET;
    IUnknown *output = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&var));

    switch (V_VT(&var))
    {
    case VT_EMPTY:
        break;
    case VT_UNKNOWN:
    case VT_DISPATCH:
        if (!V_UNKNOWN(&var))
            break;

        output_type = PROCESSOR_OUTPUT_STREAM;
        hr = IUnknown_QueryInterface(V_UNKNOWN(&var), &IID_IStream, (void **)&output);
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(V_UNKNOWN(&var), &IID_ISequentialStream, (void **)&output);
        if (FAILED(hr))
        {
            output_type = PROCESSOR_OUTPUT_RESPONSE;
            hr = IUnknown_QueryInterface(V_UNKNOWN(&var), &IID_IResponse, (void **)&output);
        }
        if (FAILED(hr))
        {
            output_type = PROCESSOR_OUTPUT_PERSISTSTREAM;
            hr = IUnknown_QueryInterface(V_UNKNOWN(&var), &IID_IPersistStream, (void **)&output);
        }
        if (FAILED(hr))
            hr = IUnknown_QueryInterface(V_UNKNOWN(&var), &IID_IPersistStreamInit, (void **)&output);
        if (FAILED(hr))
        {
            output_type = PROCESSOR_OUTPUT_NOT_SET;
            WARN("failed to get output interface, hr %#lx.\n", hr);
        }
        break;
    default:
        FIXME("output type %d not handled\n", V_VT(&var));
        hr = E_FAIL;
    }

    if (hr == S_OK)
    {
        if (This->output.unk)
            IUnknown_Release(This->output.unk);
        This->output.unk = output;
        This->output_type = output_type;
    }

    return hr;
}

static HRESULT WINAPI xslprocessor_get_output(
    IXSLProcessor *iface,
    VARIANT *output)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    TRACE("(%p)->(%p)\n", This, output);

    if (!output) return E_INVALIDARG;

    if (This->output.unk)
    {
        V_VT(output) = VT_UNKNOWN;
        V_UNKNOWN(output) = This->output.unk;
        IUnknown_AddRef(This->output.unk);
    }
    else if (This->outstr)
    {
        V_VT(output) = VT_BSTR;
        V_BSTR(output) = SysAllocString(This->outstr);
    }
    else
        V_VT(output) = VT_EMPTY;

    return S_OK;
}

static HRESULT WINAPI xslprocessor_transform(
    IXSLProcessor *iface,
    VARIANT_BOOL  *ret)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    ISequentialStream *stream = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ret);

    if (!ret)
        return E_INVALIDARG;

    if (This->output_type == PROCESSOR_OUTPUT_STREAM)
    {
        stream = This->output.stream;
        ISequentialStream_AddRef(stream);
    }
    else if (This->output_type == PROCESSOR_OUTPUT_PERSISTSTREAM ||
            This->output_type == PROCESSOR_OUTPUT_RESPONSE)
    {
        if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, (IStream **)&stream)))
            return hr;
    }

    SysFreeString(This->outstr);

    hr = node_transform_node_params(get_node_obj(This->input), This->stylesheet->node,
            &This->outstr, stream, &This->params);
    if (SUCCEEDED(hr))
    {
        IStream *src = (IStream *)stream;

        switch (This->output_type)
        {
        case PROCESSOR_OUTPUT_PERSISTSTREAM:
        {
            LARGE_INTEGER zero;

            /* for IPersistStream* output seekable stream is used */
            zero.QuadPart = 0;
            IStream_Seek(src, zero, STREAM_SEEK_SET, NULL);
            hr = IPersistStream_Load(This->output.persiststream, src);
            break;
        }
        case PROCESSOR_OUTPUT_RESPONSE:
        {
            SAFEARRAYBOUND bound;
            SAFEARRAY *array;
            HGLOBAL hglobal;
            VARIANT bin;
            DWORD size;
            void *dest;

            if (FAILED(hr = GetHGlobalFromStream(src, &hglobal)))
                break;
            size = GlobalSize(hglobal);

            bound.lLbound = 0;
            bound.cElements = size;
            if (!(array = SafeArrayCreate(VT_UI1, 1, &bound)))
                break;

            V_VT(&bin) = VT_ARRAY | VT_UI1;
            V_ARRAY(&bin) = array;

            hr = SafeArrayAccessData(array, &dest);
            if (hr == S_OK)
            {
                void *data = GlobalLock(hglobal);
                memcpy(dest, data, size);
                GlobalUnlock(hglobal);
                SafeArrayUnaccessData(array);

                IResponse_BinaryWrite(This->output.response, bin);
            }

            VariantClear(&bin);
            break;
        }
        default:
            ;
        }
    }

    if (stream)
        ISequentialStream_Release(stream);

    *ret = hr == S_OK ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;
}

static HRESULT WINAPI xslprocessor_reset( IXSLProcessor *iface )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_get_readyState(
    IXSLProcessor *iface,
    LONG *state)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, state);
    return E_NOTIMPL;
}

static HRESULT xslprocessor_set_parvalue(const VARIANT *var, struct xslprocessor_par *par)
{
    HRESULT hr = S_OK;

    switch (V_VT(var))
    {
    case VT_BSTR:
    {
        par->value = SysAllocString(V_BSTR(var));
        if (!par->value) hr = E_OUTOFMEMORY;
        break;
    }
    default:
        FIXME("value type %d not handled\n", V_VT(var));
        hr = E_NOTIMPL;
    }

    return hr;
}

static HRESULT WINAPI xslprocessor_addParameter(
    IXSLProcessor *iface,
    BSTR p,
    VARIANT var,
    BSTR uri)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    struct xslprocessor_par *cur, *par = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(p), debugstr_variant(&var),
        debugstr_w(uri));

    if (uri && *uri)
        FIXME("namespace uri is not supported\n");

    /* search for existing parameter first */
    LIST_FOR_EACH_ENTRY(cur, &This->params.list, struct xslprocessor_par, entry)
    {
        if (!wcscmp(cur->name, p))
        {
            par = cur;
            break;
        }
    }

    /* override with new value or add new parameter */
    if (par)
    {
        if (V_VT(&var) == VT_NULL || V_VT(&var) == VT_EMPTY)
        {
            /* remove parameter */
            xslprocessor_par_free(&This->params, par);
            return S_OK;
        }
        SysFreeString(par->value);
        par->value = NULL;
    }
    else
    {
        /* new parameter */
        par = malloc(sizeof(struct xslprocessor_par));
        if (!par) return E_OUTOFMEMORY;

        par->name = SysAllocString(p);
        if (!par->name)
        {
            free(par);
            return E_OUTOFMEMORY;
        }
        list_add_tail(&This->params.list, &par->entry);
        This->params.count++;
    }

    hr = xslprocessor_set_parvalue(&var, par);
    if (FAILED(hr))
        xslprocessor_par_free(&This->params, par);

    return hr;
}

static HRESULT WINAPI xslprocessor_addObject(
    IXSLProcessor *iface,
    IDispatch *obj,
    BSTR uri)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p %s): stub\n", This, obj, debugstr_w(uri));
    return E_NOTIMPL;
}

static HRESULT WINAPI xslprocessor_get_stylesheet(
    IXSLProcessor *iface,
    IXMLDOMNode  **node)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%p): stub\n", This, node);
    return E_NOTIMPL;
}

static const struct IXSLProcessorVtbl XSLProcessorVtbl =
{
    xslprocessor_QueryInterface,
    xslprocessor_AddRef,
    xslprocessor_Release,
    xslprocessor_GetTypeInfoCount,
    xslprocessor_GetTypeInfo,
    xslprocessor_GetIDsOfNames,
    xslprocessor_Invoke,
    xslprocessor_put_input,
    xslprocessor_get_input,
    xslprocessor_get_ownerTemplate,
    xslprocessor_setStartMode,
    xslprocessor_get_startMode,
    xslprocessor_get_startModeURI,
    xslprocessor_put_output,
    xslprocessor_get_output,
    xslprocessor_transform,
    xslprocessor_reset,
    xslprocessor_get_readyState,
    xslprocessor_addParameter,
    xslprocessor_addObject,
    xslprocessor_get_stylesheet
};

static const tid_t xslprocessor_iface_tids[] = {
    IXSLProcessor_tid,
    0
};

static dispex_static_data_t xslprocessor_dispex = {
    NULL,
    IXSLProcessor_tid,
    NULL,
    xslprocessor_iface_tids
};

HRESULT XSLProcessor_create(xsltemplate *template, IXSLProcessor **ppObj)
{
    xslprocessor *This;

    TRACE("(%p)\n", ppObj);

    This = malloc(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->IXSLProcessor_iface.lpVtbl = &XSLProcessorVtbl;
    This->ref = 1;
    This->input = NULL;
    This->output.unk = NULL;
    This->output_type = PROCESSOR_OUTPUT_NOT_SET;
    This->outstr = NULL;
    list_init(&This->params.list);
    This->params.count = 0;
    This->stylesheet = template;
    IXSLTemplate_AddRef(&template->IXSLTemplate_iface);
    init_dispex(&This->dispex, (IUnknown*)&This->IXSLProcessor_iface, &xslprocessor_dispex);

    *ppObj = &This->IXSLProcessor_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}
