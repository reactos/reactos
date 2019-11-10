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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml6did.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static CRITICAL_SECTION cs_dispex_static_data;
static CRITICAL_SECTION_DEBUG cs_dispex_static_data_dbg =
{
    0, 0, &cs_dispex_static_data,
    { &cs_dispex_static_data_dbg.ProcessLocksList, &cs_dispex_static_data_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dispex_static_data") }
};
static CRITICAL_SECTION cs_dispex_static_data = { &cs_dispex_static_data_dbg, -1, 0, 0, 0, 0 };


enum lib_version_t
{
    LibXml = 0,
    LibXml2,
    LibXml_Last
};

typedef struct {
    REFIID iid;
    enum lib_version_t lib;
} tid_id_t;

typedef struct {
    REFIID iid;
    unsigned short major;
} lib_id_t;

typedef struct {
    DISPID id;
    BSTR name;
    enum tid_t tid;
} func_info_t;

struct dispex_data_t {
    DWORD func_cnt;
    func_info_t *funcs;
    func_info_t **name_table;

    struct list entry;
};

static struct list dispex_data_list = LIST_INIT(dispex_data_list);
static ITypeLib *typelib[LibXml_Last];
static ITypeInfo *typeinfos[LAST_tid];

/* indexed with lib_version_t values */
static lib_id_t lib_ids[] = {
    { &LIBID_MSXML,  2 },
    { &LIBID_MSXML2, 3 }
};

static tid_id_t tid_ids[] = {
    { &IID_NULL, LibXml_Last },
    { &IID_IXMLDOMAttribute, LibXml2 },
    { &IID_IXMLDOMCDATASection, LibXml2 },
    { &IID_IXMLDOMComment, LibXml2 },
    { &IID_IXMLDOMDocument, LibXml2 },
    { &IID_IXMLDOMDocument2, LibXml2 },
    { &IID_IXMLDOMDocument3, LibXml2 },
    { &IID_IXMLDOMDocumentFragment, LibXml2 },
    { &IID_IXMLDOMDocumentType, LibXml2 },
    { &IID_IXMLDOMElement, LibXml2 },
    { &IID_IXMLDOMEntityReference, LibXml2 },
    { &IID_IXMLDOMImplementation, LibXml2 },
    { &IID_IXMLDOMNamedNodeMap, LibXml2 },
    { &IID_IXMLDOMNode, LibXml2 },
    { &IID_IXMLDOMNodeList, LibXml2 },
    { &IID_IXMLDOMParseError2, LibXml2 },
    { &IID_IXMLDOMProcessingInstruction, LibXml2 },
    { &IID_IXMLDOMSchemaCollection, LibXml2 },
    { &IID_IXMLDOMSchemaCollection2, LibXml2 },
    { &IID_IXMLDOMSelection, LibXml2 },
    { &IID_IXMLDOMText, LibXml2 },
    { &IID_IXMLElement, LibXml },
    { &IID_IXMLDocument, LibXml },
    { &IID_IXMLHTTPRequest, LibXml2 },
    { &IID_IXSLProcessor, LibXml2 },
    { &IID_IXSLTemplate, LibXml2 },
    { &IID_IVBSAXAttributes, LibXml2 },
    { &IID_IVBSAXContentHandler, LibXml2 },
    { &IID_IVBSAXDeclHandler, LibXml2 },
    { &IID_IVBSAXDTDHandler, LibXml2 },
    { &IID_IVBSAXEntityResolver, LibXml2 },
    { &IID_IVBSAXErrorHandler, LibXml2 },
    { &IID_IVBSAXLexicalHandler, LibXml2 },
    { &IID_IVBSAXLocator, LibXml2 },
    { &IID_IVBSAXXMLFilter, LibXml2 },
    { &IID_IVBSAXXMLReader, LibXml2 },
    { &IID_IMXAttributes, LibXml2 },
    { &IID_IMXReaderControl, LibXml2 },
    { &IID_IMXWriter, LibXml2 },
    { &IID_IVBMXNamespaceManager, LibXml2 },
    { &IID_IServerXMLHTTPRequest, LibXml2 }
};

const IID *get_riid_from_tid(tid_t tid)
{
    return tid_ids[tid].iid;
}

static inline unsigned get_libid_from_tid(tid_t tid)
{
    return tid_ids[tid].lib;
}

static HRESULT get_typelib(unsigned lib, ITypeLib **tl)
{
    HRESULT hres;

    if(!typelib[lib]) {
        hres = LoadRegTypeLib(lib_ids[lib].iid, lib_ids[lib].major, 0, LOCALE_SYSTEM_DEFAULT, tl);
        if(FAILED(hres)) {
            ERR("LoadRegTypeLib failed: %08x\n", hres);
            return hres;
        }

        if (InterlockedCompareExchangePointer((void**)&typelib[lib], *tl, NULL))
            ITypeLib_Release(*tl);
    }

    *tl = typelib[lib];
    return S_OK;
}

HRESULT get_typeinfo(enum tid_t tid, ITypeInfo **typeinfo)
{
    unsigned lib = get_libid_from_tid(tid);
    ITypeLib *typelib;
    HRESULT hres;

    if (FAILED(hres = get_typelib(lib, &typelib)))
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, get_riid_from_tid(tid), &ti);
        if(FAILED(hres)) {
            /* try harder with typelib from msxml.dll */
            if (FAILED(hres = get_typelib(LibXml, &typelib)))
                return hres;
            hres = ITypeLib_GetTypeInfoOfGuid(typelib, get_riid_from_tid(tid), &ti);
            if(FAILED(hres)) {
                ERR("GetTypeInfoOfGuid failed: %08x\n", hres);
                return hres;
            }
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];

    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

void release_typelib(void)
{
    dispex_data_t *iter;
    unsigned i;

    while(!list_empty(&dispex_data_list)) {
        iter = LIST_ENTRY(list_head(&dispex_data_list), dispex_data_t, entry);
        list_remove(&iter->entry);

        for(i=0; i < iter->func_cnt; i++)
            SysFreeString(iter->funcs[i].name);

        heap_free(iter->funcs);
        heap_free(iter->name_table);
        heap_free(iter);
    }

    for(i=0; i < ARRAY_SIZE(typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    for(i=0; i < ARRAY_SIZE(typelib); i++)
        if(typelib[i])
            ITypeLib_Release(typelib[i]);

    DeleteCriticalSection(&cs_dispex_static_data);
}

static void add_func_info(dispex_data_t *data, DWORD *size, tid_t tid, DISPID id, ITypeInfo *dti)
{
    HRESULT hres;

    if(data->func_cnt && data->funcs[data->func_cnt-1].id == id)
        return;

    if(data->func_cnt == *size)
        data->funcs = heap_realloc(data->funcs, (*size <<= 1)*sizeof(func_info_t));

    hres = ITypeInfo_GetDocumentation(dti, id, &data->funcs[data->func_cnt].name, NULL, NULL, NULL);
    if(FAILED(hres))
        return;

    data->funcs[data->func_cnt].id = id;
    data->funcs[data->func_cnt].tid = tid;

    data->func_cnt++;
}

static int dispid_cmp(const void *p1, const void *p2)
{
    return ((const func_info_t*)p1)->id - ((const func_info_t*)p2)->id;
}

static int func_name_cmp(const void *p1, const void *p2)
{
    return strcmpiW((*(func_info_t* const*)p1)->name, (*(func_info_t* const*)p2)->name);
}

static dispex_data_t *preprocess_dispex_data(DispatchEx *This)
{
    const tid_t *tid = This->data->iface_tids;
    FUNCDESC *funcdesc;
    dispex_data_t *data;
    DWORD size = 16, i;
    ITypeInfo *ti, *dti;
    HRESULT hres;

    TRACE("(%p)\n", This);

    hres = get_typeinfo(This->data->disp_tid, &dti);
    if(FAILED(hres)) {
        ERR("Could not get disp type info: %08x\n", hres);
        return NULL;
    }

    data = heap_alloc(sizeof(dispex_data_t));
    data->func_cnt = 0;
    data->funcs = heap_alloc(size*sizeof(func_info_t));
    list_add_tail(&dispex_data_list, &data->entry);

    while(*tid) {
        hres = get_typeinfo(*tid, &ti);
        if(FAILED(hres))
            break;

        i=0;
        while(1) {
            hres = ITypeInfo_GetFuncDesc(ti, i++, &funcdesc);
            if(FAILED(hres))
                break;

            add_func_info(data, &size, *tid, funcdesc->memid, dti);
            ITypeInfo_ReleaseFuncDesc(ti, funcdesc);
        }

        ITypeInfo_Release(ti);
        tid++;
    }

    if(!data->func_cnt) {
        heap_free(data->funcs);
        data->funcs = NULL;
    }else if(data->func_cnt != size) {
        data->funcs = heap_realloc(data->funcs, data->func_cnt * sizeof(func_info_t));
    }

    qsort(data->funcs, data->func_cnt, sizeof(func_info_t), dispid_cmp);

    if(data->funcs) {
        data->name_table = heap_alloc(data->func_cnt * sizeof(func_info_t*));
        for(i=0; i < data->func_cnt; i++)
            data->name_table[i] = data->funcs+i;
        qsort(data->name_table, data->func_cnt, sizeof(func_info_t*), func_name_cmp);
    }else {
        data->name_table = NULL;
    }

    ITypeInfo_Release(dti);
    return data;
}

static dispex_data_t *get_dispex_data(DispatchEx *This)
{
    if(This->data->data)
        return This->data->data;

    EnterCriticalSection(&cs_dispex_static_data);

    if(!This->data->data)
        This->data->data = preprocess_dispex_data(This);

    LeaveCriticalSection(&cs_dispex_static_data);

    return This->data->data;
}

static inline DispatchEx *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, DispatchEx, IDispatchEx_iface);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(This->data->disp_tid, ppTInfo);
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x %s %x %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags,
                                pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    dispex_data_t *data;
    int min, max, n, c;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameEnsure|fdexNameImplicit))
        FIXME("Unsupported grfdex %x\n", grfdex);

    data = get_dispex_data(This);
    if(!data)
        return E_FAIL;

    min = 0;
    max = data->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        c = strcmpiW(data->name_table[n]->name, bstrName);
        if(!c) {
            if((grfdex & fdexNameCaseSensitive) && strcmpW(data->name_table[n]->name, bstrName))
                break;

            *pid = data->name_table[n]->id;
            return S_OK;
        }

        if(c > 0)
            max = n-1;
        else
            min = n+1;
    }

    if(This->data->vtbl && This->data->vtbl->get_dispid) {
        HRESULT hres;

        hres = This->data->vtbl->get_dispid(This->outer, bstrName, grfdex, pid);
        if(hres != DISP_E_UNKNOWNNAME)
            return hres;
    }

    TRACE("not found %s\n", debugstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static BOOL is_propputref_id(DISPID id)
{
    switch (id)
    {
    case DISPID_DOM_DOCUMENT_DOCUMENTELEMENT:
    case DISPID_XMLDOM_DOCUMENT2_SCHEMAS:
    case DISPID_XMLDOM_SELECTION_CONTEXT:
    case DISPID_XMLDOM_TEMPLATE_STYLESHEET:
    case DISPID_SAX_CONTENTHANDLER_DOCUMENTLOCATOR:
    case DISPID_SAX_XMLFILTER_PARENT:
    case DISPID_SAX_XMLREADER_ENTITYRESOLVER:
    case DISPID_SAX_XMLREADER_CONTENTHANDLER:
    case DISPID_SAX_XMLREADER_DTDHANDLER:
    case DISPID_SAX_XMLREADER_ERRORHANDLER:
    case DISPID_MXXML_FILTER_ENTITYRESOLVER:
    case DISPID_MXXML_FILTER_CONTENTHANDLER:
    case DISPID_MXXML_FILTER_DTDHANDLER:
    case DISPID_MXXML_FILTER_ERRORHANDLER:
        return TRUE;
    default:
        return FALSE;
    }
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    IUnknown *unk;
    ITypeInfo *ti;
    dispex_data_t *data;
    UINT argerr=0;
    int min, max, n;
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(This->data->vtbl && This->data->vtbl->invoke) {
        hres = This->data->vtbl->invoke(This->outer, id, lcid, wFlags, pdp, pvarRes, pei);
        if (hres != DISP_E_UNKNOWNNAME) return hres;
    }

    if(wFlags == DISPATCH_CONSTRUCT) {
        FIXME("DISPATCH_CONSTRUCT not implemented\n");
        return E_NOTIMPL;
    }

    data = get_dispex_data(This);
    if(!data)
        return E_FAIL;

    min = 0;
    max = data->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        if(data->funcs[n].id == id)
            break;

        if(data->funcs[n].id < id)
            min = n+1;
        else
            max = n-1;
    }

    if(min > max) {
        WARN("invalid id %x\n", id);
        return DISP_E_UNKNOWNNAME;
    }

    hres = get_typeinfo(data->funcs[n].tid, &ti);
    if(FAILED(hres)) {
        ERR("Could not get type info: %08x\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->outer, get_riid_from_tid(data->funcs[n].tid), (void**)&unk);
    if(FAILED(hres)) {
        ERR("Could not get iface: %08x\n", hres);
        ITypeInfo_Release(ti);
        return E_FAIL;
    }

    if (is_propputref_id(id) && wFlags == DISPATCH_PROPERTYPUT)
        wFlags = DISPATCH_PROPERTYPUTREF;
    hres = ITypeInfo_Invoke(ti, unk, id, wFlags, pdp, pvarRes, pei, &argerr);

    ITypeInfo_Release(ti);
    IUnknown_Release(unk);
    return hres;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE("Not implemented in native msxml3 (%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE("Not implemented in native msxml3 (%p)->(%x)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE("Not implemented in native msxml3 (%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE("Not implemented in native msxml3 (%p)->(%x %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE(" Not implemented in native msxml3 (%p)->(%x %x %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    TRACE("Not implemented in native msxml3 (%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

static IDispatchExVtbl DispatchExVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

BOOL dispex_query_interface(DispatchEx *This, REFIID riid, void **ppv)
{
    static const IID IID_UndocumentedScriptIface =
        {0x719c3050,0xf9d3,0x11cf,{0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa0}};
    static const IID IID_IDispatchJS =
        {0x719c3050,0xf9d3,0x11cf,{0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa6}};

    if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchJS, riid)) {
        TRACE("(%p)->(IID_IDispatchJS %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else if(IsEqualGUID(&IID_UndocumentedScriptIface, riid)) {
        TRACE("(%p)->(IID_UndocumentedScriptIface %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else if (IsEqualGUID(&IID_IObjectIdentity, riid)) {
        TRACE("IID_IObjectIdentity not supported returning NULL\n");
        *ppv = NULL;
    }else {
        return FALSE;
    }

    if(*ppv)
        IUnknown_AddRef((IUnknown*)*ppv);
    return TRUE;
}

void init_dispex(DispatchEx *dispex, IUnknown *outer, dispex_static_data_t *data)
{
    dispex->IDispatchEx_iface.lpVtbl = &DispatchExVtbl;
    dispex->outer = outer;
    dispex->data = data;
}
