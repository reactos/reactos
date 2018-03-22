/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#ifdef HAVE_LIBXML2
#include <libxml/parser.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msxml6.h"
#ifdef __REACTOS__
#include <wingdi.h>
#endif
#include "mshtml.h"
#include "mshtmhst.h"
#include "perhist.h"
#include "docobj.h"

#include "wine/debug.h"

#include "msxml_private.h"

#ifdef HAVE_LIBXML2

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    IPersistMoniker IPersistMoniker_iface;
    IPersistHistory IPersistHistory_iface;
    IOleCommandTarget IOleCommandTarget_iface;
    IOleObject IOleObject_iface;

    LONG ref;

    IUnknown *html_doc;
    IMoniker *mon;
} XMLView;

typedef struct
{
    IMoniker IMoniker_iface;
    LONG ref;
    IMoniker *mon;

    IStream *stream;
} Moniker;

typedef struct
{
    IBindStatusCallback IBindStatusCallback_iface;
    LONG ref;
    IBindStatusCallback *bsc;

    IMoniker *mon;
    IStream *stream;
} BindStatusCallback;

typedef struct
{
    IBinding IBinding_iface;
    LONG ref;
    IBinding *binding;
} Binding;

static inline Binding* impl_from_IBinding(IBinding *iface)
{
    return CONTAINING_RECORD(iface, Binding, IBinding_iface);
}

