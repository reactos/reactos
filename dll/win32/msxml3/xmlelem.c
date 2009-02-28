/*
 * XML Element implementation
 *
 * Copyright 2007 James Hawkins
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"
#include "ocidl.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

static HRESULT XMLElementCollection_create( IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj );

/**********************************************************************
 * IXMLElement
 */
typedef struct _xmlelem
{
    const IXMLElementVtbl *lpVtbl;
    LONG ref;
    xmlNodePtr node;
} xmlelem;

static inline xmlelem *impl_from_IXMLElement(IXMLElement *iface)
{
    return (xmlelem *)((char*)iface - FIELD_OFFSET(xmlelem, lpVtbl));
}

static HRESULT WINAPI xmlelem_QueryInterface(IXMLElement *iface, REFIID riid, void** ppvObject)
{
    xmlelem *This = impl_from_IXMLElement(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXMLElement))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLElement_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlelem_AddRef(IXMLElement *iface)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlelem_Release(IXMLElement *iface)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI xmlelem_GetTypeInfoCount(IXMLElement *iface, UINT* pctinfo)
{
    xmlelem *This = impl_from_IXMLElement(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI xmlelem_GetTypeInfo(IXMLElement *iface, UINT iTInfo,
                                          LCID lcid, ITypeInfo** ppTInfo)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLElement_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI xmlelem_GetIDsOfNames(IXMLElement *iface, REFIID riid,
                                            LPOLESTR* rgszNames, UINT cNames,
                                            LCID lcid, DISPID* rgDispId)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLElement_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmlelem_Invoke(IXMLElement *iface, DISPID dispIdMember,
                                     REFIID riid, LCID lcid, WORD wFlags,
                                     DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                     EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLElement_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static inline BSTR str_dup_upper(BSTR str)
{
    INT len = (lstrlenW(str) + 1) * sizeof(WCHAR);
    BSTR p = SysAllocStringLen(NULL, len);
    if (p)
    {
        memcpy(p, str, len);
        CharUpperW(p);
    }
    return p;
}

static HRESULT WINAPI xmlelem_get_tagName(IXMLElement *iface, BSTR *p)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    BSTR temp;

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    temp = bstr_from_xmlChar(This->node->name);
    *p = str_dup_upper(temp);
    SysFreeString(temp);

    TRACE("returning %s\n", debugstr_w(*p));

    return S_OK;
}

static HRESULT WINAPI xmlelem_put_tagName(IXMLElement *iface, BSTR p)
{
    FIXME("(%p, %p): stub\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_get_parent(IXMLElement *iface, IXMLElement **parent)
{
    xmlelem *This = impl_from_IXMLElement(iface);

    TRACE("(%p, %p)\n", iface, parent);

    if (!parent)
        return E_INVALIDARG;

    *parent = NULL;

    if (!This->node->parent)
        return S_FALSE;

    return XMLElement_create((IUnknown *)iface, This->node->parent, (LPVOID *)parent);
}

static HRESULT WINAPI xmlelem_setAttribute(IXMLElement *iface, BSTR strPropertyName,
                                            VARIANT PropertyValue)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlChar *name, *value;
    xmlAttrPtr attr;

    TRACE("(%p, %s)\n", iface, debugstr_w(strPropertyName));

    if (!strPropertyName || V_VT(&PropertyValue) != VT_BSTR)
        return E_INVALIDARG;

    name = xmlChar_from_wchar(strPropertyName);
    value = xmlChar_from_wchar(V_BSTR(&PropertyValue));
    attr = xmlSetProp(This->node, name, value);

    HeapFree(GetProcessHeap(), 0, name);
    HeapFree(GetProcessHeap(), 0, value);
    return (attr) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_getAttribute(IXMLElement *iface, BSTR strPropertyName,
                                           VARIANT *PropertyValue)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlChar *val = NULL, *name;
    xmlAttrPtr ptr;

    TRACE("(%p, %s, %p)\n", iface, debugstr_w(strPropertyName), PropertyValue);

    if (!PropertyValue)
        return E_INVALIDARG;

    VariantInit(PropertyValue);
    V_BSTR(PropertyValue) = NULL;

    if (!strPropertyName)
        return E_INVALIDARG;

    name = xmlChar_from_wchar(strPropertyName);
    ptr = This->node->properties;
    while (ptr)
    {
        if (!lstrcmpiA((LPSTR)name, (LPCSTR)ptr->name))
        {
            val = xmlNodeListGetString(ptr->doc, ptr->children, 1);
            break;
        }

        ptr = ptr->next;
    }

    if (val)
    {
        V_VT(PropertyValue) = VT_BSTR;
        V_BSTR(PropertyValue) = bstr_from_xmlChar(val);
    }

    HeapFree(GetProcessHeap(), 0, name);
    xmlFree(val);
    TRACE("returning %s\n", debugstr_w(V_BSTR(PropertyValue)));
    return (val) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_removeAttribute(IXMLElement *iface, BSTR strPropertyName)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlChar *name;
    xmlAttrPtr attr;
    int res;
    HRESULT hr = S_FALSE;

    TRACE("(%p, %s)\n", iface, debugstr_w(strPropertyName));

    if (!strPropertyName)
        return E_INVALIDARG;

    name = xmlChar_from_wchar(strPropertyName);
    attr = xmlHasProp(This->node, name);
    if (!attr)
        goto done;

    res = xmlRemoveProp(attr);

    if (res == 0)
        hr = S_OK;

done:
    HeapFree(GetProcessHeap(), 0, name);
    return hr;
}

static HRESULT WINAPI xmlelem_get_children(IXMLElement *iface, IXMLElementCollection **p)
{
    xmlelem *This = impl_from_IXMLElement(iface);

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    return XMLElementCollection_create((IUnknown *)iface, This->node->children, (LPVOID *)p);
}

static long type_libxml_to_msxml(xmlElementType type)
{
    switch (type)
    {
        case XML_ELEMENT_NODE:
            return XMLELEMTYPE_ELEMENT;
        case XML_TEXT_NODE:
            return XMLELEMTYPE_TEXT;
        case XML_COMMENT_NODE:
            return XMLELEMTYPE_COMMENT;
        case XML_DOCUMENT_NODE:
            return XMLELEMTYPE_DOCUMENT;
        case XML_DTD_NODE:
            return XMLELEMTYPE_DTD;
        case XML_PI_NODE:
            return XMLELEMTYPE_PI;
        default:
            break;
    }

    return XMLELEMTYPE_OTHER;
}

static HRESULT WINAPI xmlelem_get_type(IXMLElement *iface, long *p)
{
    xmlelem *This = impl_from_IXMLElement(iface);

    TRACE("(%p, %p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    *p = type_libxml_to_msxml(This->node->type);
    TRACE("returning %ld\n", *p);
    return S_OK;
}

static HRESULT WINAPI xmlelem_get_text(IXMLElement *iface, BSTR *p)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlChar *content;

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    content = xmlNodeGetContent(This->node);
    *p = bstr_from_xmlChar(content);
    TRACE("returning %s\n", debugstr_w(*p));

    xmlFree(content);
    return S_OK;
}

static HRESULT WINAPI xmlelem_put_text(IXMLElement *iface, BSTR p)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlChar *content;

    TRACE("(%p, %s)\n", iface, debugstr_w(p));

    /* FIXME: test which types can be used */
    if (This->node->type == XML_ELEMENT_NODE)
        return E_NOTIMPL;

    content = xmlChar_from_wchar(p);
    xmlNodeSetContent(This->node, content);

    HeapFree( GetProcessHeap(), 0, content);

    return S_OK;
}

static HRESULT WINAPI xmlelem_addChild(IXMLElement *iface, IXMLElement *pChildElem,
                                        long lIndex, long lreserved)
{
    xmlelem *This = impl_from_IXMLElement(iface);
    xmlelem *childElem = impl_from_IXMLElement(pChildElem);
    xmlNodePtr child;

    TRACE("(%p, %p, %ld, %ld)\n", iface, pChildElem, lIndex, lreserved);

    if (lIndex == 0)
        child = xmlAddChild(This->node, childElem->node);
    else
        child = xmlAddNextSibling(This->node, childElem->node->last);

    return (child) ? S_OK : S_FALSE;
}

static HRESULT WINAPI xmlelem_removeChild(IXMLElement *iface, IXMLElement *pChildElem)
{
    FIXME("(%p, %p): stub\n", iface, pChildElem);
    return E_NOTIMPL;
}

static const struct IXMLElementVtbl xmlelem_vtbl =
{
    xmlelem_QueryInterface,
    xmlelem_AddRef,
    xmlelem_Release,
    xmlelem_GetTypeInfoCount,
    xmlelem_GetTypeInfo,
    xmlelem_GetIDsOfNames,
    xmlelem_Invoke,
    xmlelem_get_tagName,
    xmlelem_put_tagName,
    xmlelem_get_parent,
    xmlelem_setAttribute,
    xmlelem_getAttribute,
    xmlelem_removeAttribute,
    xmlelem_get_children,
    xmlelem_get_type,
    xmlelem_get_text,
    xmlelem_put_text,
    xmlelem_addChild,
    xmlelem_removeChild
};

HRESULT XMLElement_create(IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj)
{
    xmlelem *elem;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if (!ppObj)
        return E_INVALIDARG;

    *ppObj = NULL;

    elem = HeapAlloc(GetProcessHeap(), 0, sizeof (*elem));
    if(!elem)
        return E_OUTOFMEMORY;

    elem->lpVtbl = &xmlelem_vtbl;
    elem->ref = 1;
    elem->node = node;

    *ppObj = &elem->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

/************************************************************************
 * IXMLElementCollection
 */
typedef struct _xmlelem_collection
{
    const IXMLElementCollectionVtbl *lpVtbl;
    const IEnumVARIANTVtbl          *lpvtblIEnumVARIANT;
    LONG ref;
    LONG length;
    xmlNodePtr node;

    /* IEnumVARIANT members */
    xmlNodePtr current;
} xmlelem_collection;

static inline xmlelem_collection *impl_from_IXMLElementCollection(IXMLElementCollection *iface)
{
    return (xmlelem_collection *)((char*)iface - FIELD_OFFSET(xmlelem_collection, lpVtbl));
}

static inline xmlelem_collection *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return (xmlelem_collection *)((char*)iface - FIELD_OFFSET(xmlelem_collection, lpvtblIEnumVARIANT));
}

static HRESULT WINAPI xmlelem_collection_QueryInterface(IXMLElementCollection *iface, REFIID riid, void** ppvObject)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXMLElementCollection))
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        *ppvObject = &(This->lpvtblIEnumVARIANT);
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLElementCollection_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlelem_collection_AddRef(IXMLElementCollection *iface)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlelem_collection_Release(IXMLElementCollection *iface)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI xmlelem_collection_GetTypeInfoCount(IXMLElementCollection *iface, UINT* pctinfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_GetTypeInfo(IXMLElementCollection *iface, UINT iTInfo,
                                                     LCID lcid, ITypeInfo** ppTInfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_GetIDsOfNames(IXMLElementCollection *iface, REFIID riid,
                                                       LPOLESTR* rgszNames, UINT cNames,
                                                       LCID lcid, DISPID* rgDispId)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_Invoke(IXMLElementCollection *iface, DISPID dispIdMember,
                                                REFIID riid, LCID lcid, WORD wFlags,
                                                DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                                EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_put_length(IXMLElementCollection *iface, long v)
{
    TRACE("(%p, %ld)\n", iface, v);
    return E_FAIL;
}

static HRESULT WINAPI xmlelem_collection_get_length(IXMLElementCollection *iface, long *p)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = This->length;
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_get__newEnum(IXMLElementCollection *iface, IUnknown **ppUnk)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);

    TRACE("(%p, %p)\n", iface, ppUnk);

    if (!ppUnk)
        return E_INVALIDARG;

    *ppUnk = (IUnknown *)This;
    IUnknown_AddRef(*ppUnk);
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_item(IXMLElementCollection *iface, VARIANT var1,
                                              VARIANT var2, IDispatch **ppDisp)
{
    xmlelem_collection *This = impl_from_IXMLElementCollection(iface);
    xmlNodePtr ptr = This->node;
    int index, i;

    TRACE("(%p, %p)\n", iface, ppDisp);

    if (!ppDisp)
        return E_INVALIDARG;

    *ppDisp = NULL;

    index = V_I4(&var1);
    if (index < 0)
        return E_INVALIDARG;
    if (index >= This->length)
        return E_FAIL;

    for (i = 0; i < index; i++)
        ptr = ptr->next;

    return XMLElement_create((IUnknown *)iface, ptr, (LPVOID *)ppDisp);
}

static const struct IXMLElementCollectionVtbl xmlelem_collection_vtbl =
{
    xmlelem_collection_QueryInterface,
    xmlelem_collection_AddRef,
    xmlelem_collection_Release,
    xmlelem_collection_GetTypeInfoCount,
    xmlelem_collection_GetTypeInfo,
    xmlelem_collection_GetIDsOfNames,
    xmlelem_collection_Invoke,
    xmlelem_collection_put_length,
    xmlelem_collection_get_length,
    xmlelem_collection_get__newEnum,
    xmlelem_collection_item
};

/************************************************************************
 * xmlelem_collection implementation of IEnumVARIANT.
 */
static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_QueryInterface(
    IEnumVARIANT *iface, REFIID riid, LPVOID *ppvObj)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);
    return IXMLDocument_QueryInterface((IXMLDocument *)this, riid, ppvObj);
}

