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

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS

#include <config.h>

//#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
//# include <libxml/xmlerror.h>
#endif

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <ole2.h>
#include <msxml6.h>

#include "msxml_private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    DispatchEx dispex;
    IXSLTemplate IXSLTemplate_iface;
    LONG ref;

    IXMLDOMNode *node;
} xsltemplate;

typedef struct
{
    DispatchEx dispex;
    IXSLProcessor IXSLProcessor_iface;
    LONG ref;

    xsltemplate *stylesheet;
    IXMLDOMNode *input;

    IStream     *output;
    BSTR         outstr;
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
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xsltemplate_Release( IXSLTemplate *iface )
{
    xsltemplate *This = impl_from_IXSLTemplate( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref);
    if ( ref == 0 )
    {
        if (This->node) IXMLDOMNode_Release( This->node );
        release_dispex(&This->dispex);
        heap_free( This );
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

HRESULT XSLTemplate_create(IUnknown *outer, void **ppObj)
{
    xsltemplate *This;

    TRACE("(%p, %p)\n", outer, ppObj);

    if(outer) FIXME("support aggregation, outer\n");

    This = heap_alloc( sizeof (*This) );
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
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xslprocessor_Release( IXSLProcessor *iface )
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref);
    if ( ref == 0 )
    {
        if (This->input) IXMLDOMNode_Release(This->input);
        if (This->output) IStream_Release(This->output);
        SysFreeString(This->outstr);
        IXSLTemplate_Release(&This->stylesheet->IXSLTemplate_iface);
        release_dispex(&This->dispex);
        heap_free( This );
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

        hr = DOMDocument_create(MSXML_DEFAULT, NULL, (void**)&doc);
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
    VARIANT output)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );
    IStream *stream;
    HRESULT hr;

    FIXME("(%p)->(%s): semi-stub\n", This, debugstr_variant(&output));

    switch (V_VT(&output))
    {
      case VT_EMPTY:
        stream = NULL;
        hr = S_OK;
        break;
      case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&output), &IID_IStream, (void**)&stream);
        break;
      default:
        hr = E_FAIL;
    }

    if (hr == S_OK)
    {
        if (This->output) IStream_Release(This->output);
        This->output = stream;
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

    if (This->output)
    {
        V_VT(output) = VT_UNKNOWN;
        V_UNKNOWN(output) = (IUnknown*)This->output;
        IStream_AddRef(This->output);
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
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ret);

    if (!ret) return E_INVALIDARG;

    SysFreeString(This->outstr);
    hr = IXMLDOMNode_transformNode(This->input, This->stylesheet->node, &This->outstr);
    if (hr == S_OK)
    {
        if (This->output)
        {
            ULONG len = 0;

            /* output to stream */
            hr = IStream_Write(This->output, This->outstr, SysStringByteLen(This->outstr), &len);
            *ret = len == SysStringByteLen(This->outstr) ? VARIANT_TRUE : VARIANT_FALSE;
        }
    }
    else
        *ret = VARIANT_FALSE;

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

static HRESULT WINAPI xslprocessor_addParameter(
    IXSLProcessor *iface,
    BSTR p,
    VARIANT var,
    BSTR uri)
{
    xslprocessor *This = impl_from_IXSLProcessor( iface );

    FIXME("(%p)->(%s %s %s): stub\n", This, debugstr_w(p), debugstr_variant(&var),
        debugstr_w(uri));
    return E_NOTIMPL;
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

    This = heap_alloc( sizeof (*This) );
    if(!This)
        return E_OUTOFMEMORY;

    This->IXSLProcessor_iface.lpVtbl = &XSLProcessorVtbl;
    This->ref = 1;
    This->input = NULL;
    This->output = NULL;
    This->outstr = NULL;
    This->stylesheet = template;
    IXSLTemplate_AddRef(&template->IXSLTemplate_iface);
    init_dispex(&This->dispex, (IUnknown*)&This->IXSLProcessor_iface, &xslprocessor_dispex);

    *ppObj = &This->IXSLProcessor_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}