static HRESULT WINAPI XMLView_Binding_QueryInterface(
        IBinding *iface, REFIID riid, void **ppvObject)
{
    Binding *This = impl_from_IBinding(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IBinding)) {
        *ppvObject = iface;
        IBinding_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI XMLView_Binding_AddRef(IBinding *iface)
{
    Binding *This = impl_from_IBinding(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI XMLView_Binding_Release(IBinding *iface)
{
    Binding *This = impl_from_IBinding(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        IBinding_Release(This->binding);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI XMLView_Binding_Abort(IBinding *iface)
{
    Binding *This = impl_from_IBinding(iface);
    TRACE("(%p)\n", This);

    return IBinding_Abort(This->binding);
}

static HRESULT WINAPI XMLView_Binding_Suspend(IBinding *iface)
{
    Binding *This = impl_from_IBinding(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Binding_Resume(IBinding *iface)
{
    Binding *This = impl_from_IBinding(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Binding_SetPriority(
        IBinding *iface, LONG nPriority)
{
    Binding *This = impl_from_IBinding(iface);
    TRACE("(%p)->(%d)\n", This, nPriority);

    return IBinding_SetPriority(This->binding, nPriority);
}

static HRESULT WINAPI XMLView_Binding_GetPriority(
        IBinding *iface, LONG *pnPriority)
{
    Binding *This = impl_from_IBinding(iface);
    TRACE("(%p)->(%p)\n", This, pnPriority);

    return IBinding_GetPriority(This->binding, pnPriority);
}

static HRESULT WINAPI XMLView_Binding_GetBindResult(IBinding *iface,
        CLSID *pclsidProtocol, DWORD *pdwResult, LPOLESTR *pszResult,
        DWORD *pdwReserved)
{
    Binding *This = impl_from_IBinding(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_guid(pclsidProtocol),
            pdwResult, pszResult, pdwReserved);
    return E_NOTIMPL;
}

static IBindingVtbl XMLView_BindingVtbl = {
    XMLView_Binding_QueryInterface,
    XMLView_Binding_AddRef,
    XMLView_Binding_Release,
    XMLView_Binding_Abort,
    XMLView_Binding_Suspend,
    XMLView_Binding_Resume,
    XMLView_Binding_SetPriority,
    XMLView_Binding_GetPriority,
    XMLView_Binding_GetBindResult
};

static inline HRESULT XMLView_Binding_Create(IBinding *binding, IBinding **ret)
{
    Binding *bind;

    bind = heap_alloc_zero(sizeof(Binding));
    if(!bind)
        return E_OUTOFMEMORY;

    bind->IBinding_iface.lpVtbl = &XMLView_BindingVtbl;
    bind->ref = 1;

    bind->binding = binding;
    IBinding_AddRef(binding);

    *ret = &bind->IBinding_iface;
    return S_OK;
}

static inline BindStatusCallback* impl_from_IBindStatusCallback(
        IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, BindStatusCallback,
            IBindStatusCallback_iface);
}

static HRESULT WINAPI XMLView_BindStatusCallback_QueryInterface(
        IBindStatusCallback *iface, REFIID riid, void **ppvObject)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IBindStatusCallback)) {
        *ppvObject = iface;
        IBindStatusCallback_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI XMLView_BindStatusCallback_AddRef(
        IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI XMLView_BindStatusCallback_Release(
        IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        if(This->stream)
            IStream_Release(This->stream);
        IBindStatusCallback_Release(This->bsc);
        IMoniker_Release(This->mon);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnStartBinding(
        IBindStatusCallback *iface, DWORD dwReserved, IBinding *pib)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    IBinding *binding;
    HRESULT hres;

    TRACE("(%p)->(%x %p)\n", This, dwReserved, pib);

    hres = XMLView_Binding_Create(pib, &binding);
    if(FAILED(hres)) {
        IBinding_Abort(pib);
        return hres;
    }

    hres = IBindStatusCallback_OnStartBinding(This->bsc, dwReserved, binding);
    if(FAILED(hres)) {
        IBinding_Abort(binding);
        return hres;
    }

    IBinding_Release(binding);
    return hres;
}

static HRESULT WINAPI XMLView_BindStatusCallback_GetPriority(
        IBindStatusCallback *iface, LONG *pnPriority)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnLowResource(
        IBindStatusCallback *iface, DWORD reserved)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%x)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnProgress(
        IBindStatusCallback *iface, ULONG ulProgress, ULONG ulProgressMax,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%d %d %x %s)\n", This, ulProgress, ulProgressMax,
            ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        return IBindStatusCallback_OnProgress(This->bsc, ulProgress,
                ulProgressMax, ulStatusCode, szStatusText);
    case BINDSTATUS_MIMETYPEAVAILABLE:
        return S_OK;
    default:
        FIXME("ulStatusCode: %d\n", ulStatusCode);
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnStopBinding(
        IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%x %s)\n", This, hresult, debugstr_w(szError));
    return IBindStatusCallback_OnStopBinding(This->bsc, hresult, szError);
}

static HRESULT WINAPI XMLView_BindStatusCallback_GetBindInfo(
        IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);
    return IBindStatusCallback_GetBindInfo(This->bsc, grfBINDF, pbindinfo);
}

static inline HRESULT report_data(BindStatusCallback *This)
{
    FORMATETC formatetc = {0, NULL, 1, -1, TYMED_ISTREAM};
    STGMEDIUM stgmedium;
    LARGE_INTEGER off;
    ULARGE_INTEGER size;
    HRESULT hres;

    off.QuadPart = 0;
    hres = IStream_Seek(This->stream, off, STREAM_SEEK_CUR, &size);
    if(FAILED(hres))
        return hres;

    hres = IStream_Seek(This->stream, off, STREAM_SEEK_SET, NULL);
    if(FAILED(hres))
        return hres;

    stgmedium.tymed = TYMED_ISTREAM;
    stgmedium.u.pstm = This->stream;
    stgmedium.pUnkForRelease = NULL;

    hres = IBindStatusCallback_OnDataAvailable(This->bsc,
            BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
            size.u.LowPart, &formatetc, &stgmedium);

    IStream_Release(This->stream);
    This->stream = NULL;
    return hres;
}

static inline HRESULT display_error_page(BindStatusCallback *This)
{
    FIXME("Error page not implemented yet.\n");
    return report_data(This);
}

static inline HRESULT handle_xml_load(BindStatusCallback *This)
{
    static const WCHAR selectW[] = {'p','r','o','c','e','s','s','i','n','g','-',
        'i','n','s','t','r','u','c','t','i','o','n','(','\'','x','m','l',
        '-','s','t','y','l','e','s','h','e','e','t','\'',')',0};
    static const WCHAR hrefW[] = {'h','r','e','f','=',0};

    IXMLDOMDocument3 *xml = NULL, *xsl = NULL;
    IXMLDOMNode *stylesheet;
    IBindCtx *pbc;
    IMoniker *mon;
    LPOLESTR xsl_url;
    LARGE_INTEGER off;
    VARIANT_BOOL succ;
    VARIANT var;
    WCHAR *href = NULL, *p;
    BSTR bstr;
    HRESULT hres;

    off.QuadPart = 0;
    hres = IStream_Seek(This->stream, off, STREAM_SEEK_SET, NULL);
    if(FAILED(hres))
        return display_error_page(This);

    hres = DOMDocument_create(MSXML_DEFAULT, (void**)&xml);
    if(FAILED(hres))
        return display_error_page(This);

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)This->stream;
    hres = IXMLDOMDocument3_load(xml, var, &succ);
    if(FAILED(hres) || !succ) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }
    V_VT(&var) = VT_EMPTY;

    bstr = SysAllocString(selectW);
    hres = IXMLDOMDocument3_selectSingleNode(xml, bstr, &stylesheet);
    SysFreeString(bstr);
    if(hres != S_OK) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    hres = IXMLDOMNode_get_nodeValue(stylesheet, &var);
    IXMLDOMNode_Release(stylesheet);
    if(SUCCEEDED(hres) && V_VT(&var)!=VT_BSTR) {
        FIXME("Variant type %d not supported\n", V_VT(&var));
        VariantClear(&var);
        hres = E_FAIL;
    }
    if(FAILED(hres)) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    /* TODO: fix parsing processing instruction value */
    if((p = strstrW(V_BSTR(&var), hrefW))) {
        p += ARRAY_SIZE(hrefW) - 1;
        if(*p!='\'' && *p!='\"') p = NULL;
        else {
            href = p+1;
            p = strchrW(href, *p);
        }
    }
    if(p) {
        *p = 0;
    } else {
        VariantClear(&var);
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    hres = CreateURLMonikerEx(This->mon, href, &mon, 0);
    VariantClear(&var);
    if(FAILED(hres)) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    hres = CreateBindCtx(0, &pbc);
    if(SUCCEEDED(hres)) {
        hres = IMoniker_GetDisplayName(mon, pbc, NULL, &xsl_url);
        IMoniker_Release(mon);
        IBindCtx_Release(pbc);
    }
    if(FAILED(hres)) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(xsl_url);
    CoTaskMemFree(xsl_url);
    if(!V_BSTR(&var)) {
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    hres = DOMDocument_create(MSXML_DEFAULT, (void**)&xsl);
    if(FAILED(hres)) {
        VariantClear(&var);
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    /* TODO: do the binding asynchronously */
    hres = IXMLDOMDocument3_load(xsl, var, &succ);
    VariantClear(&var);
    if(FAILED(hres) || !succ) {
        IXMLDOMDocument3_Release(xsl);
        IXMLDOMDocument3_Release(xml);
        return display_error_page(This);
    }

    hres = IXMLDOMDocument3_transformNode(xml, (IXMLDOMNode*)xsl, &bstr);
    IXMLDOMDocument3_Release(xsl);
    IXMLDOMDocument3_Release(xml);
    if(FAILED(hres))
        return display_error_page(This);

    hres = IStream_Seek(This->stream, off, STREAM_SEEK_SET, NULL);
    if(FAILED(hres)) {
        SysFreeString(bstr);
        return display_error_page(This);
    }

    hres = IStream_Write(This->stream, (BYTE*)bstr,
            SysStringLen(bstr)*sizeof(WCHAR), NULL);
    SysFreeString(bstr);
    if(FAILED(hres))
        return display_error_page(This);

    return report_data(This);
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnDataAvailable(
        IBindStatusCallback *iface, DWORD grfBSCF, DWORD dwSize,
        FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    char buf[1024];
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    if(!This->stream)
        return E_FAIL;

    do {
        hres = IStream_Read(pstgmed->u.pstm, buf, sizeof(buf), &size);
        IStream_Write(This->stream, buf, size, &size);
    } while(hres==S_OK && size);

    if(FAILED(hres) && hres!=E_PENDING)
        return hres;
    if(hres != S_FALSE)
        return S_OK;

    return handle_xml_load(This);
}

static HRESULT WINAPI XMLView_BindStatusCallback_OnObjectAvailable(
        IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

static IBindStatusCallbackVtbl XMLView_BindStatusCallbackVtbl = {
    XMLView_BindStatusCallback_QueryInterface,
    XMLView_BindStatusCallback_AddRef,
    XMLView_BindStatusCallback_Release,
    XMLView_BindStatusCallback_OnStartBinding,
    XMLView_BindStatusCallback_GetPriority,
    XMLView_BindStatusCallback_OnLowResource,
    XMLView_BindStatusCallback_OnProgress,
    XMLView_BindStatusCallback_OnStopBinding,
    XMLView_BindStatusCallback_GetBindInfo,
    XMLView_BindStatusCallback_OnDataAvailable,
    XMLView_BindStatusCallback_OnObjectAvailable
};

static inline HRESULT XMLView_BindStatusCallback_Create(IBindStatusCallback *bsc_html,
        IMoniker *mon, IStream *stream, IBindStatusCallback **ret)
{
    BindStatusCallback *bsc;

    bsc = heap_alloc_zero(sizeof(BindStatusCallback));
    if(!bsc)
        return E_OUTOFMEMORY;

    bsc->IBindStatusCallback_iface.lpVtbl = &XMLView_BindStatusCallbackVtbl;
    bsc->ref = 1;

    bsc->bsc = bsc_html;
    IBindStatusCallback_AddRef(bsc_html);
    bsc->stream = stream;
    IStream_AddRef(bsc->stream);
    bsc->mon = mon;
    IMoniker_AddRef(mon);

    *ret = &bsc->IBindStatusCallback_iface;
    return S_OK;
}

static inline Moniker* impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, Moniker, IMoniker_iface);
}

static HRESULT WINAPI XMLView_Moniker_QueryInterface(
        IMoniker *iface, REFIID riid, void **ppvObject)
{
    Moniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IPersist)
            || IsEqualGUID(riid, &IID_IPersistStream) || IsEqualGUID(riid, &IID_IMoniker)) {
        *ppvObject = iface;
        IMoniker_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI XMLView_Moniker_AddRef(IMoniker *iface)
{
    Moniker *This = impl_from_IMoniker(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI XMLView_Moniker_Release(IMoniker *iface)
{
    Moniker *This = impl_from_IMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        IMoniker_Release(This->mon);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI XMLView_Moniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_IsDirty(IMoniker *iface)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Load(IMoniker *iface, IStream *pStm)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pStm);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Save(IMoniker *iface,
        IStream *pStm, BOOL fClearDirty)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %x)\n", This, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_GetSizeMax(
        IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_BindToObject(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, REFIID riidResult, void **ppvResult)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft,
            debugstr_guid(riidResult), ppvResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, REFIID riid, void **ppvObj)
{
    Moniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObj);

    if(IsEqualGUID(riid, &IID_IStream)) {
        if(!This->stream)
            return E_FAIL;

        *ppvObj = This->stream;
        This->stream = NULL;
        return S_ASYNCHRONOUS;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Reduce(IMoniker *iface, IBindCtx *pbc,
        DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %d %p %p)\n", This, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %x %p)\n", This, pmkRight, fOnlyIfNotGeneric, ppmkComposite);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Enum(IMoniker *iface,
        BOOL fForward, IEnumMoniker **ppenumMoniker)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%x %p)\n", This, fForward, ppenumMoniker);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_IsEqual(IMoniker *iface,
        IMoniker *pmkOtherMoniker)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pmkOtherMoniker);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pdwHash);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_IsRunning(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pbc, pmkToLeft, pmkNewlyRunning);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_GetTimeOfLastChange(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pbc, pmkToLeft, pFileTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_CommonPrefixWith(IMoniker *iface,
        IMoniker *pmkOther, IMoniker **ppmkPrefix)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pmkOther, ppmkPrefix);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_RelativePathTo(IMoniker *iface,
        IMoniker *pmkOther, IMoniker **ppmkRelPath)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pmkOther, ppmkRelPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_GetDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    Moniker *This = impl_from_IMoniker(iface);
    TRACE("(%p)->(%p %p %p)\n", This, pbc, pmkToLeft, ppszDisplayName);
    return IMoniker_GetDisplayName(This->mon, pbc, pmkToLeft, ppszDisplayName);
}

static HRESULT WINAPI XMLView_Moniker_ParseDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p %p %s %p %p)\n", This, pbc, pmkToLeft,
            debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_Moniker_IsSystemMoniker(
        IMoniker *iface, DWORD *pdwMksys)
{
    Moniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pdwMksys);
    return E_NOTIMPL;
}