static ULONG WINAPI xmlelem_collection_IEnumVARIANT_AddRef(
    IEnumVARIANT *iface)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);
    return IXMLDocument_AddRef((IXMLDocument *)this);
}

static ULONG WINAPI xmlelem_collection_IEnumVARIANT_Release(
    IEnumVARIANT *iface)
{
    xmlelem_collection *this = impl_from_IEnumVARIANT(iface);
    return IXMLDocument_Release((IXMLDocument *)this);
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Next(
    IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    xmlelem_collection *This = impl_from_IEnumVARIANT(iface);
    xmlNodePtr ptr = This->current;

    TRACE("(%p, %d, %p, %p)\n", iface, celt, rgVar, pCeltFetched);

    if (!rgVar)
        return E_INVALIDARG;

    /* FIXME: handle celt */
    if (pCeltFetched)
        *pCeltFetched = 1;

    This->current = This->current->next;

    V_VT(rgVar) = VT_DISPATCH;
    return XMLElement_create((IUnknown *)iface, ptr, (LPVOID *)&V_DISPATCH(rgVar));
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Skip(
    IEnumVARIANT *iface, ULONG celt)
{
    FIXME("(%p, %d): stub\n", iface, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Reset(
    IEnumVARIANT *iface)
{
    xmlelem_collection *This = impl_from_IEnumVARIANT(iface);
    This->current = This->node;
    return S_OK;
}

static HRESULT WINAPI xmlelem_collection_IEnumVARIANT_Clone(
    IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    FIXME("(%p, %p): stub\n", iface, ppEnum);
    return E_NOTIMPL;
}

static const struct IEnumVARIANTVtbl xmlelem_collection_IEnumVARIANTvtbl =
{
    xmlelem_collection_IEnumVARIANT_QueryInterface,
    xmlelem_collection_IEnumVARIANT_AddRef,
    xmlelem_collection_IEnumVARIANT_Release,
    xmlelem_collection_IEnumVARIANT_Next,
    xmlelem_collection_IEnumVARIANT_Skip,
    xmlelem_collection_IEnumVARIANT_Reset,
    xmlelem_collection_IEnumVARIANT_Clone
};

static HRESULT XMLElementCollection_create(IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj)
{
    xmlelem_collection *collection;
    xmlNodePtr ptr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    *ppObj = NULL;

    if (!node)
        return S_FALSE;

    collection = HeapAlloc(GetProcessHeap(), 0, sizeof (*collection));
    if(!collection)
        return E_OUTOFMEMORY;

    collection->lpVtbl = &xmlelem_collection_vtbl;
    collection->lpvtblIEnumVARIANT = &xmlelem_collection_IEnumVARIANTvtbl;
    collection->ref = 1;
    collection->length = 0;
    collection->node = node;
    collection->current = node;

    ptr = node;
    while (ptr)
    {
        collection->length++;
        ptr = ptr->next;
    }

    *ppObj = &collection->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

#endif
