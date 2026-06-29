/*
 *    XPath/XSLPattern query result node list implementation
 *
 * Copyright 2005 Mike McCormack
 * Copyright 2007 Mikolaj Zalewski
 * Copyright 2010 Adam Martinson for CodeWeavers
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
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml2did.h"

#include "msxml_private.h"

#include "wine/debug.h"

/* This file implements the object returned by a XPath query. Note that this is
 * not the IXMLDOMNodeList returned by childNodes - it's implemented in nodelist.c.
 * They are different because the list returned by XPath queries:
 *  - is static - gives the results for the XML tree as it existed during the
 *    execution of the query
 *  - supports IXMLDOMSelection
 *
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

int registerNamespaces(xmlXPathContextPtr ctxt);
xmlChar* XSLPattern_to_XPath(xmlXPathContextPtr ctxt, xmlChar const* xslpat_str);

typedef struct
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    IUnknown *outer;
    BOOL own;

    LONG pos;

    const struct enumvariant_funcs *funcs;
} enumvariant;

typedef struct
{
    DispatchEx dispex;
    IXMLDOMSelection IXMLDOMSelection_iface;
    LONG ref;
    xmlNodePtr node;
    xmlXPathObjectPtr result;
    int resultPos;
    IEnumVARIANT *enumvariant;
} domselection;

static HRESULT selection_get_item(IUnknown *iface, LONG index, VARIANT* item)
{
    V_VT(item) = VT_DISPATCH;
    return IXMLDOMSelection_get_item((IXMLDOMSelection*)iface, index, (IXMLDOMNode**)&V_DISPATCH(item));
}

static HRESULT selection_next(IUnknown *iface)
{
    IXMLDOMNode *node;
    HRESULT hr = IXMLDOMSelection_nextNode((IXMLDOMSelection*)iface, &node);
    if (hr == S_OK) IXMLDOMNode_Release(node);
    return hr;
}

static const struct enumvariant_funcs selection_enumvariant = {
    selection_get_item,
    selection_next
};

static inline domselection *impl_from_IXMLDOMSelection( IXMLDOMSelection *iface )
{
    return CONTAINING_RECORD(iface, domselection, IXMLDOMSelection_iface);
}

static inline enumvariant *impl_from_IEnumVARIANT( IEnumVARIANT *iface )
{
    return CONTAINING_RECORD(iface, enumvariant, IEnumVARIANT_iface);
}

static HRESULT WINAPI domselection_QueryInterface(
    IXMLDOMSelection *iface,
    REFIID riid,
    void** ppvObject )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppvObject);

    if(!ppvObject)
        return E_INVALIDARG;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) ||
         IsEqualGUID( riid, &IID_IXMLDOMSelection ))
    {
        *ppvObject = &This->IXMLDOMSelection_iface;
    }
    else if (IsEqualGUID( riid, &IID_IEnumVARIANT ))
    {
        if (!This->enumvariant)
        {
            HRESULT hr = create_enumvariant((IUnknown*)iface, FALSE, &selection_enumvariant, &This->enumvariant);
            if (FAILED(hr)) return hr;
        }

        return IEnumVARIANT_QueryInterface(This->enumvariant, &IID_IEnumVARIANT, ppvObject);
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMSelection_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI domselection_AddRef(
    IXMLDOMSelection *iface )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domselection_Release(
    IXMLDOMSelection *iface )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        xmlXPathFreeObject(This->result);
        xmldoc_release(This->node->doc);
        if (This->enumvariant) IEnumVARIANT_Release(This->enumvariant);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI domselection_GetTypeInfoCount(
    IXMLDOMSelection *iface,
    UINT* pctinfo )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domselection_GetTypeInfo(
    IXMLDOMSelection *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domselection_GetIDsOfNames(
    IXMLDOMSelection *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domselection_Invoke(
    IXMLDOMSelection *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domselection_get_item(
        IXMLDOMSelection* iface,
        LONG index,
        IXMLDOMNode** listItem)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("%p, %ld, %p.\n", iface, index, listItem);

    if(!listItem)
        return E_INVALIDARG;

    *listItem = NULL;

    if (index < 0 || index >= xmlXPathNodeSetGetLength(This->result->nodesetval))
        return S_FALSE;

    *listItem = create_node(xmlXPathNodeSetItem(This->result->nodesetval, index));
    This->resultPos = index + 1;

    return S_OK;
}

static HRESULT WINAPI domselection_get_length(
        IXMLDOMSelection* iface,
        LONG* listLength)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("(%p)->(%p)\n", This, listLength);

    if(!listLength)
        return E_INVALIDARG;

    *listLength = xmlXPathNodeSetGetLength(This->result->nodesetval);
    return S_OK;
}

static HRESULT WINAPI domselection_nextNode(
        IXMLDOMSelection* iface,
        IXMLDOMNode** nextItem)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("(%p)->(%p)\n", This, nextItem );

    if(!nextItem)
        return E_INVALIDARG;

    *nextItem = NULL;

    if (This->resultPos >= xmlXPathNodeSetGetLength(This->result->nodesetval))
        return S_FALSE;

    *nextItem = create_node(xmlXPathNodeSetItem(This->result->nodesetval, This->resultPos));
    This->resultPos++;
    return S_OK;
}

static HRESULT WINAPI domselection_reset(
        IXMLDOMSelection* iface)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("%p\n", This);
    This->resultPos = 0;
    return S_OK;
}

static HRESULT WINAPI domselection_get__newEnum(
        IXMLDOMSelection* iface,
        IUnknown** enumv)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );

    TRACE("(%p)->(%p)\n", This, enumv);

    return create_enumvariant((IUnknown*)iface, TRUE, &selection_enumvariant, (IEnumVARIANT**)enumv);
}

static HRESULT WINAPI domselection_get_expr(
        IXMLDOMSelection* iface,
        BSTR *p)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_put_expr(
        IXMLDOMSelection* iface,
        BSTR p)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_w(p));
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_get_context(
        IXMLDOMSelection* iface,
        IXMLDOMNode **node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_putref_context(
        IXMLDOMSelection* iface,
        IXMLDOMNode *node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_peekNode(
        IXMLDOMSelection* iface,
        IXMLDOMNode **node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_matches(
        IXMLDOMSelection* iface,
        IXMLDOMNode *node,
        IXMLDOMNode **out_node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p %p)\n", This, node, out_node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_removeNext(
        IXMLDOMSelection* iface,
        IXMLDOMNode **node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_removeAll(
        IXMLDOMSelection* iface)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_clone(
        IXMLDOMSelection* iface,
        IXMLDOMSelection **node)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_getProperty(
        IXMLDOMSelection* iface,
        BSTR p,
        VARIANT *var)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(p), var);
    return E_NOTIMPL;
}

static HRESULT WINAPI domselection_setProperty(
        IXMLDOMSelection* iface,
        BSTR p,
        VARIANT var)
{
    domselection *This = impl_from_IXMLDOMSelection( iface );
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(p), debugstr_variant(&var));
    return E_NOTIMPL;
}

static const struct IXMLDOMSelectionVtbl domselection_vtbl =
{
    domselection_QueryInterface,
    domselection_AddRef,
    domselection_Release,
    domselection_GetTypeInfoCount,
    domselection_GetTypeInfo,
    domselection_GetIDsOfNames,
    domselection_Invoke,
    domselection_get_item,
    domselection_get_length,
    domselection_nextNode,
    domselection_reset,
    domselection_get__newEnum,
    domselection_get_expr,
    domselection_put_expr,
    domselection_get_context,
    domselection_putref_context,
    domselection_peekNode,
    domselection_matches,
    domselection_removeNext,
    domselection_removeAll,
    domselection_clone,
    domselection_getProperty,
    domselection_setProperty
};

/* IEnumVARIANT support */
static HRESULT WINAPI enumvariant_QueryInterface(
    IEnumVARIANT *iface,
    REFIID riid,
    void** ppvObject )
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID( riid, &IID_IUnknown ))
    {
        if (This->own)
            *ppvObject = &This->IEnumVARIANT_iface;
        else
            return IUnknown_QueryInterface(This->outer, riid, ppvObject);
    }
    else if (IsEqualGUID( riid, &IID_IEnumVARIANT ))
    {
        *ppvObject = &This->IEnumVARIANT_iface;
    }
    else
        return IUnknown_QueryInterface(This->outer, riid, ppvObject);

    IEnumVARIANT_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI enumvariant_AddRef(IEnumVARIANT *iface )
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI enumvariant_Release(IEnumVARIANT *iface )
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        if (This->own) IUnknown_Release(This->outer);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI enumvariant_Next(
    IEnumVARIANT *iface,
    ULONG celt,
    VARIANT *var,
    ULONG *fetched)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    ULONG ret_count = 0;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, var, fetched);

    if (fetched) *fetched = 0;

    if (celt && !var) return E_INVALIDARG;

    for (; celt > 0; celt--, var++, This->pos++)
    {
        HRESULT hr = This->funcs->get_item(This->outer, This->pos, var);
        if (hr != S_OK)
        {
            V_VT(var) = VT_EMPTY;
            break;
        }
        ret_count++;
    }

    if (fetched) *fetched = ret_count;

    /* we need to advance one step more for some reason */
    if (ret_count)
    {
        if (This->funcs->next)
            This->funcs->next(This->outer);
    }

    return celt == 0 ? S_OK : S_FALSE;
}