static IMonikerVtbl XMLView_MonikerVtbl = {
    XMLView_Moniker_QueryInterface,
    XMLView_Moniker_AddRef,
    XMLView_Moniker_Release,
    XMLView_Moniker_GetClassID,
    XMLView_Moniker_IsDirty,
    XMLView_Moniker_Load,
    XMLView_Moniker_Save,
    XMLView_Moniker_GetSizeMax,
    XMLView_Moniker_BindToObject,
    XMLView_Moniker_BindToStorage,
    XMLView_Moniker_Reduce,
    XMLView_Moniker_ComposeWith,
    XMLView_Moniker_Enum,
    XMLView_Moniker_IsEqual,
    XMLView_Moniker_Hash,
    XMLView_Moniker_IsRunning,
    XMLView_Moniker_GetTimeOfLastChange,
    XMLView_Moniker_Inverse,
    XMLView_Moniker_CommonPrefixWith,
    XMLView_Moniker_RelativePathTo,
    XMLView_Moniker_GetDisplayName,
    XMLView_Moniker_ParseDisplayName,
    XMLView_Moniker_IsSystemMoniker
};

static inline HRESULT XMLView_Moniker_Create(IMoniker *mon,
        IStream *stream, IMoniker **ret)
{
    Moniker *wrap;

    wrap = heap_alloc_zero(sizeof(Moniker));
    if(!wrap)
        return E_OUTOFMEMORY;

    wrap->IMoniker_iface.lpVtbl = &XMLView_MonikerVtbl;
    wrap->ref = 1;

    wrap->mon = mon;
    IMoniker_AddRef(mon);
    wrap->stream = stream;
    IStream_AddRef(stream);

    *ret = &wrap->IMoniker_iface;
    return S_OK;
}

