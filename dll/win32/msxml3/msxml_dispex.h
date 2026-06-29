/*
 * Copyright 2005 Mike McCormack
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

#ifndef __MSXML_DISPEX__
#define __MSXML_DISPEX__

#include "dispex.h"

#include "wine/list.h"

typedef enum
{
    MSXML_DEFAULT = 0,
    MSXML2        = 20,
    MSXML26       = 26,
    MSXML3        = 30,
    MSXML4        = 40,
    MSXML6        = 60
} MSXML_VERSION;

typedef enum tid_t
{
    NULL_tid,
    IXMLDOMAttribute_tid,
    IXMLDOMCDATASection_tid,
    IXMLDOMComment_tid,
    IXMLDOMDocument_tid,
    IXMLDOMDocument2_tid,
    IXMLDOMDocument3_tid,
    IXMLDOMDocumentFragment_tid,
    IXMLDOMDocumentType_tid,
    IXMLDOMElement_tid,
    IXMLDOMEntityReference_tid,
    IXMLDOMImplementation_tid,
    IXMLDOMNamedNodeMap_tid,
    IXMLDOMNode_tid,
    IXMLDOMNodeList_tid,
    IXMLDOMParseError2_tid,
    IXMLDOMProcessingInstruction_tid,
    IXMLDOMSchemaCollection_tid,
    IXMLDOMSchemaCollection2_tid,
    IXMLDOMSelection_tid,
    IXMLDOMText_tid,
    IXMLElement_tid,
    IXMLDocument_tid,
    IXMLHTTPRequest_tid,
    IXSLProcessor_tid,
    IXSLTemplate_tid,
    IVBSAXAttributes_tid,
    IVBSAXContentHandler_tid,
    IVBSAXDeclHandler_tid,
    IVBSAXDTDHandler_tid,
    IVBSAXEntityResolver_tid,
    IVBSAXErrorHandler_tid,
    IVBSAXLexicalHandler_tid,
    IVBSAXLocator_tid,
    IVBSAXXMLFilter_tid,
    IVBSAXXMLReader_tid,
    IMXAttributes_tid,
    IMXReaderControl_tid,
    IMXWriter_tid,
    IVBMXNamespaceManager_tid,
    IServerXMLHTTPRequest_tid,
    LAST_tid
} tid_t;

extern HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo);
extern void release_typelib(void);

typedef struct dispex_data_t dispex_data_t;

typedef struct
{
    HRESULT (*get_dispid)(IUnknown*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(IUnknown*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*);
} dispex_static_data_vtbl_t;

typedef struct
{
    const dispex_static_data_vtbl_t *vtbl;
    const tid_t disp_tid;
    dispex_data_t *data;
    const tid_t* const iface_tids;
} dispex_static_data_t;

typedef struct
{
    IDispatchEx IDispatchEx_iface;

    IUnknown *outer;

    dispex_static_data_t *data;
} DispatchEx;

void init_dispex(DispatchEx*,IUnknown*,dispex_static_data_t*);
void release_dispex(DispatchEx*);
BOOL dispex_query_interface(DispatchEx*,REFIID,void**);
const IID *get_riid_from_tid(enum tid_t tid);

static inline HRESULT return_bstr(const WCHAR *value, BSTR *p)
{
    if (!p)
        return E_INVALIDARG;

    if (value)
    {
        *p = SysAllocString(value);
        if (!*p)
            return E_OUTOFMEMORY;
    }
    else
        *p = NULL;

    return S_OK;
}

static inline HRESULT return_bstrn(const WCHAR *value, int len, BSTR *p)
{
    if (value)
    {
        *p = SysAllocStringLen(value, len);
        if (!*p)
            return E_OUTOFMEMORY;
    }
    else
        *p = NULL;

    return S_OK;
}

extern HRESULT dom_document_create(MSXML_VERSION class_version, void **document);

typedef struct bsc_t bsc_t;

HRESULT create_moniker_from_url(LPCWSTR, IMoniker**);
HRESULT create_uri(IUri *base, const WCHAR *, IUri **);
HRESULT bind_url(IMoniker*, HRESULT (*onDataAvailable)(void*,char*,DWORD), void*, bsc_t**);
HRESULT detach_bsc(bsc_t*);
IUri *get_base_uri(IUnknown *site);

#endif /* __MSXML_DISPEX__ */