static HRESULT WINAPI enumvariant_Skip(
    IEnumVARIANT *iface,
    ULONG celt)
{
    FIXME("%p, %lu: stub\n", iface, celt);

    return E_NOTIMPL;
}

static HRESULT WINAPI enumvariant_Reset(IEnumVARIANT *iface)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );

    TRACE("%p\n", This);
    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI enumvariant_Clone(
    IEnumVARIANT *iface, IEnumVARIANT **ppenum)
{
    enumvariant *This = impl_from_IEnumVARIANT( iface );
    FIXME("(%p)->(%p): stub\n", This, ppenum);
    return E_NOTIMPL;
}

static const struct IEnumVARIANTVtbl EnumVARIANTVtbl =
{
    enumvariant_QueryInterface,
    enumvariant_AddRef,
    enumvariant_Release,
    enumvariant_Next,
    enumvariant_Skip,
    enumvariant_Reset,
    enumvariant_Clone
};

HRESULT create_enumvariant(IUnknown *outer, BOOL own, const struct enumvariant_funcs *funcs, IEnumVARIANT **penum)
{
    enumvariant *This;

    This = malloc(sizeof(enumvariant));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumVARIANT_iface.lpVtbl = &EnumVARIANTVtbl;
    This->ref = 0;
    This->outer = outer;
    This->own = own;
    This->pos = 0;
    This->funcs = funcs;

    if (This->own)
        IUnknown_AddRef(This->outer);

    *penum = &This->IEnumVARIANT_iface;
    IEnumVARIANT_AddRef(*penum);
    return S_OK;
}