static inline XMLView* impl_from_IPersistMoniker(IPersistMoniker *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_PersistMoniker_QueryInterface(
        IPersistMoniker *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IPersistMoniker(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IPersistMoniker))
        *ppvObject = &This->IPersistMoniker_iface;
    else if(IsEqualGUID(riid, &IID_IPersistHistory) ||
            IsEqualGUID(riid, &IID_IPersist))
        *ppvObject = &This->IPersistHistory_iface;
    else if(IsEqualGUID(riid, &IID_IOleCommandTarget))
        *ppvObject = &This->IOleCommandTarget_iface;
    else if(IsEqualGUID(riid, &IID_IOleObject))
        *ppvObject = &This->IOleObject_iface;
    else
        return IUnknown_QueryInterface(This->html_doc, riid, ppvObject);

    IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
    return S_OK;
}

static ULONG WINAPI XMLView_PersistMoniker_AddRef(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI XMLView_PersistMoniker_Release(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        if(This->mon)
            IMoniker_Release(This->mon);
        IUnknown_Release(This->html_doc);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI XMLView_PersistMoniker_GetClassID(
        IPersistMoniker *iface, CLSID *pClassID)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_IsDirty(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_Load(IPersistMoniker *iface,
        BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    static const WCHAR XSLParametersW[] = {'X','S','L','P','a','r','a','m','e','t','e','r','s',0};
    static const WCHAR XMLBufferStreamW[] = {'X','M','L','B','u','f','f','e','r','S','t','r','e','a','m',0};
    static const WCHAR DWNBINDINFOW[] = {'_','_','D','W','N','B','I','N','D','I','N','F','O',0};
    static const WCHAR HTMLLOADOPTIONSW[] = {'_','_','H','T','M','L','L','O','A','D','O','P','T','I','O','N','S',0};
    static const WCHAR BSCBHolderW[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

    XMLView *This = impl_from_IPersistMoniker(iface);
    IPersistMoniker *html_persist_mon;
    IBindStatusCallback *bsc, *bsc_html;
    IBindCtx *bindctx;
    IStream *stream;
    IMoniker *mon;
    IUnknown *unk;
    HRESULT hres;

    TRACE("(%p)->(%x %p %p %x)\n", This, fFullyAvailable, pimkName, pibc, grfMode);

    hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)XSLParametersW, &unk);
    if(SUCCEEDED(hres)) {
        FIXME("ignoring XSLParameters\n");
        IUnknown_Release(unk);
    }
    hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)XMLBufferStreamW, &unk);
    if(SUCCEEDED(hres)) {
        FIXME("ignoring XMLBufferStream\n");
        IUnknown_Release(unk);
    }

    hres = CreateBindCtx(0, &bindctx);
    if(FAILED(hres))
        return hres;

    hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)DWNBINDINFOW, &unk);
    if(SUCCEEDED(hres)) {
        IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)DWNBINDINFOW, unk);
        IUnknown_Release(unk);
    }
    hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)HTMLLOADOPTIONSW, &unk);
    if(SUCCEEDED(hres)) {
        IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)HTMLLOADOPTIONSW, unk);
        IUnknown_Release(unk);
    }
    hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM, &unk);
    if(SUCCEEDED(hres)) {
        IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM, unk);
        IUnknown_Release(unk);
    }

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if(FAILED(hres)) {
        IBindCtx_Release(bindctx);
        return hres;
    }

    if(This->mon)
        IMoniker_Release(This->mon);
    This->mon = pimkName;
    IMoniker_AddRef(This->mon);

    hres = XMLView_Moniker_Create(This->mon, stream, &mon);
    if(FAILED(hres)) {
        IStream_Release(stream);
        IBindCtx_Release(bindctx);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->html_doc,
            &IID_IPersistMoniker, (void**)&html_persist_mon);
    if(FAILED(hres)) {
        IMoniker_Release(mon);
        IStream_Release(stream);
        IBindCtx_Release(bindctx);
        return hres;
    }

    hres = IPersistMoniker_Load(html_persist_mon, FALSE, mon, bindctx, 0);
    IPersistMoniker_Release(html_persist_mon);
    IMoniker_Release(mon);
    if(FAILED(hres)) {
        IStream_Release(stream);
        IBindCtx_Release(bindctx);
        return hres;
    }

    hres = IBindCtx_GetObjectParam(bindctx, (LPOLESTR)BSCBHolderW, &unk);
    IBindCtx_Release(bindctx);
    if(FAILED(hres)) {
        IStream_Release(stream);
        return hres;
    }
    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&bsc_html);
    IUnknown_Release(unk);
    if(FAILED(hres)) {
        IStream_Release(stream);
        return hres;
    }

    hres = XMLView_BindStatusCallback_Create(bsc_html, This->mon, stream, &bsc);
    IStream_Release(stream);
    if(FAILED(hres)) {
        IBindStatusCallback_OnStopBinding(bsc_html, hres, NULL);
        IBindStatusCallback_Release(bsc_html);
        return hres;
    }

    hres = RegisterBindStatusCallback(pibc, bsc, NULL, 0);
    IBindStatusCallback_Release(bsc);
    if(FAILED(hres)) {
        IBindStatusCallback_OnStopBinding(bsc_html, hres, NULL);
        IBindStatusCallback_Release(bsc_html);
        return hres;
    }

    hres = IMoniker_BindToStorage(pimkName, pibc, NULL,
            &IID_IStream, (void**)&stream);
    if(FAILED(hres)) {
        IBindStatusCallback_OnStopBinding(bsc_html, hres, NULL);
        IBindStatusCallback_Release(bsc_html);
        return hres;
    }

    if(stream)
        IStream_Release(stream);
    IBindStatusCallback_Release(bsc_html);
    return S_OK;
}

