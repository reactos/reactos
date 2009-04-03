/*
 * XML Document implementation
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
#include "wininet.h"
#include "winreg.h"
#include "shlwapi.h"
#include "ocidl.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

/* FIXME: IXMLDocument needs to implement
 *   - IXMLError
 *   - IPersistMoniker
 */

typedef struct _xmldoc
{
    const IXMLDocumentVtbl       *lpVtbl;
    const IPersistStreamInitVtbl *lpvtblIPersistStreamInit;
    LONG ref;
    HRESULT error;

    /* IXMLDocument */
    IXMLElement *root;
    xmlDocPtr xmldoc;

    /* IPersistStream */
    IStream *stream;
} xmldoc;

static inline xmldoc *impl_from_IXMLDocument(IXMLDocument *iface)
{
    return (xmldoc *)((char*)iface - FIELD_OFFSET(xmldoc, lpVtbl));
}

static inline xmldoc *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return (xmldoc *)((char*)iface - FIELD_OFFSET(xmldoc, lpvtblIPersistStreamInit));
}

static HRESULT WINAPI xmldoc_QueryInterface(IXMLDocument *iface, REFIID riid, void** ppvObject)
{
    xmldoc *This = impl_from_IXMLDocument(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXMLDocument) ||
        IsEqualGUID(riid, &IID_IXMLDOMDocument))
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(&IID_IPersistStreamInit, riid) ||
             IsEqualGUID(&IID_IPersistStream, riid))
    {
        *ppvObject = &(This->lpvtblIPersistStreamInit);
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDocument_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmldoc_AddRef(IXMLDocument *iface)
{
    xmldoc *This = impl_from_IXMLDocument(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmldoc_Release(IXMLDocument *iface)
{
    xmldoc *This = impl_from_IXMLDocument(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        xmlFreeDoc(This->xmldoc);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI xmldoc_GetTypeInfoCount(IXMLDocument *iface, UINT* pctinfo)
{
    xmldoc *This = impl_from_IXMLDocument(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI xmldoc_GetTypeInfo(IXMLDocument *iface, UINT iTInfo,
                                         LCID lcid, ITypeInfo** ppTInfo)
{
    xmldoc *This = impl_from_IXMLDocument(iface);
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDocument_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI xmldoc_GetIDsOfNames(IXMLDocument *iface, REFIID riid,
                                           LPOLESTR* rgszNames, UINT cNames,
                                           LCID lcid, DISPID* rgDispId)
{
    xmldoc *This = impl_from_IXMLDocument(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDocument_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmldoc_Invoke(IXMLDocument *iface, DISPID dispIdMember,
                                    REFIID riid, LCID lcid, WORD wFlags,
                                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                    EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    xmldoc *This = impl_from_IXMLDocument(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDocument_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmldoc_get_root(IXMLDocument *iface, IXMLElement **p)
{
    xmldoc *This = impl_from_IXMLDocument(iface);

    TRACE("(%p, %p)\n", iface, p);

    if (!p)
        return E_INVALIDARG;

    *p = This->root;
    if (!*p)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI xmldoc_get_fileSize(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_put_fileModifiedDate(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_fileUpdatedDate(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_URL(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

typedef struct {
    const struct IBindStatusCallbackVtbl *lpVtbl;
} bsc;

static HRESULT WINAPI bsc_QueryInterface(
    IBindStatusCallback *iface,
    REFIID riid,
    LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI bsc_AddRef(
    IBindStatusCallback *iface )
{
    return 2;
}

static ULONG WINAPI bsc_Release(
    IBindStatusCallback *iface )
{
    return 1;
}

static HRESULT WINAPI bsc_OnStartBinding(
        IBindStatusCallback* iface,
        DWORD dwReserved,
        IBinding* pib)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetPriority(
        IBindStatusCallback* iface,
        LONG* pnPriority)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnLowResource(
        IBindStatusCallback* iface,
        DWORD reserved)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnProgress(
        IBindStatusCallback* iface,
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnStopBinding(
        IBindStatusCallback* iface,
        HRESULT hresult,
        LPCWSTR szError)
{
    return S_OK;
}

static HRESULT WINAPI bsc_GetBindInfo(
        IBindStatusCallback* iface,
        DWORD* grfBINDF,
        BINDINFO* pbindinfo)
{
    *grfBINDF = BINDF_RESYNCHRONIZE;

    return S_OK;
}

static HRESULT WINAPI bsc_OnDataAvailable(
        IBindStatusCallback* iface,
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC* pformatetc,
        STGMEDIUM* pstgmed)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnObjectAvailable(
        IBindStatusCallback* iface,
        REFIID riid,
        IUnknown* punk)
{
    return S_OK;
}

static const struct IBindStatusCallbackVtbl bsc_vtbl =
{
    bsc_QueryInterface,
    bsc_AddRef,
    bsc_Release,
    bsc_OnStartBinding,
    bsc_GetPriority,
    bsc_OnLowResource,
    bsc_OnProgress,
    bsc_OnStopBinding,
    bsc_GetBindInfo,
    bsc_OnDataAvailable,
    bsc_OnObjectAvailable
};

static bsc xmldoc_bsc = { &bsc_vtbl };

static HRESULT WINAPI xmldoc_put_URL(IXMLDocument *iface, BSTR p)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    IStream *stream;
    IBindCtx *bctx;
    IMoniker *moniker;
    IPersistStreamInit *persist;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(p));

    if (!p)
        return E_INVALIDARG;

    if (!PathIsURLW(p))
    {
        WCHAR fullpath[MAX_PATH];
        DWORD needed = sizeof(url) / sizeof(WCHAR);

        if (!PathSearchAndQualifyW(p, fullpath, sizeof(fullpath) / sizeof(WCHAR)))
        {
            ERR("can't find path\n");
            return E_FAIL;
        }

        if (FAILED(UrlCreateFromPathW(fullpath, url, &needed, 0)))
        {
            ERR("can't create url from path\n");
            return E_FAIL;
        }

        p = url;
    }

    hr = CreateURLMoniker(NULL, p, &moniker);
    if (FAILED(hr))
        return hr;

    CreateAsyncBindCtx(0, (IBindStatusCallback *)&xmldoc_bsc, 0, &bctx);

    hr = IMoniker_BindToStorage(moniker, bctx, NULL, &IID_IStream, (LPVOID *)&stream);
    IBindCtx_Release(bctx);
    IMoniker_Release(moniker);
    if (FAILED(hr))
        return hr;

    hr = IXMLDocument_QueryInterface(iface, &IID_IPersistStreamInit, (LPVOID *)&persist);
    if (FAILED(hr))
    {
        IStream_Release(stream);
        return hr;
    }

    hr = IPersistStreamInit_Load(persist, stream);
    IPersistStreamInit_Release(persist);
    IStream_Release(stream);

    return hr;
}

static HRESULT WINAPI xmldoc_get_mimeType(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_readyState(IXMLDocument *iface, long *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_charset(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_put_charset(IXMLDocument *iface, BSTR p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_version(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_doctype(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_get_dtdURl(IXMLDocument *iface, BSTR *p)
{
    FIXME("(%p, %p): stub\n", iface, p);
    return E_NOTIMPL;
}

static xmlElementType type_msxml_to_libxml(long type)
{
    switch (type)
    {
        case XMLELEMTYPE_ELEMENT:
            return XML_ELEMENT_NODE;
        case XMLELEMTYPE_TEXT:
            return XML_TEXT_NODE;
        case XMLELEMTYPE_COMMENT:
            return XML_COMMENT_NODE;
        case XMLELEMTYPE_DOCUMENT:
            return XML_DOCUMENT_NODE;
        case XMLELEMTYPE_DTD:
            return XML_DTD_NODE;
        case XMLELEMTYPE_PI:
            return XML_PI_NODE;
        default:
            break;
    }

    return -1; /* FIXME: what is OTHER in msxml? */
}

static HRESULT WINAPI xmldoc_createElement(IXMLDocument *iface, VARIANT vType,
                                           VARIANT var1, IXMLElement **ppElem)
{
    xmlNodePtr node;
    static const xmlChar empty[] = "\0";

    TRACE("(%p, %p)\n", iface, ppElem);

    if (!ppElem)
        return E_INVALIDARG;

    *ppElem = NULL;

    if (V_VT(&vType) != VT_I4)
        return E_INVALIDARG;

    if(type_msxml_to_libxml(V_I4(&vType)) == -1)
        return E_NOTIMPL;

    node = xmlNewNode(NULL, empty);
    node->type = type_msxml_to_libxml(V_I4(&vType));

    /* FIXME: create xmlNodePtr based on vType and var1 */
    return XMLElement_create((IUnknown *)iface, node, (LPVOID *)ppElem);
}

static const struct IXMLDocumentVtbl xmldoc_vtbl =
{
    xmldoc_QueryInterface,
    xmldoc_AddRef,
    xmldoc_Release,
    xmldoc_GetTypeInfoCount,
    xmldoc_GetTypeInfo,
    xmldoc_GetIDsOfNames,
    xmldoc_Invoke,
    xmldoc_get_root,
    xmldoc_get_fileSize,
    xmldoc_put_fileModifiedDate,
    xmldoc_get_fileUpdatedDate,
    xmldoc_get_URL,
    xmldoc_put_URL,
    xmldoc_get_mimeType,
    xmldoc_get_readyState,
    xmldoc_get_charset,
    xmldoc_put_charset,
    xmldoc_get_version,
    xmldoc_get_doctype,
    xmldoc_get_dtdURl,
    xmldoc_createElement
};

/************************************************************************
 * xmldoc implementation of IPersistStreamInit.
 */
static HRESULT WINAPI xmldoc_IPersistStreamInit_QueryInterface(
    IPersistStreamInit *iface, REFIID riid, LPVOID *ppvObj)
{
    xmldoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDocument_QueryInterface((IXMLDocument *)this, riid, ppvObj);
}

static ULONG WINAPI xmldoc_IPersistStreamInit_AddRef(
    IPersistStreamInit *iface)
{
    xmldoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDocument_AddRef((IXMLDocument *)this);
}

static ULONG WINAPI xmldoc_IPersistStreamInit_Release(
    IPersistStreamInit *iface)
{
    xmldoc *this = impl_from_IPersistStreamInit(iface);
    return IXMLDocument_Release((IXMLDocument *)this);
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_GetClassID(
    IPersistStreamInit *iface, CLSID *classid)
{
    FIXME("(%p,%p): stub!\n", iface, classid);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_IsDirty(
    IPersistStreamInit *iface)
{
    FIXME("(%p): stub!\n", iface);
    return E_NOTIMPL;
}

xmlDocPtr parse_xml(char *ptr, int len)
{
#ifdef HAVE_XMLREADMEMORY
    return xmlReadMemory(ptr, len, NULL, NULL,
                         XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS);
#else
    return xmlParseMemory(ptr, len);
#endif
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_Load(
    IPersistStreamInit *iface, LPSTREAM pStm)
{
    xmldoc *This = impl_from_IPersistStreamInit(iface);
    xmlNodePtr xmlnode;
    HRESULT hr;
    HGLOBAL hglobal;
    DWORD read, written, len;
    BYTE buf[4096];
    char *ptr;

    TRACE("(%p, %p)\n", iface, pStm);

    if (!pStm)
        return E_INVALIDARG;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
    if (FAILED(hr))
        return hr;

    do
    {
        IStream_Read(pStm, buf, sizeof(buf), &read);
        hr = IStream_Write(This->stream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    if (FAILED(hr))
    {
        ERR("Failed to copy stream\n");
        return hr;
    }

    hr = GetHGlobalFromStream(This->stream, &hglobal);
    if (FAILED(hr))
        return hr;

    len = GlobalSize(hglobal);
    ptr = GlobalLock(hglobal);
    if (len != 0)
        This->xmldoc = parse_xml(ptr, len);
    GlobalUnlock(hglobal);

    if (!This->xmldoc)
    {
        ERR("Failed to parse xml\n");
        return E_FAIL;
    }

    xmlnode = xmlDocGetRootElement(This->xmldoc);
    return XMLElement_create((IUnknown *)This, xmlnode, (LPVOID *)&This->root);
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_Save(
    IPersistStreamInit *iface, LPSTREAM pStm, BOOL fClearDirty)
{
    FIXME("(%p, %p, %d): stub!\n", iface, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_GetSizeMax(
    IPersistStreamInit *iface, ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p, %p): stub!\n", iface, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmldoc_IPersistStreamInit_InitNew(
    IPersistStreamInit *iface)
{
    FIXME("(%p): stub!\n", iface);
    return E_NOTIMPL;
}

static const IPersistStreamInitVtbl xmldoc_IPersistStreamInit_VTable =
{
  xmldoc_IPersistStreamInit_QueryInterface,
  xmldoc_IPersistStreamInit_AddRef,
  xmldoc_IPersistStreamInit_Release,
  xmldoc_IPersistStreamInit_GetClassID,
  xmldoc_IPersistStreamInit_IsDirty,
  xmldoc_IPersistStreamInit_Load,
  xmldoc_IPersistStreamInit_Save,
  xmldoc_IPersistStreamInit_GetSizeMax,
  xmldoc_IPersistStreamInit_InitNew
};

HRESULT XMLDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    xmldoc *doc;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    doc = HeapAlloc(GetProcessHeap(), 0, sizeof (*doc));
    if(!doc)
        return E_OUTOFMEMORY;

    doc->lpVtbl = &xmldoc_vtbl;
    doc->lpvtblIPersistStreamInit = &xmldoc_IPersistStreamInit_VTable;
    doc->ref = 1;
    doc->error = S_OK;
    doc->root = NULL;
    doc->xmldoc = NULL;
    doc->stream = NULL;

    *ppObj = &doc->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

#else

HRESULT XMLDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use an XMLDocument object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