static HRESULT domselection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    WCHAR *ptr;
    int idx = 0;

    for(ptr = name; *ptr >= '0' && *ptr <= '9'; ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_DOM_COLLECTION_BASE + idx;
    TRACE("ret %lx\n", *dispid);
    return S_OK;
}

static HRESULT domselection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    domselection *This = impl_from_IXMLDOMSelection( (IXMLDOMSelection*)iface );

    TRACE("%p, %ld, %lx, %x, %p, %p, %p.\n", iface, id, lcid, flags, params, res, ei);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = NULL;

    if (id < DISPID_DOM_COLLECTION_BASE || id > DISPID_DOM_COLLECTION_MAX)
        return DISP_E_UNKNOWNNAME;

    switch(flags)
    {
        case INVOKE_PROPERTYGET:
        {
            IXMLDOMNode *disp = NULL;

            IXMLDOMSelection_get_item(&This->IXMLDOMSelection_iface, id - DISPID_DOM_COLLECTION_BASE, &disp);
            V_DISPATCH(res) = (IDispatch*)disp;
            break;
        }
        default:
        {
            FIXME("unimplemented flags %x\n", flags);
            break;
        }
    }

    TRACE("ret %p\n", V_DISPATCH(res));

    return S_OK;
}

static const dispex_static_data_vtbl_t domselection_dispex_vtbl = {
    domselection_get_dispid,
    domselection_invoke
};

static const tid_t domselection_iface_tids[] = {
    IXMLDOMSelection_tid,
    0
};
static dispex_static_data_t domselection_dispex = {
    &domselection_dispex_vtbl,
    IXMLDOMSelection_tid,
    NULL,
    domselection_iface_tids
};

#define XSLPATTERN_CHECK_ARGS(n) \
    if (nargs != n) { \
        FIXME("XSLPattern syntax error: Expected %i arguments, got %i\n", n, nargs); \
        xmlXPathSetArityError(pctx); \
        return; \
    }