static HRESULT WINAPI XMLView_PersistMoniker_Save(IPersistMoniker *iface,
        IMoniker *pimkName, LPBC pbc, BOOL fRemember)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_SaveCompleted(
        IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_GetCurMoniker(
        IPersistMoniker *iface, IMoniker **ppimkName)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p)\n", This, ppimkName);
    return E_NOTIMPL;
}

static IPersistMonikerVtbl XMLView_PersistMonikerVtbl = {
    XMLView_PersistMoniker_QueryInterface,
    XMLView_PersistMoniker_AddRef,
    XMLView_PersistMoniker_Release,
    XMLView_PersistMoniker_GetClassID,
    XMLView_PersistMoniker_IsDirty,
    XMLView_PersistMoniker_Load,
    XMLView_PersistMoniker_Save,
    XMLView_PersistMoniker_SaveCompleted,
    XMLView_PersistMoniker_GetCurMoniker
};

static inline XMLView* impl_from_IPersistHistory(IPersistHistory *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IPersistHistory_iface);
}

static HRESULT WINAPI XMLView_PersistHistory_QueryInterface(
        IPersistHistory *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_PersistHistory_AddRef(IPersistHistory *iface)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_PersistHistory_Release(IPersistHistory *iface)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_PersistHistory_GetClassID(
        IPersistHistory *iface, CLSID *pClassID)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_LoadHistory(
        IPersistHistory *iface, IStream *pStream, IBindCtx *pbc)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p %p)\n", This, pStream, pbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_SaveHistory(
        IPersistHistory *iface, IStream *pStream)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_SetPositionCookie(
        IPersistHistory *iface, DWORD dwPositioncookie)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%d)\n", This, dwPositioncookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_GetPositionCookie(
        IPersistHistory *iface, DWORD *pdwPositioncookie)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pdwPositioncookie);
    return E_NOTIMPL;
}

static IPersistHistoryVtbl XMLView_PersistHistoryVtbl = {
    XMLView_PersistHistory_QueryInterface,
    XMLView_PersistHistory_AddRef,
    XMLView_PersistHistory_Release,
    XMLView_PersistHistory_GetClassID,
    XMLView_PersistHistory_LoadHistory,
    XMLView_PersistHistory_SaveHistory,
    XMLView_PersistHistory_SetPositionCookie,
    XMLView_PersistHistory_GetPositionCookie
};

static inline XMLView* impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IOleCommandTarget_iface);
}

static HRESULT WINAPI XMLView_OleCommandTarget_QueryInterface(
        IOleCommandTarget *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_OleCommandTarget_Release(IOleCommandTarget *iface)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_OleCommandTarget_QueryStatus(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%p %x %p %p)\n", This, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleCommandTarget_Exec(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
        VARIANT *pvaIn, VARIANT *pvaOut)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%p %d %x %p %p)\n", This, pguidCmdGroup,
            nCmdID, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static IOleCommandTargetVtbl XMLView_OleCommandTargetVtbl = {
    XMLView_OleCommandTarget_QueryInterface,
    XMLView_OleCommandTarget_AddRef,
    XMLView_OleCommandTarget_Release,
    XMLView_OleCommandTarget_QueryStatus,
    XMLView_OleCommandTarget_Exec
};