static void XSLPattern_index(xmlXPathParserContextPtr pctx, int nargs)
{
    XSLPATTERN_CHECK_ARGS(0);

    xmlXPathPositionFunction(pctx, 0);
    xmlXPathReturnNumber(pctx, xmlXPathPopNumber(pctx) - 1.0);
}

static void XSLPattern_end(xmlXPathParserContextPtr pctx, int nargs)
{
    double pos, last;
    XSLPATTERN_CHECK_ARGS(0);

    xmlXPathPositionFunction(pctx, 0);
    pos = xmlXPathPopNumber(pctx);
    xmlXPathLastFunction(pctx, 0);
    last = xmlXPathPopNumber(pctx);
    xmlXPathReturnBoolean(pctx, pos == last);
}

static void XSLPattern_nodeType(xmlXPathParserContextPtr pctx, int nargs)
{
    XSLPATTERN_CHECK_ARGS(0);
    xmlXPathReturnNumber(pctx, pctx->context->node->type);
}

static void XSLPattern_OP_IEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) == 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_INEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) != 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_ILt(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) < 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_ILEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) <= 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_IGt(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) > 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void XSLPattern_OP_IGEq(xmlXPathParserContextPtr pctx, int nargs)
{
    xmlChar *arg1, *arg2;
    XSLPATTERN_CHECK_ARGS(2);

    arg2 = xmlXPathPopString(pctx);
    arg1 = xmlXPathPopString(pctx);
    xmlXPathReturnBoolean(pctx, xmlStrcasecmp(arg1, arg2) >= 0);
    xmlFree(arg1);
    xmlFree(arg2);
}

static void query_serror(void* ctx, const xmlError* err)
{
    LIBXML2_CALLBACK_SERROR(domselection_create, err);
}

HRESULT create_selection(xmlNodePtr node, xmlChar* query, IXMLDOMNodeList **out)
{
    domselection *This = malloc(sizeof(domselection));
    xmlXPathContextPtr ctxt = xmlXPathNewContext(node->doc);
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", node, debugstr_a((char const*)query), out);

    *out = NULL;
    if (!This || !ctxt || !query)
    {
        xmlXPathFreeContext(ctxt);
        free(This);
        return E_OUTOFMEMORY;
    }

    This->IXMLDOMSelection_iface.lpVtbl = &domselection_vtbl;
    This->ref = 1;
    This->resultPos = 0;
    This->node = node;
    This->enumvariant = NULL;
    init_dispex(&This->dispex, (IUnknown*)&This->IXMLDOMSelection_iface, &domselection_dispex);
    xmldoc_add_ref(This->node->doc);

    ctxt->error = query_serror;
    ctxt->node = node;
    registerNamespaces(ctxt);
    xmlXPathContextSetCache(ctxt, 1, -1, 0);

    if (is_xpathmode(This->node->doc))
    {
        xmlXPathRegisterAllFunctions(ctxt);
        This->result = xmlXPathEvalExpression(query, ctxt);
    }
    else
    {
        xmlChar* pattern_query = XSLPattern_to_XPath(ctxt, query);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"not", xmlXPathNotFunction);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"boolean", xmlXPathBooleanFunction);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"index", XSLPattern_index);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"end", XSLPattern_end);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"nodeType", XSLPattern_nodeType);

        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IEq", XSLPattern_OP_IEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_INEq", XSLPattern_OP_INEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_ILt", XSLPattern_OP_ILt);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_ILEq", XSLPattern_OP_ILEq);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IGt", XSLPattern_OP_IGt);
        xmlXPathRegisterFunc(ctxt, (xmlChar const*)"OP_IGEq", XSLPattern_OP_IGEq);

        This->result = xmlXPathEvalExpression(pattern_query, ctxt);
        xmlFree(pattern_query);
    }

    if (!This->result || This->result->type != XPATH_NODESET)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    *out = (IXMLDOMNodeList*)&This->IXMLDOMSelection_iface;
    hr = S_OK;
    TRACE("found %d matches\n", xmlXPathNodeSetGetLength(This->result->nodesetval));

cleanup:
    if (FAILED(hr))
        IXMLDOMSelection_Release( &This->IXMLDOMSelection_iface );
    xmlXPathFreeContext(ctxt);
    return hr;
}