static inline XMLView* impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IOleObject_iface);
}

static HRESULT WINAPI XMLView_OleObject_QueryInterface(
        IOleObject *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_OleObject_AddRef(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_OleObject_Release(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_OleObject_SetClientSite(
        IOleObject *iface, IOleClientSite *pClientSite)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetClientSite(
        IOleObject *iface, IOleClientSite **ppClientSite)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetHostNames(IOleObject *iface,
        LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x)\n", This, dwSaveOption);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetMoniker(IOleObject *iface,
        DWORD dwWhichMoniker, IMoniker *pmk)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetMoniker(IOleObject *iface,
        DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %x %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_InitFromData(IOleObject *iface,
        IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %x)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetClipboardData(IOleObject *iface,
        DWORD dwReserved, IDataObject **ppDataObject)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_DoVerb(IOleObject *iface,
        LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p %p %d %p %p)\n", This, iVerb, lpmsg,
            pActiveSite, lindex, hwndParent, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_EnumVerbs(
        IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->{%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Update(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_IsUpToDate(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetUserType(IOleObject *iface,
        DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetExtent(IOleObject *iface,
        DWORD dwDrawAspect, SIZEL *psizel)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetExtent(IOleObject *iface,
        DWORD dwDrawAspect, SIZEL *psizel)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Advise(IOleObject *iface,
        IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Unadvise(
        IOleObject *iface, DWORD dwConnection)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_EnumAdvise(
        IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetMiscStatus(
        IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetColorScheme(
        IOleObject *iface, LOGPALETTE *pLogpal)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static IOleObjectVtbl XMLView_OleObjectVtbl = {
    XMLView_OleObject_QueryInterface,
    XMLView_OleObject_AddRef,
    XMLView_OleObject_Release,
    XMLView_OleObject_SetClientSite,
    XMLView_OleObject_GetClientSite,
    XMLView_OleObject_SetHostNames,
    XMLView_OleObject_Close,
    XMLView_OleObject_SetMoniker,
    XMLView_OleObject_GetMoniker,
    XMLView_OleObject_InitFromData,
    XMLView_OleObject_GetClipboardData,
    XMLView_OleObject_DoVerb,
    XMLView_OleObject_EnumVerbs,
    XMLView_OleObject_Update,
    XMLView_OleObject_IsUpToDate,
    XMLView_OleObject_GetUserClassID,
    XMLView_OleObject_GetUserType,
    XMLView_OleObject_SetExtent,
    XMLView_OleObject_GetExtent,
    XMLView_OleObject_Advise,
    XMLView_OleObject_Unadvise,
    XMLView_OleObject_EnumAdvise,
    XMLView_OleObject_GetMiscStatus,
    XMLView_OleObject_SetColorScheme
};

HRESULT XMLView_create(void **ppObj)
{
    XMLView *This;
    HRESULT hres;

    TRACE("(%p)\n", ppObj);

    This = heap_alloc_zero(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->IPersistMoniker_iface.lpVtbl = &XMLView_PersistMonikerVtbl;
    This->IPersistHistory_iface.lpVtbl = &XMLView_PersistHistoryVtbl;
    This->IOleCommandTarget_iface.lpVtbl = &XMLView_OleCommandTargetVtbl;
    This->IOleObject_iface.lpVtbl = &XMLView_OleObjectVtbl;
    This->ref = 1;

    hres = CoCreateInstance(&CLSID_HTMLDocument, (IUnknown*)&This->IPersistMoniker_iface,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&This->html_doc);
    if(FAILED(hres)) {
        heap_free(This);
        return hres;
    }

    *ppObj = &This->IPersistMoniker_iface;
    return S_OK;
}

#else

HRESULT XMLView_create(void **ppObj)
{
    MESSAGE("This program tried to use a XMLView object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif /* HAVE_LIBXML2 */
