/*
 *	TYPELIB Marshaler
 *
 *	Copyright 2002,2005	Marcus Meissner
 *
 * The olerelay debug channel allows you to see calls marshalled by
 * the typelib marshaller. It is not a generic COM relaying system.
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

#include "precomp.h"

#include "typelib.h"

#include <wine/exception.h>

static const WCHAR IDispatchW[] = { 'I','D','i','s','p','a','t','c','h',0};

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(olerelay);

static HRESULT TMarshalDispatchChannel_Create(
    IRpcChannelBuffer *pDelegateChannel, REFIID tmarshal_riid,
    IRpcChannelBuffer **ppChannel);

typedef struct _marshal_state {
    LPBYTE	base;
    int		size;
    int		curoff;
} marshal_state;

/* used in the olerelay code to avoid having the L"" stuff added by debugstr_w */
static char *relaystr(WCHAR *in) {
    char *tmp = (char *)debugstr_w(in);
    tmp += 2;
    tmp[strlen(tmp)-1] = '\0';
    return tmp;
}

static HRESULT
xbuf_resize(marshal_state *buf, DWORD newsize)
{
    if(buf->size >= newsize)
        return S_FALSE;

    if(buf->base)
    {
        buf->base = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf->base, newsize);
        if(!buf->base)
            return E_OUTOFMEMORY;
    }
    else
    {
        buf->base = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newsize);
        if(!buf->base)
            return E_OUTOFMEMORY;
    }
    buf->size = newsize;
    return S_OK;
}

static HRESULT
xbuf_add(marshal_state *buf, const BYTE *stuff, DWORD size)
{
    HRESULT hr;

    if(buf->size - buf->curoff < size)
    {
        hr = xbuf_resize(buf, buf->size + size + 100);
        if(FAILED(hr)) return hr;
    }
    memcpy(buf->base+buf->curoff,stuff,size);
    buf->curoff += size;
    return S_OK;
}

static HRESULT
xbuf_get(marshal_state *buf, LPBYTE stuff, DWORD size) {
    if (buf->size < buf->curoff+size) return E_FAIL;
    memcpy(stuff,buf->base+buf->curoff,size);
    buf->curoff += size;
    return S_OK;
}

static HRESULT
xbuf_skip(marshal_state *buf, DWORD size) {
    if (buf->size < buf->curoff+size) return E_FAIL;
    buf->curoff += size;
    return S_OK;
}

static HRESULT
_unmarshal_interface(marshal_state *buf, REFIID riid, LPUNKNOWN *pUnk) {
    IStream		*pStm;
    ULARGE_INTEGER	newpos;
    LARGE_INTEGER	seekto;
    ULONG		res;
    HRESULT		hres;
    DWORD		xsize;

    TRACE("...%s...\n",debugstr_guid(riid));
    
    *pUnk = NULL;
    hres = xbuf_get(buf,(LPBYTE)&xsize,sizeof(xsize));
    if (hres) {
        ERR("xbuf_get failed\n");
        return hres;
    }
    
    if (xsize == 0) return S_OK;
    
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) {
	ERR("Stream create failed %x\n",hres);
	return hres;
    }
    
    hres = IStream_Write(pStm,buf->base+buf->curoff,xsize,&res);
    if (hres) {
        ERR("stream write %x\n",hres);
        IStream_Release(pStm);
        return hres;
    }
    
    memset(&seekto,0,sizeof(seekto));
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    if (hres) {
        ERR("Failed Seek %x\n",hres);
        IStream_Release(pStm);
        return hres;
    }
    
    hres = CoUnmarshalInterface(pStm,riid,(LPVOID*)pUnk);
    if (hres) {
	ERR("Unmarshalling interface %s failed with %x\n",debugstr_guid(riid),hres);
	IStream_Release(pStm);
	return hres;
    }
    
    IStream_Release(pStm);
    return xbuf_skip(buf,xsize);
}

static HRESULT
_marshal_interface(marshal_state *buf, REFIID riid, LPUNKNOWN pUnk) {
    LPBYTE		tempbuf = NULL;
    IStream		*pStm = NULL;
    STATSTG		ststg;
    ULARGE_INTEGER	newpos;
    LARGE_INTEGER	seekto;
    ULONG		res;
    DWORD		xsize;
    HRESULT		hres;

    if (!pUnk) {
	/* this is valid, if for instance we serialize
	 * a VT_DISPATCH with NULL ptr which apparently
	 * can happen. S_OK to make sure we continue
	 * serializing.
	 */
        WARN("pUnk is NULL\n");
        xsize = 0;
        return xbuf_add(buf,(LPBYTE)&xsize,sizeof(xsize));
    }

    TRACE("...%s...\n",debugstr_guid(riid));
    
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) {
	ERR("Stream create failed %x\n",hres);
	goto fail;
    }
    
    hres = CoMarshalInterface(pStm,riid,pUnk,0,NULL,0);
    if (hres) {
	ERR("Marshalling interface %s failed with %x\n", debugstr_guid(riid), hres);
	goto fail;
    }
    
    hres = IStream_Stat(pStm,&ststg,STATFLAG_NONAME);
    if (hres) {
        ERR("Stream stat failed\n");
        goto fail;
    }
    
    tempbuf = HeapAlloc(GetProcessHeap(), 0, ststg.cbSize.u.LowPart);
    memset(&seekto,0,sizeof(seekto));
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    if (hres) {
        ERR("Failed Seek %x\n",hres);
        goto fail;
    }
    
    hres = IStream_Read(pStm,tempbuf,ststg.cbSize.u.LowPart,&res);
    if (hres) {
        ERR("Failed Read %x\n",hres);
        goto fail;
    }
    
    xsize = ststg.cbSize.u.LowPart;
    xbuf_add(buf,(LPBYTE)&xsize,sizeof(xsize));
    hres = xbuf_add(buf,tempbuf,ststg.cbSize.u.LowPart);
    
    HeapFree(GetProcessHeap(),0,tempbuf);
    IStream_Release(pStm);
    
    return hres;
    
fail:
    xsize = 0;
    xbuf_add(buf,(LPBYTE)&xsize,sizeof(xsize));
    if (pStm) IStream_Release(pStm);
    HeapFree(GetProcessHeap(), 0, tempbuf);
    return hres;
}

/********************* OLE Proxy/Stub Factory ********************************/
static HRESULT WINAPI
PSFacBuf_QueryInterface(LPPSFACTORYBUFFER iface, REFIID iid, LPVOID *ppv) {
    if (IsEqualIID(iid,&IID_IPSFactoryBuffer)||IsEqualIID(iid,&IID_IUnknown)) {
        *ppv = iface;
	/* No ref counting, static class */
	return S_OK;
    }
    FIXME("(%s) unknown IID?\n",debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PSFacBuf_AddRef(LPPSFACTORYBUFFER iface) { return 2; }
static ULONG WINAPI PSFacBuf_Release(LPPSFACTORYBUFFER iface) { return 1; }

struct ifacepsredirect_data
{
    ULONG size;
    DWORD mask;
    GUID  iid;
    ULONG nummethods;
    GUID  tlbid;
    GUID  base;
    ULONG name_len;
    ULONG name_offset;
};

struct tlibredirect_data
{
    ULONG  size;
    DWORD  res;
    ULONG  name_len;
    ULONG  name_offset;
    LANGID langid;
    WORD   flags;
    ULONG  help_len;
    ULONG  help_offset;
    WORD   major_version;
    WORD   minor_version;
};

static BOOL actctx_get_typelib_module(REFIID riid, WCHAR *module, DWORD len)
{
    struct ifacepsredirect_data *iface;
    struct tlibredirect_data *tlib;
    ACTCTX_SECTION_KEYED_DATA data;
    WCHAR *ptrW;

    data.cbSize = sizeof(data);
    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
            riid, &data))
        return FALSE;

    iface = (struct ifacepsredirect_data*)data.lpData;
    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
            &iface->tlbid, &data))
        return FALSE;

    tlib = (struct tlibredirect_data*)data.lpData;
    ptrW = (WCHAR*)((BYTE*)data.lpSectionBase + tlib->name_offset);

    if (tlib->name_len/sizeof(WCHAR) >= len) {
        ERR("need larger module buffer, %u\n", tlib->name_len);
        return FALSE;
    }

    memcpy(module, ptrW, tlib->name_len);
    module[tlib->name_len/sizeof(WCHAR)] = 0;
    return TRUE;
}

static HRESULT reg_get_typelib_module(REFIID riid, WCHAR *module, DWORD len)
{
    HKEY	ikey;
    REGSAM	opposite = (sizeof(void*) == 8) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL	is_wow64;
    char	tlguid[200],typelibkey[300],interfacekey[300],ver[100];
    char	tlfn[260];
    DWORD	tlguidlen, verlen, type;
    LONG	tlfnlen, err;

    sprintf( interfacekey, "Interface\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\Typelib",
	riid->Data1, riid->Data2, riid->Data3,
	riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
	riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]
    );

    err = RegOpenKeyExA(HKEY_CLASSES_ROOT,interfacekey,0,KEY_READ,&ikey);
    if (err && (opposite == KEY_WOW64_32KEY || (IsWow64Process(GetCurrentProcess(), &is_wow64)
                                                && is_wow64))) {
        err = RegOpenKeyExA(HKEY_CLASSES_ROOT,interfacekey,0,KEY_READ|opposite,&ikey);
    }
    if (err) {
        ERR("No %s key found.\n",interfacekey);
        return E_FAIL;
    }
    tlguidlen = sizeof(tlguid);
    if (RegQueryValueExA(ikey,NULL,NULL,&type,(LPBYTE)tlguid,&tlguidlen)) {
	ERR("Getting typelib guid failed.\n");
	RegCloseKey(ikey);
	return E_FAIL;
    }
    verlen = sizeof(ver);
    if (RegQueryValueExA(ikey,"Version",NULL,&type,(LPBYTE)ver,&verlen)) {
	ERR("Could not get version value?\n");
	RegCloseKey(ikey);
	return E_FAIL;
    }
    RegCloseKey(ikey);
    sprintf(typelibkey,"Typelib\\%s\\%s\\0\\win%u",tlguid,ver,(sizeof(void*) == 8) ? 64 : 32);
    tlfnlen = sizeof(tlfn);
    if (RegQueryValueA(HKEY_CLASSES_ROOT,typelibkey,tlfn,&tlfnlen)) {
#ifdef _WIN64
        sprintf(typelibkey,"Typelib\\%s\\%s\\0\\win32",tlguid,ver);
        tlfnlen = sizeof(tlfn);
        if (RegQueryValueA(HKEY_CLASSES_ROOT,typelibkey,tlfn,&tlfnlen)) {
#endif
            ERR("Could not get typelib fn?\n");
            return E_FAIL;
#ifdef _WIN64
        }
#endif
    }
    MultiByteToWideChar(CP_ACP, 0, tlfn, -1, module, len);
    return S_OK;
}

static HRESULT
_get_typeinfo_for_iid(REFIID riid, ITypeInfo **typeinfo)
{
    OLECHAR   moduleW[260];
    ITypeLib *typelib;
    HRESULT   hres;

    *typeinfo = NULL;

    moduleW[0] = 0;
    if (!actctx_get_typelib_module(riid, moduleW, sizeof(moduleW)/sizeof(moduleW[0]))) {
        hres = reg_get_typelib_module(riid, moduleW, sizeof(moduleW)/sizeof(moduleW[0]));
        if (FAILED(hres))
            return hres;
    }

    hres = LoadTypeLib(moduleW, &typelib);
    if (hres != S_OK) {
	ERR("Failed to load typelib for %s, but it should be there.\n",debugstr_guid(riid));
	return hres;
    }

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, riid, typeinfo);
    ITypeLib_Release(typelib);
    if (hres != S_OK)
	ERR("typelib does not contain info for %s\n", debugstr_guid(riid));

    return hres;
}

/*
 * Determine the number of functions including all inherited functions
 * and well as the size of the vtbl.
 * Note for non-dual dispinterfaces we simply return the size of IDispatch.
 */
static HRESULT num_of_funcs(ITypeInfo *tinfo, unsigned int *num,
                            unsigned int *vtbl_size)
{
    HRESULT hr;
    TYPEATTR *attr;
    ITypeInfo *tinfo2;
    UINT inherited_funcs = 0, i;

    *num = 0;
    if(vtbl_size) *vtbl_size = 0;

    hr = ITypeInfo_GetTypeAttr(tinfo, &attr);
    if (hr)
    {
        ERR("GetTypeAttr failed with %x\n", hr);
        return hr;
    }

    if(attr->typekind == TKIND_DISPATCH)
    {
        if(attr->wTypeFlags & TYPEFLAG_FDUAL)
        {
            HREFTYPE href;

            ITypeInfo_ReleaseTypeAttr(tinfo, attr);
            hr = ITypeInfo_GetRefTypeOfImplType(tinfo, -1, &href);
            if(FAILED(hr))
            {
                ERR("Unable to get interface href from dual dispinterface\n");
                return hr;
            }
            hr = ITypeInfo_GetRefTypeInfo(tinfo, href, &tinfo2);
            if(FAILED(hr))
            {
                ERR("Unable to get interface from dual dispinterface\n");
                return hr;
            }
            hr = num_of_funcs(tinfo2, num, vtbl_size);
            ITypeInfo_Release(tinfo2);
            return hr;
        }
        else /* non-dual dispinterface */
        {
            /* These will be the size of IDispatchVtbl */
            *num = attr->cbSizeVft / sizeof(void *);
            if(vtbl_size) *vtbl_size = attr->cbSizeVft;
            ITypeInfo_ReleaseTypeAttr(tinfo, attr);
            return hr;
        }
    }

    for (i = 0; i < attr->cImplTypes; i++)
    {
        HREFTYPE href;
        ITypeInfo *pSubTypeInfo;
        UINT sub_funcs;

        hr = ITypeInfo_GetRefTypeOfImplType(tinfo, i, &href);
        if (FAILED(hr)) goto end;
        hr = ITypeInfo_GetRefTypeInfo(tinfo, href, &pSubTypeInfo);
        if (FAILED(hr)) goto end;

        hr = num_of_funcs(pSubTypeInfo, &sub_funcs, NULL);
        ITypeInfo_Release(pSubTypeInfo);

        if(FAILED(hr)) goto end;
        inherited_funcs += sub_funcs;
    }

    *num = inherited_funcs + attr->cFuncs;
    if(vtbl_size) *vtbl_size = attr->cbSizeVft;

 end:
    ITypeInfo_ReleaseTypeAttr(tinfo, attr);
    return hr;
}

#ifdef __i386__

#include "pshpack1.h"
typedef struct _TMAsmProxy {
    DWORD	lealeax;
    BYTE	pushleax;
    BYTE	pushlval;
    DWORD	nr;
    BYTE	lcall;
    DWORD	xcall;
    BYTE	lret;
    WORD	bytestopop;
    WORD	nop;
} TMAsmProxy;
#include "poppack.h"

#elif defined(__x86_64__)

#include "pshpack1.h"
typedef struct _TMAsmProxy {
    BYTE    pushq_rbp;
    BYTE    movq_rsp_rbp[3];
    DWORD   subq_0x20_rsp;
    DWORD   movq_rcx_0x10rbp;
    DWORD   movq_rdx_0x18rbp;
    DWORD   movq_r8_0x20rbp;
    DWORD   movq_r9_0x28rbp;
    BYTE    movq_rcx[3];
    DWORD   nr;
    DWORD   leaq_0x10rbp_rdx;
    WORD    movq_rax;
    void   *xcall;
    WORD    callq_rax;
    BYTE    movq_rbp_rsp[3];
    BYTE    popq_rbp;
    BYTE    ret;
    DWORD   nop;
} TMAsmProxy;
#include "poppack.h"

#else /* __i386__ */
#ifdef _MSC_VER
#pragma message("You need to implement stubless proxies for your architecture")
#else
# warning You need to implement stubless proxies for your architecture
#endif
typedef struct _TMAsmProxy {
    char a;
} TMAsmProxy;
#endif

typedef struct _TMProxyImpl {
    LPVOID                             *lpvtbl;
    IRpcProxyBuffer                     IRpcProxyBuffer_iface;
    LONG				ref;

    TMAsmProxy				*asmstubs;
    ITypeInfo*				tinfo;
    IRpcChannelBuffer*			chanbuf;
    IID					iid;
    CRITICAL_SECTION	crit;
    IUnknown				*outerunknown;
    IDispatch				*dispatch;
    IRpcProxyBuffer			*dispatch_proxy;
} TMProxyImpl;

static inline TMProxyImpl *impl_from_IRpcProxyBuffer( IRpcProxyBuffer *iface )
{
    return CONTAINING_RECORD(iface, TMProxyImpl, IRpcProxyBuffer_iface);
}

static HRESULT WINAPI
TMProxyImpl_QueryInterface(LPRPCPROXYBUFFER iface, REFIID riid, LPVOID *ppv)
{
    TRACE("()\n");
    if (IsEqualIID(riid,&IID_IUnknown)||IsEqualIID(riid,&IID_IRpcProxyBuffer)) {
        *ppv = iface;
        IRpcProxyBuffer_AddRef(iface);
        return S_OK;
    }
    FIXME("no interface for %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
TMProxyImpl_AddRef(LPRPCPROXYBUFFER iface)
{
    TMProxyImpl *This = impl_from_IRpcProxyBuffer( iface );
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI
TMProxyImpl_Release(LPRPCPROXYBUFFER iface)
{
    TMProxyImpl *This = impl_from_IRpcProxyBuffer( iface );
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (!refCount)
    {
        if (This->dispatch_proxy) IRpcProxyBuffer_Release(This->dispatch_proxy);
        This->crit.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->crit);
        if (This->chanbuf) IRpcChannelBuffer_Release(This->chanbuf);
        VirtualFree(This->asmstubs, 0, MEM_RELEASE);
        HeapFree(GetProcessHeap(), 0, This->lpvtbl);
        ITypeInfo_Release(This->tinfo);
        CoTaskMemFree(This);
    }
    return refCount;
}

static HRESULT WINAPI
TMProxyImpl_Connect(
    LPRPCPROXYBUFFER iface,IRpcChannelBuffer* pRpcChannelBuffer)
{
    TMProxyImpl *This = impl_from_IRpcProxyBuffer( iface );

    TRACE("(%p)\n", pRpcChannelBuffer);

    EnterCriticalSection(&This->crit);

    IRpcChannelBuffer_AddRef(pRpcChannelBuffer);
    This->chanbuf = pRpcChannelBuffer;

    LeaveCriticalSection(&This->crit);

    if (This->dispatch_proxy)
    {
        IRpcChannelBuffer *pDelegateChannel;
        HRESULT hr = TMarshalDispatchChannel_Create(pRpcChannelBuffer, &This->iid, &pDelegateChannel);
        if (FAILED(hr))
            return hr;
        hr = IRpcProxyBuffer_Connect(This->dispatch_proxy, pDelegateChannel);
        IRpcChannelBuffer_Release(pDelegateChannel);
        return hr;
    }

    return S_OK;
}

static void WINAPI
TMProxyImpl_Disconnect(LPRPCPROXYBUFFER iface)
{
    TMProxyImpl *This = impl_from_IRpcProxyBuffer( iface );

    TRACE("()\n");

    EnterCriticalSection(&This->crit);

    IRpcChannelBuffer_Release(This->chanbuf);
    This->chanbuf = NULL;

    LeaveCriticalSection(&This->crit);

    if (This->dispatch_proxy)
        IRpcProxyBuffer_Disconnect(This->dispatch_proxy);
}


static const IRpcProxyBufferVtbl tmproxyvtable = {
    TMProxyImpl_QueryInterface,
    TMProxyImpl_AddRef,
    TMProxyImpl_Release,
    TMProxyImpl_Connect,
    TMProxyImpl_Disconnect
};

/* how much space do we use on stack in DWORD_PTR steps. */
static int
_argsize(TYPEDESC *tdesc, ITypeInfo *tinfo) {
    DWORD ret;
    switch (tdesc->vt) {
    case VT_I8:
    case VT_UI8:
        ret = 8;
        break;
    case VT_R8:
        ret = sizeof(double);
        break;
    case VT_CY:
        ret = sizeof(CY);
        break;
    case VT_DATE:
        ret = sizeof(DATE);
        break;
    case VT_DECIMAL:
        ret = sizeof(DECIMAL);
        break;
    case VT_VARIANT:
        ret = sizeof(VARIANT);
        break;
    case VT_USERDEFINED:
    {
        ITypeInfo *tinfo2;
        TYPEATTR *tattr;
        HRESULT hres;

        hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
        if (FAILED(hres))
            return 0; /* should fail critically in serialize_param */
        ITypeInfo_GetTypeAttr(tinfo2,&tattr);
        ret = tattr->cbSizeInstance;
        ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
        ITypeInfo_Release(tinfo2);
        break;
    }
    default:
        ret = sizeof(DWORD_PTR);
        break;
    }

    return (ret + sizeof(DWORD_PTR) - 1) / sizeof(DWORD_PTR);
}

/* how much space do we use on the heap (in bytes) */
static int
_xsize(const TYPEDESC *td, ITypeInfo *tinfo) {
    switch (td->vt) {
    case VT_DATE:
	return sizeof(DATE);
    case VT_CY:
        return sizeof(CY);
    case VT_VARIANT:
	return sizeof(VARIANT);
    case VT_CARRAY: {
	int i, arrsize = 1;
	const ARRAYDESC *adesc = td->u.lpadesc;

	for (i=0;i<adesc->cDims;i++)
	    arrsize *= adesc->rgbounds[i].cElements;
	return arrsize*_xsize(&adesc->tdescElem, tinfo);
    }
    case VT_UI8:
    case VT_I8:
    case VT_R8:
	return 8;
    case VT_UI2:
    case VT_I2:
    case VT_BOOL:
	return 2;
    case VT_UI1:
    case VT_I1:
	return 1;
    case VT_USERDEFINED:
    {
        ITypeInfo *tinfo2;
        TYPEATTR *tattr;
        HRESULT hres;
        DWORD ret;

        hres = ITypeInfo_GetRefTypeInfo(tinfo,td->u.hreftype,&tinfo2);
        if (FAILED(hres))
            return 0;
        ITypeInfo_GetTypeAttr(tinfo2,&tattr);
        ret = tattr->cbSizeInstance;
        ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
        ITypeInfo_Release(tinfo2);
        return ret;
    }
    default:
	return sizeof(DWORD_PTR);
    }
}

/* Whether we pass this type by reference or by value */
static BOOL
_passbyref(const TYPEDESC *td, ITypeInfo *tinfo) {
    return (td->vt == VT_USERDEFINED ||
            td->vt == VT_VARIANT     ||
            td->vt == VT_PTR);
}

static HRESULT
serialize_param(
    ITypeInfo		*tinfo,
    BOOL		writeit,
    BOOL		debugout,
    BOOL		dealloc,
    TYPEDESC		*tdesc,
    DWORD_PTR		*arg,
    marshal_state	*buf)
{
    HRESULT hres = S_OK;
    VARTYPE vartype;

    TRACE("(tdesc.vt %s)\n",debugstr_vt(tdesc->vt));

    vartype = tdesc->vt;
    if ((vartype & 0xf000) == VT_ARRAY)
        vartype = VT_SAFEARRAY;

    switch (vartype) {
    case VT_DATE:
    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%s\n", wine_dbgstr_longlong(*(ULONGLONG *)arg));
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,8);
	return hres;
    case VT_ERROR:
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_R4:
    case VT_UI4:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%x\n", *(DWORD *)arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%04x\n", *(WORD *)arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_I1:
    case VT_UI1:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%02x\n", *(BYTE *)arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_VARIANT: {
        if (debugout) TRACE_(olerelay)("%s", debugstr_variant((VARIANT *)arg));
        if (writeit)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            ULONG size = VARIANT_UserSize(&flags, buf->curoff, (VARIANT *)arg);
            xbuf_resize(buf, size);
            VARIANT_UserMarshal(&flags, buf->base + buf->curoff, (VARIANT *)arg);
            buf->curoff = size;
        }
        if (dealloc)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            VARIANT_UserFree(&flags, (VARIANT *)arg);
        }
        return S_OK;
    }
    case VT_BSTR: {
	if (writeit && debugout) {
	    if (*arg)
                   TRACE_(olerelay)("%s",relaystr((WCHAR*)*arg));
	    else
		    TRACE_(olerelay)("<bstr NULL>");
	}
        if (writeit)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            ULONG size = BSTR_UserSize(&flags, buf->curoff, (BSTR *)arg);
            xbuf_resize(buf, size);
            BSTR_UserMarshal(&flags, buf->base + buf->curoff, (BSTR *)arg);
            buf->curoff = size;
        }
        if (dealloc)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            BSTR_UserFree(&flags, (BSTR *)arg);
        }
        return S_OK;
    }
    case VT_PTR: {
	DWORD cookie;
	BOOL        derefhere = TRUE;

	if (tdesc->u.lptdesc->vt == VT_USERDEFINED) {
	    ITypeInfo	*tinfo2;
	    TYPEATTR	*tattr;

	    hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.lptdesc->u.hreftype,&tinfo2);
	    if (hres) {
		ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.lptdesc->u.hreftype);
		return hres;
	    }
	    ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	    switch (tattr->typekind) {
            case TKIND_ALIAS:
                if (tattr->tdescAlias.vt == VT_USERDEFINED)
                {
                    DWORD href = tattr->tdescAlias.u.hreftype;
                    ITypeInfo_ReleaseTypeAttr(tinfo, tattr);
                    ITypeInfo_Release(tinfo2);
                    hres = ITypeInfo_GetRefTypeInfo(tinfo,href,&tinfo2);
                    if (hres) {
                        ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.lptdesc->u.hreftype);
                        return hres;
                    }
                    ITypeInfo_GetTypeAttr(tinfo2,&tattr);
                    derefhere = (tattr->typekind != TKIND_DISPATCH &&
                                 tattr->typekind != TKIND_INTERFACE &&
                                 tattr->typekind != TKIND_COCLASS);
                }
                break;
	    case TKIND_ENUM:	/* confirmed */
	    case TKIND_RECORD:	/* FIXME: mostly untested */
		break;
	    case TKIND_DISPATCH:	/* will be done in VT_USERDEFINED case */
	    case TKIND_INTERFACE:	/* will be done in VT_USERDEFINED case */
            case TKIND_COCLASS:         /* will be done in VT_USERDEFINED case */
		derefhere=FALSE;
		break;
	    default:
		FIXME("unhandled switch cases tattr->typekind %d\n", tattr->typekind);
		derefhere=FALSE;
		break;
	    }
	    ITypeInfo_ReleaseTypeAttr(tinfo, tattr);
	    ITypeInfo_Release(tinfo2);
	}

	if (debugout) TRACE_(olerelay)("*");
	/* Write always, so the other side knows when it gets a NULL pointer.
	 */
	cookie = *arg ? 0x42424242 : 0;
	hres = xbuf_add(buf,(LPBYTE)&cookie,sizeof(cookie));
	if (hres)
	    return hres;
	if (!*arg) {
	    if (debugout) TRACE_(olerelay)("NULL");
	    return S_OK;
	}
	hres = serialize_param(tinfo,writeit,debugout,dealloc,tdesc->u.lptdesc,(DWORD_PTR *)*arg,buf);
	if (derefhere && dealloc) HeapFree(GetProcessHeap(),0,(LPVOID)*arg);
	return hres;
    }
    case VT_UNKNOWN:
	if (debugout) TRACE_(olerelay)("unk(0x%lx)", *arg);
	if (writeit)
	    hres = _marshal_interface(buf,&IID_IUnknown,(LPUNKNOWN)*arg);
	if (dealloc && *(IUnknown **)arg)
	    IUnknown_Release((LPUNKNOWN)*arg);
	return hres;
    case VT_DISPATCH:
	if (debugout) TRACE_(olerelay)("idisp(0x%lx)", *arg);
	if (writeit)
	    hres = _marshal_interface(buf,&IID_IDispatch,(LPUNKNOWN)*arg);
	if (dealloc && *(IUnknown **)arg)
	    IUnknown_Release((LPUNKNOWN)*arg);
	return hres;
    case VT_VOID:
	if (debugout) TRACE_(olerelay)("<void>");
	return S_OK;
    case VT_USERDEFINED: {
	ITypeInfo	*tinfo2;
	TYPEATTR	*tattr;

	hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
	if (hres) {
	    ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.hreftype);
	    return hres;
	}
	ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	switch (tattr->typekind) {
	case TKIND_DISPATCH:
	case TKIND_INTERFACE:
	    if (writeit)
	       hres=_marshal_interface(buf,&(tattr->guid),(LPUNKNOWN)arg);
	    if (dealloc)
	        IUnknown_Release((LPUNKNOWN)arg);
	    break;
        case TKIND_COCLASS: {
            GUID iid = tattr->guid;
            unsigned int i;
            int type_flags;

            for(i = 0; i < tattr->cImplTypes; i++) {
                if(SUCCEEDED(ITypeInfo_GetImplTypeFlags(tinfo2, i, &type_flags)) &&
                   type_flags == (IMPLTYPEFLAG_FSOURCE|IMPLTYPEFLAG_FDEFAULT)) {
                    ITypeInfo *tinfo3;
                    TYPEATTR *tattr2;
                    HREFTYPE href;
                    if(FAILED(ITypeInfo_GetRefTypeOfImplType(tinfo2, i, &href)))
                        break;
                    if(FAILED(ITypeInfo_GetRefTypeInfo(tinfo2, href, &tinfo3)))
                        break;
                    if(SUCCEEDED(ITypeInfo_GetTypeAttr(tinfo3, &tattr2))) {
                        iid = tattr2->guid;
                        ITypeInfo_ReleaseTypeAttr(tinfo3, tattr2);
                    }
                    ITypeInfo_Release(tinfo3);
                    break;
                }
            }

            if(writeit)
                hres=_marshal_interface(buf, &iid, (LPUNKNOWN)arg);
            if(dealloc)
                IUnknown_Release((LPUNKNOWN)arg);
            break;
        }
	case TKIND_RECORD: {
	    int i;
	    if (debugout) TRACE_(olerelay)("{");
	    for (i=0;i<tattr->cVars;i++) {
		VARDESC *vdesc;
		ELEMDESC *elem2;
		TYPEDESC *tdesc2;

		hres = ITypeInfo_GetVarDesc(tinfo2, i, &vdesc);
		if (hres) {
		    ERR("Could not get vardesc of %d\n",i);
		    return hres;
		}
		elem2 = &vdesc->elemdescVar;
		tdesc2 = &elem2->tdesc;
		hres = serialize_param(
		    tinfo2,
		    writeit,
		    debugout,
		    dealloc,
		    tdesc2,
		    (DWORD_PTR *)(((LPBYTE)arg)+vdesc->u.oInst),
		    buf
		);
                ITypeInfo_ReleaseVarDesc(tinfo2, vdesc);
		if (hres!=S_OK)
		    return hres;
		if (debugout && (i<(tattr->cVars-1)))
		    TRACE_(olerelay)(",");
	    }
	    if (debugout) TRACE_(olerelay)("}");
	    break;
	}
	case TKIND_ALIAS:
	    hres = serialize_param(tinfo2,writeit,debugout,dealloc,&tattr->tdescAlias,arg,buf);
	    break;
	case TKIND_ENUM:
	    hres = S_OK;
	    if (debugout) TRACE_(olerelay)("%x", *(DWORD *)arg);
	    if (writeit)
	        hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	    break;
	default:
	    FIXME("Unhandled typekind %d\n",tattr->typekind);
	    hres = E_FAIL;
	    break;
	}
	ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
	ITypeInfo_Release(tinfo2);
	return hres;
    }
    case VT_CARRAY: {
	ARRAYDESC *adesc = tdesc->u.lpadesc;
	int i, arrsize = 1;

	if (debugout) TRACE_(olerelay)("carr");
	for (i=0;i<adesc->cDims;i++) {
	    if (debugout) TRACE_(olerelay)("[%d]",adesc->rgbounds[i].cElements);
	    arrsize *= adesc->rgbounds[i].cElements;
	}
	if (debugout) TRACE_(olerelay)("(vt %s)",debugstr_vt(adesc->tdescElem.vt));
	if (debugout) TRACE_(olerelay)("[");
	for (i=0;i<arrsize;i++) {
            LPBYTE base = _passbyref(&adesc->tdescElem, tinfo) ? (LPBYTE) *arg : (LPBYTE) arg;
	    hres = serialize_param(tinfo, writeit, debugout, dealloc, &adesc->tdescElem, (DWORD_PTR *)((LPBYTE)base+i*_xsize(&adesc->tdescElem, tinfo)), buf);
	    if (hres)
		return hres;
	    if (debugout && (i<arrsize-1)) TRACE_(olerelay)(",");
	}
	if (debugout) TRACE_(olerelay)("]");
	if (dealloc)
	    HeapFree(GetProcessHeap(), 0, *(void **)arg);
	return S_OK;
    }
    case VT_SAFEARRAY: {
        if (writeit)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            ULONG size = LPSAFEARRAY_UserSize(&flags, buf->curoff, (LPSAFEARRAY *)arg);
            xbuf_resize(buf, size);
            LPSAFEARRAY_UserMarshal(&flags, buf->base + buf->curoff, (LPSAFEARRAY *)arg);
            buf->curoff = size;
        }
        if (dealloc)
        {
            ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
            LPSAFEARRAY_UserFree(&flags, (LPSAFEARRAY *)arg);
        }
        return S_OK;
    }
    default:
	ERR("Unhandled marshal type %d.\n",tdesc->vt);
	return S_OK;
    }
}

static HRESULT
deserialize_param(
    ITypeInfo		*tinfo,
    BOOL		readit,
    BOOL		debugout,
    BOOL		alloc,
    TYPEDESC		*tdesc,
    DWORD_PTR		*arg,
    marshal_state	*buf)
{
    HRESULT hres = S_OK;
    VARTYPE vartype;

    TRACE("vt %s at %p\n",debugstr_vt(tdesc->vt),arg);

    vartype = tdesc->vt;
    if ((vartype & 0xf000) == VT_ARRAY)
        vartype = VT_SAFEARRAY;

    while (1) {
	switch (vartype) {
	case VT_VARIANT: {
	    if (readit)
	    {
		ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
		unsigned char *buffer;
		buffer = VARIANT_UserUnmarshal(&flags, buf->base + buf->curoff, (VARIANT *)arg);
		buf->curoff = buffer - buf->base;
	    }
	    return S_OK;
	}
        case VT_DATE:
        case VT_I8:
        case VT_UI8:
        case VT_R8:
        case VT_CY:
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)arg,8);
		if (hres) ERR("Failed to read integer 8 byte\n");
	    }
	    if (debugout) TRACE_(olerelay)("%s", wine_dbgstr_longlong(*(ULONGLONG *)arg));
	    return hres;
        case VT_ERROR:
        case VT_I4:
        case VT_INT:
        case VT_UINT:
        case VT_R4:
        case VT_UI4:
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)arg,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
	    }
	    if (debugout) TRACE_(olerelay)("%x", *(DWORD *)arg);
	    return hres;
        case VT_I2:
        case VT_UI2:
        case VT_BOOL:
	    if (readit) {
		DWORD x;
		hres = xbuf_get(buf,(LPBYTE)&x,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
		memcpy(arg,&x,2);
	    }
	    if (debugout) TRACE_(olerelay)("%04x", *(WORD *)arg);
	    return hres;
        case VT_I1:
	case VT_UI1:
	    if (readit) {
		DWORD x;
		hres = xbuf_get(buf,(LPBYTE)&x,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
		memcpy(arg,&x,1);
	    }
	    if (debugout) TRACE_(olerelay)("%02x", *(BYTE *)arg);
	    return hres;
	case VT_BSTR: {
	    if (readit)
	    {
		ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
		unsigned char *buffer;
		buffer = BSTR_UserUnmarshal(&flags, buf->base + buf->curoff, (BSTR *)arg);
		buf->curoff = buffer - buf->base;
		if (debugout) TRACE_(olerelay)("%s",debugstr_w(*(BSTR *)arg));
	    }
	    return S_OK;
	}
	case VT_PTR: {
	    DWORD	cookie;
	    BOOL        derefhere = TRUE;

	    if (tdesc->u.lptdesc->vt == VT_USERDEFINED) {
		ITypeInfo	*tinfo2;
		TYPEATTR	*tattr;

		hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.lptdesc->u.hreftype,&tinfo2);
		if (hres) {
		    ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.lptdesc->u.hreftype);
		    return hres;
		}
		ITypeInfo_GetTypeAttr(tinfo2,&tattr);
		switch (tattr->typekind) {
                case TKIND_ALIAS:
                    if (tattr->tdescAlias.vt == VT_USERDEFINED)
                    {
                        DWORD href = tattr->tdescAlias.u.hreftype;
                        ITypeInfo_ReleaseTypeAttr(tinfo, tattr);
                        ITypeInfo_Release(tinfo2);
                        hres = ITypeInfo_GetRefTypeInfo(tinfo,href,&tinfo2);
                        if (hres) {
                            ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.lptdesc->u.hreftype);
                            return hres;
                        }
                        ITypeInfo_GetTypeAttr(tinfo2,&tattr);
                        derefhere = (tattr->typekind != TKIND_DISPATCH &&
                                     tattr->typekind != TKIND_INTERFACE &&
                                     tattr->typekind != TKIND_COCLASS);
                    }
                    break;
		case TKIND_ENUM:	/* confirmed */
		case TKIND_RECORD:	/* FIXME: mostly untested */
		    break;
		case TKIND_DISPATCH:	/* will be done in VT_USERDEFINED case */
		case TKIND_INTERFACE:	/* will be done in VT_USERDEFINED case */
                case TKIND_COCLASS:     /* will be done in VT_USERDEFINED case */
		    derefhere=FALSE;
		    break;
		default:
		    FIXME("unhandled switch cases tattr->typekind %d\n", tattr->typekind);
		    derefhere=FALSE;
		    break;
		}
		ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
		ITypeInfo_Release(tinfo2);
	    }
	    /* read it in all cases, we need to know if we have 
	     * NULL pointer or not.
	     */
	    hres = xbuf_get(buf,(LPBYTE)&cookie,sizeof(cookie));
	    if (hres) {
		ERR("Failed to load pointer cookie.\n");
		return hres;
	    }
	    if (cookie != 0x42424242) {
		/* we read a NULL ptr from the remote side */
		if (debugout) TRACE_(olerelay)("NULL");
		*arg = 0;
		return S_OK;
	    }
	    if (debugout) TRACE_(olerelay)("*");
	    if (alloc) {
		/* Allocate space for the referenced struct */
		if (derefhere)
		    *arg=(DWORD_PTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_xsize(tdesc->u.lptdesc, tinfo));
	    }
	    if (derefhere)
		return deserialize_param(tinfo, readit, debugout, alloc, tdesc->u.lptdesc, (DWORD_PTR *)*arg, buf);
	    else
		return deserialize_param(tinfo, readit, debugout, alloc, tdesc->u.lptdesc, arg, buf);
        }
	case VT_UNKNOWN:
	    /* FIXME: UNKNOWN is unknown ..., but allocate 4 byte for it */
	    if (alloc)
	        *arg=(DWORD_PTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DWORD_PTR));
	    hres = S_OK;
	    if (readit)
		hres = _unmarshal_interface(buf,&IID_IUnknown,(LPUNKNOWN*)arg);
	    if (debugout)
		TRACE_(olerelay)("unk(%p)",arg);
	    return hres;
	case VT_DISPATCH:
	    hres = S_OK;
	    if (readit)
		hres = _unmarshal_interface(buf,&IID_IDispatch,(LPUNKNOWN*)arg);
	    if (debugout)
		TRACE_(olerelay)("idisp(%p)",arg);
	    return hres;
	case VT_VOID:
	    if (debugout) TRACE_(olerelay)("<void>");
	    return S_OK;
	case VT_USERDEFINED: {
	    ITypeInfo	*tinfo2;
	    TYPEATTR	*tattr;

	    hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
	    if (hres) {
		ERR("Could not get typeinfo of hreftype %x for VT_USERDEFINED.\n",tdesc->u.hreftype);
		return hres;
	    }
	    hres = ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	    if (hres) {
		ERR("Could not get typeattr in VT_USERDEFINED.\n");
	    } else {
		switch (tattr->typekind) {
		case TKIND_DISPATCH:
		case TKIND_INTERFACE:
		    if (readit)
			hres = _unmarshal_interface(buf,&(tattr->guid),(LPUNKNOWN*)arg);
		    break;
                case TKIND_COCLASS: {
                    GUID iid = tattr->guid;
                    unsigned int i;
                    int type_flags;

                    for(i = 0; i < tattr->cImplTypes; i++) {
                        if(SUCCEEDED(ITypeInfo_GetImplTypeFlags(tinfo2, i, &type_flags)) &&
                           type_flags == (IMPLTYPEFLAG_FSOURCE|IMPLTYPEFLAG_FDEFAULT)) {
                            ITypeInfo *tinfo3;
                            TYPEATTR *tattr2;
                            HREFTYPE href;
                            if(FAILED(ITypeInfo_GetRefTypeOfImplType(tinfo2, i, &href)))
                                break;
                            if(FAILED(ITypeInfo_GetRefTypeInfo(tinfo2, href, &tinfo3)))
                                break;
                            if(SUCCEEDED(ITypeInfo_GetTypeAttr(tinfo3, &tattr2))) {
                                iid = tattr2->guid;
                                ITypeInfo_ReleaseTypeAttr(tinfo3, tattr2);
                            }
                            ITypeInfo_Release(tinfo3);
                            break;
                        }
                    }

                    if(readit)
                        hres = _unmarshal_interface(buf, &iid, (LPUNKNOWN*)arg);
                    break;
                }
		case TKIND_RECORD: {
		    int i;

		    if (debugout) TRACE_(olerelay)("{");
		    for (i=0;i<tattr->cVars;i++) {
			VARDESC *vdesc;

			hres = ITypeInfo_GetVarDesc(tinfo2, i, &vdesc);
			if (hres) {
			    ERR("Could not get vardesc of %d\n",i);
			    ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
			    ITypeInfo_Release(tinfo2);
			    return hres;
			}
			hres = deserialize_param(
			    tinfo2,
			    readit,
			    debugout,
			    alloc,
			    &vdesc->elemdescVar.tdesc,
			    (DWORD_PTR *)(((LPBYTE)arg)+vdesc->u.oInst),
			    buf
			);
                        ITypeInfo_ReleaseVarDesc(tinfo2, vdesc);
		        if (debugout && (i<tattr->cVars-1)) TRACE_(olerelay)(",");
		    }
		    if (debugout) TRACE_(olerelay)("}");
		    break;
		}
		case TKIND_ALIAS:
		    hres = deserialize_param(tinfo2,readit,debugout,alloc,&tattr->tdescAlias,arg,buf);
		    break;
		case TKIND_ENUM:
		    if (readit) {
		        hres = xbuf_get(buf,(LPBYTE)arg,sizeof(DWORD));
		        if (hres) ERR("Failed to read enum (4 byte)\n");
		    }
		    if (debugout) TRACE_(olerelay)("%x", *(DWORD *)arg);
		    break;
		default:
		    ERR("Unhandled typekind %d\n",tattr->typekind);
		    hres = E_FAIL;
		    break;
		}
		ITypeInfo_ReleaseTypeAttr(tinfo2, tattr);
	    }
	    if (hres)
		ERR("failed to stuballoc in TKIND_RECORD.\n");
	    ITypeInfo_Release(tinfo2);
	    return hres;
	}
	case VT_CARRAY: {
	    /* arg is pointing to the start of the array. */
            LPBYTE base = (LPBYTE) arg;
	    ARRAYDESC *adesc = tdesc->u.lpadesc;
	    int		arrsize,i;
	    arrsize = 1;
	    if (adesc->cDims > 1) FIXME("cDims > 1 in VT_CARRAY. Does it work?\n");
	    for (i=0;i<adesc->cDims;i++)
		arrsize *= adesc->rgbounds[i].cElements;
            if (_passbyref(&adesc->tdescElem, tinfo))
            {
	        base = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_xsize(tdesc->u.lptdesc, tinfo) * arrsize);
                *arg = (DWORD_PTR)base;
            }
	    for (i=0;i<arrsize;i++)
		deserialize_param(
		    tinfo,
		    readit,
		    debugout,
		    alloc,
		    &adesc->tdescElem,
		    (DWORD_PTR *)(base + i*_xsize(&adesc->tdescElem, tinfo)),
		    buf
		);
	    return S_OK;
	}
        case VT_SAFEARRAY: {
	    if (readit)
	    {
		ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
		unsigned char *buffer;
		buffer = LPSAFEARRAY_UserUnmarshal(&flags, buf->base + buf->curoff, (LPSAFEARRAY *)arg);
		buf->curoff = buffer - buf->base;
	    }
	    return S_OK;
	}
	default:
	    ERR("No handler for VT type %d!\n",tdesc->vt);
	    return S_OK;
	}
    }
}

/* Retrieves a function's funcdesc, searching back into inherited interfaces. */
static HRESULT get_funcdesc(ITypeInfo *tinfo, int iMethod, ITypeInfo **tactual, const FUNCDESC **fdesc,
                            BSTR *iname, BSTR *fname, UINT *num)
{
    HRESULT hr;
    UINT i, impl_types;
    UINT inherited_funcs = 0;
    TYPEATTR *attr;

    if (fname) *fname = NULL;
    if (iname) *iname = NULL;
    if (num) *num = 0;
    *tactual = NULL;

    hr = ITypeInfo_GetTypeAttr(tinfo, &attr);
    if (FAILED(hr))
    {
        ERR("GetTypeAttr failed with %x\n",hr);
        return hr;
    }

    if(attr->typekind == TKIND_DISPATCH)
    {
        if(attr->wTypeFlags & TYPEFLAG_FDUAL)
        {
            HREFTYPE href;
            ITypeInfo *tinfo2;

            hr = ITypeInfo_GetRefTypeOfImplType(tinfo, -1, &href);
            if(FAILED(hr))
            {
                ERR("Cannot get interface href from dual dispinterface\n");
                ITypeInfo_ReleaseTypeAttr(tinfo, attr);
                return hr;
            }
            hr = ITypeInfo_GetRefTypeInfo(tinfo, href, &tinfo2);
            if(FAILED(hr))
            {
                ERR("Cannot get interface from dual dispinterface\n");
                ITypeInfo_ReleaseTypeAttr(tinfo, attr);
                return hr;
            }
            hr = get_funcdesc(tinfo2, iMethod, tactual, fdesc, iname, fname, num);
            ITypeInfo_Release(tinfo2);
            ITypeInfo_ReleaseTypeAttr(tinfo, attr);
            return hr;
        }
        ERR("Shouldn't be called with a non-dual dispinterface\n");
        return E_FAIL;
    }

    impl_types = attr->cImplTypes;
    ITypeInfo_ReleaseTypeAttr(tinfo, attr);

    for (i = 0; i < impl_types; i++)
    {
        HREFTYPE href;
        ITypeInfo *pSubTypeInfo;
        UINT sub_funcs;

        hr = ITypeInfo_GetRefTypeOfImplType(tinfo, i, &href);
        if (FAILED(hr)) return hr;
        hr = ITypeInfo_GetRefTypeInfo(tinfo, href, &pSubTypeInfo);
        if (FAILED(hr)) return hr;

        hr = get_funcdesc(pSubTypeInfo, iMethod, tactual, fdesc, iname, fname, &sub_funcs);
        inherited_funcs += sub_funcs;
        ITypeInfo_Release(pSubTypeInfo);
        if(SUCCEEDED(hr)) return hr;
    }
    if(iMethod < inherited_funcs)
    {
        ERR("shouldn't be here\n");
        return E_INVALIDARG;
    }

    for(i = inherited_funcs; i <= iMethod; i++)
    {
        hr = ITypeInfoImpl_GetInternalFuncDesc(tinfo, i - inherited_funcs, fdesc);
        if(FAILED(hr))
        {
            if(num) *num = i;
            return hr;
        }
    }

    /* found it. We don't care about num so zero it */
    if(num) *num = 0;
    *tactual = tinfo;
    ITypeInfo_AddRef(*tactual);
    if (fname) ITypeInfo_GetDocumentation(tinfo,(*fdesc)->memid,fname,NULL,NULL,NULL);
    if (iname) ITypeInfo_GetDocumentation(tinfo,-1,iname,NULL,NULL,NULL);
    return S_OK;
}

static inline BOOL is_in_elem(const ELEMDESC *elem)
{
    return (elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN || !elem->u.paramdesc.wParamFlags);
}

static inline BOOL is_out_elem(const ELEMDESC *elem)
{
    return (elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT || !elem->u.paramdesc.wParamFlags);
}

static DWORD WINAPI xCall(int method, void **args)
{
    TMProxyImpl *tpinfo = args[0];
    DWORD_PTR *xargs;
    const FUNCDESC	*fdesc;
    HRESULT		hres;
    int			i;
    marshal_state	buf;
    RPCOLEMESSAGE	msg;
    ULONG		status;
    BSTR		fname,iname;
    BSTR		names[10];
    UINT		nrofnames;
    DWORD		remoteresult = 0;
    ITypeInfo 		*tinfo;
    IRpcChannelBuffer *chanbuf;

    EnterCriticalSection(&tpinfo->crit);

    hres = get_funcdesc(tpinfo->tinfo,method,&tinfo,&fdesc,&iname,&fname,NULL);
    if (hres) {
        ERR("Did not find typeinfo/funcdesc entry for method %d!\n",method);
        LeaveCriticalSection(&tpinfo->crit);
        return E_FAIL;
    }

    if (!tpinfo->chanbuf)
    {
        WARN("Tried to use disconnected proxy\n");
        ITypeInfo_Release(tinfo);
        LeaveCriticalSection(&tpinfo->crit);
        return RPC_E_DISCONNECTED;
    }
    chanbuf = tpinfo->chanbuf;
    IRpcChannelBuffer_AddRef(chanbuf);

    LeaveCriticalSection(&tpinfo->crit);

    if (TRACE_ON(olerelay)) {
       TRACE_(olerelay)("->");
	if (iname)
	    TRACE_(olerelay)("%s:",relaystr(iname));
	if (fname)
	    TRACE_(olerelay)("%s(%d)",relaystr(fname),method);
	else
	    TRACE_(olerelay)("%d",method);
	TRACE_(olerelay)("(");
    }

    SysFreeString(iname);
    SysFreeString(fname);

    memset(&buf,0,sizeof(buf));

    /* normal typelib driven serializing */

    /* Need them for hack below */
    memset(names,0,sizeof(names));
    if (ITypeInfo_GetNames(tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames))
	nrofnames = 0;
    if (nrofnames > sizeof(names)/sizeof(names[0]))
	ERR("Need more names!\n");

    xargs = (DWORD_PTR *)(args + 1);
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	if (TRACE_ON(olerelay)) {
	    if (i) TRACE_(olerelay)(",");
	    if (i+1<nrofnames && names[i+1])
		TRACE_(olerelay)("%s=",relaystr(names[i+1]));
	}
	/* No need to marshal other data than FIN and any VT_PTR. */
        if (!is_in_elem(elem))
        {
            if (elem->tdesc.vt != VT_PTR)
            {
                xargs+=_argsize(&elem->tdesc, tinfo);
                TRACE_(olerelay)("[out]");
                continue;
            }
            else
            {
                memset( *(void **)xargs, 0, _xsize( elem->tdesc.u.lptdesc, tinfo ) );
            }
        }

	hres = serialize_param(
	    tinfo,
	    is_in_elem(elem),
	    TRACE_ON(olerelay),
	    FALSE,
	    &elem->tdesc,
	    xargs,
	    &buf
	);

	if (hres) {
	    ERR("Failed to serialize param, hres %x\n",hres);
	    break;
	}
	xargs+=_argsize(&elem->tdesc, tinfo);
    }
    TRACE_(olerelay)(")");

    memset(&msg,0,sizeof(msg));
    msg.cbBuffer = buf.curoff;
    msg.iMethod  = method;
    hres = IRpcChannelBuffer_GetBuffer(chanbuf,&msg,&(tpinfo->iid));
    if (hres) {
	ERR("RpcChannelBuffer GetBuffer failed, %x\n",hres);
	goto exit;
    }
    memcpy(msg.Buffer,buf.base,buf.curoff);
    TRACE_(olerelay)("\n");
    hres = IRpcChannelBuffer_SendReceive(chanbuf,&msg,&status);
    if (hres) {
	ERR("RpcChannelBuffer SendReceive failed, %x\n",hres);
	goto exit;
    }

    TRACE_(olerelay)(" status = %08x (",status);
    if (buf.base)
	buf.base = HeapReAlloc(GetProcessHeap(),0,buf.base,msg.cbBuffer);
    else
	buf.base = HeapAlloc(GetProcessHeap(),0,msg.cbBuffer);
    buf.size = msg.cbBuffer;
    memcpy(buf.base,msg.Buffer,buf.size);
    buf.curoff = 0;

    /* generic deserializer using typelib description */
    xargs = (DWORD_PTR *)(args + 1);
    status = S_OK;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;

        if (i) TRACE_(olerelay)(",");
        if (i+1<nrofnames && names[i+1]) TRACE_(olerelay)("%s=",relaystr(names[i+1]));

	/* No need to marshal other data than FOUT and any VT_PTR */
	if (!is_out_elem(elem) && (elem->tdesc.vt != VT_PTR)) {
	    xargs += _argsize(&elem->tdesc, tinfo);
	    TRACE_(olerelay)("[in]");
	    continue;
	}
	hres = deserialize_param(
	    tinfo,
	    is_out_elem(elem),
	    TRACE_ON(olerelay),
	    FALSE,
	    &(elem->tdesc),
	    xargs,
	    &buf
        );
	if (hres) {
	    ERR("Failed to unmarshall param, hres %x\n",hres);
	    status = hres;
	    break;
	}
	xargs += _argsize(&elem->tdesc, tinfo);
    }

    hres = xbuf_get(&buf, (LPBYTE)&remoteresult, sizeof(DWORD));
    if (hres != S_OK)
        goto exit;
    TRACE_(olerelay)(") = %08x\n", remoteresult);

    hres = remoteresult;

exit:
    IRpcChannelBuffer_FreeBuffer(chanbuf,&msg);
    for (i = 0; i < nrofnames; i++)
        SysFreeString(names[i]);
    HeapFree(GetProcessHeap(),0,buf.base);
    IRpcChannelBuffer_Release(chanbuf);
    ITypeInfo_Release(tinfo);
    TRACE("-- 0x%08x\n", hres);
    return hres;
}

static HRESULT WINAPI ProxyIUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    TMProxyImpl *proxy = (TMProxyImpl *)iface;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (proxy->outerunknown)
        return IUnknown_QueryInterface(proxy->outerunknown, riid, ppv);

    FIXME("No interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ProxyIUnknown_AddRef(IUnknown *iface)
{
    TMProxyImpl *proxy = (TMProxyImpl *)iface;

    TRACE("\n");

    if (proxy->outerunknown)
        return IUnknown_AddRef(proxy->outerunknown);

    return 2; /* FIXME */
}

static ULONG WINAPI ProxyIUnknown_Release(IUnknown *iface)
{
    TMProxyImpl *proxy = (TMProxyImpl *)iface;

    TRACE("\n");

    if (proxy->outerunknown)
        return IUnknown_Release(proxy->outerunknown);

    return 1; /* FIXME */
}

static HRESULT WINAPI ProxyIDispatch_GetTypeInfoCount(LPDISPATCH iface, UINT * pctinfo)
{
    TMProxyImpl *This = (TMProxyImpl *)iface;

    TRACE("(%p)\n", pctinfo);

    return IDispatch_GetTypeInfoCount(This->dispatch, pctinfo);
}

static HRESULT WINAPI ProxyIDispatch_GetTypeInfo(LPDISPATCH iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    TMProxyImpl *This = (TMProxyImpl *)iface;

    TRACE("(%d, %x, %p)\n", iTInfo, lcid, ppTInfo);

    return IDispatch_GetTypeInfo(This->dispatch, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI ProxyIDispatch_GetIDsOfNames(LPDISPATCH iface, REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
{
    TMProxyImpl *This = (TMProxyImpl *)iface;

    TRACE("(%s, %p, %d, 0x%x, %p)\n", debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    return IDispatch_GetIDsOfNames(This->dispatch, riid, rgszNames,
                                   cNames, lcid, rgDispId);
}

static HRESULT WINAPI ProxyIDispatch_Invoke(LPDISPATCH iface, DISPID dispIdMember, REFIID riid, LCID lcid,
                                            WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult,
                                            EXCEPINFO * pExcepInfo, UINT * puArgErr)
{
    TMProxyImpl *This = (TMProxyImpl *)iface;

    TRACE("(%d, %s, 0x%x, 0x%x, %p, %p, %p, %p)\n", dispIdMember,
          debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult,
          pExcepInfo, puArgErr);

    return IDispatch_Invoke(This->dispatch, dispIdMember, riid, lcid,
                            wFlags, pDispParams, pVarResult, pExcepInfo,
                            puArgErr);
}

typedef struct
{
    IRpcChannelBuffer     IRpcChannelBuffer_iface;
    LONG                  refs;
    /* the IDispatch-derived interface we are handling */
    IID                   tmarshal_iid;
    IRpcChannelBuffer    *pDelegateChannel;
} TMarshalDispatchChannel;

static inline TMarshalDispatchChannel *impl_from_IRpcChannelBuffer(IRpcChannelBuffer *iface)
{
    return CONTAINING_RECORD(iface, TMarshalDispatchChannel, IRpcChannelBuffer_iface);
}

static HRESULT WINAPI TMarshalDispatchChannel_QueryInterface(IRpcChannelBuffer *iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown))
    {
        *ppv = iface;
        IRpcChannelBuffer_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI TMarshalDispatchChannel_AddRef(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TMarshalDispatchChannel_Release(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->refs);
    if (ref)
        return ref;

    IRpcChannelBuffer_Release(This->pDelegateChannel);
    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI TMarshalDispatchChannel_GetBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg, REFIID riid)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    TRACE("(%p, %s)\n", olemsg, debugstr_guid(riid));
    /* Note: we are pretending to invoke a method on the interface identified
     * by tmarshal_iid so that we can re-use the IDispatch proxy/stub code
     * without the RPC runtime getting confused by not exporting an IDispatch interface */
    return IRpcChannelBuffer_GetBuffer(This->pDelegateChannel, olemsg, &This->tmarshal_iid);
}

static HRESULT WINAPI TMarshalDispatchChannel_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    TRACE("(%p, %p)\n", olemsg, pstatus);
    return IRpcChannelBuffer_SendReceive(This->pDelegateChannel, olemsg, pstatus);
}

static HRESULT WINAPI TMarshalDispatchChannel_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    TRACE("(%p)\n", olemsg);
    return IRpcChannelBuffer_FreeBuffer(This->pDelegateChannel, olemsg);
}

static HRESULT WINAPI TMarshalDispatchChannel_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    TRACE("(%p,%p)\n", pdwDestContext, ppvDestContext);
    return IRpcChannelBuffer_GetDestCtx(This->pDelegateChannel, pdwDestContext, ppvDestContext);
}

static HRESULT WINAPI TMarshalDispatchChannel_IsConnected(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = impl_from_IRpcChannelBuffer(iface);
    TRACE("()\n");
    return IRpcChannelBuffer_IsConnected(This->pDelegateChannel);
}

static const IRpcChannelBufferVtbl TMarshalDispatchChannelVtbl =
{
    TMarshalDispatchChannel_QueryInterface,
    TMarshalDispatchChannel_AddRef,
    TMarshalDispatchChannel_Release,
    TMarshalDispatchChannel_GetBuffer,
    TMarshalDispatchChannel_SendReceive,
    TMarshalDispatchChannel_FreeBuffer,
    TMarshalDispatchChannel_GetDestCtx,
    TMarshalDispatchChannel_IsConnected
};

static HRESULT TMarshalDispatchChannel_Create(
    IRpcChannelBuffer *pDelegateChannel, REFIID tmarshal_riid,
    IRpcChannelBuffer **ppChannel)
{
    TMarshalDispatchChannel *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IRpcChannelBuffer_iface.lpVtbl = &TMarshalDispatchChannelVtbl;
    This->refs = 1;
    IRpcChannelBuffer_AddRef(pDelegateChannel);
    This->pDelegateChannel = pDelegateChannel;
    This->tmarshal_iid = *tmarshal_riid;

    *ppChannel = &This->IRpcChannelBuffer_iface;
    return S_OK;
}


static inline HRESULT get_facbuf_for_iid(REFIID riid, IPSFactoryBuffer **facbuf)
{
    HRESULT       hr;
    CLSID         clsid;

    if ((hr = CoGetPSClsid(riid, &clsid)))
        return hr;
    return CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL,
                             &IID_IPSFactoryBuffer, (LPVOID*)facbuf);
}

static HRESULT init_proxy_entry_point(TMProxyImpl *proxy, unsigned int num)
{
    int j;
    /* nrofargs including This */
    int nrofargs = 1;
    ITypeInfo *tinfo2;
    TMAsmProxy	*xasm = proxy->asmstubs + num;
    HRESULT hres;
    const FUNCDESC *fdesc;

    hres = get_funcdesc(proxy->tinfo, num, &tinfo2, &fdesc, NULL, NULL, NULL);
    if (hres) {
        ERR("GetFuncDesc %x should not fail here.\n",hres);
        return hres;
    }
    ITypeInfo_Release(tinfo2);
    /* some args take more than 4 byte on the stack */
    for (j=0;j<fdesc->cParams;j++)
        nrofargs += _argsize(&fdesc->lprgelemdescParam[j].tdesc, proxy->tinfo);

#ifdef __i386__
    if (fdesc->callconv != CC_STDCALL) {
        ERR("calling convention is not stdcall????\n");
        return E_FAIL;
    }
/* leal 4(%esp),%eax
 * pushl %eax
 * pushl <nr>
 * call xCall
 * lret <nr>
 */
    xasm->lealeax       = 0x0424448d;
    xasm->pushleax      = 0x50;
    xasm->pushlval      = 0x68;
    xasm->nr            = num;
    xasm->lcall         = 0xe8;
    xasm->xcall         = (char *)xCall - (char *)&xasm->lret;
    xasm->lret          = 0xc2;
    xasm->bytestopop    = nrofargs * 4;
    xasm->nop           = 0x9090;
    proxy->lpvtbl[fdesc->oVft / sizeof(void *)] = xasm;

#elif defined(__x86_64__)

    xasm->pushq_rbp         = 0x55;         /* pushq %rbp */
    xasm->movq_rsp_rbp[0]   = 0x48;         /* movq %rsp,%rbp */
    xasm->movq_rsp_rbp[1]   = 0x89;
    xasm->movq_rsp_rbp[2]   = 0xe5;
    xasm->subq_0x20_rsp     = 0x20ec8348;   /* subq 0x20,%rsp */
    xasm->movq_rcx_0x10rbp  = 0x104d8948;   /* movq %rcx,0x10(%rbp) */
    xasm->movq_rdx_0x18rbp  = 0x18558948;   /* movq %rdx,0x18(%rbp) */
    xasm->movq_r8_0x20rbp   = 0x2045894c;   /* movq %r8,0x20(%rbp) */
    xasm->movq_r9_0x28rbp   = 0x284d894c;   /* movq %r9,0x28(%rbp) */
    xasm->movq_rcx[0]       = 0x48;         /* movq <num>,%rcx */
    xasm->movq_rcx[1]       = 0xc7;
    xasm->movq_rcx[2]       = 0xc1;
    xasm->nr                = num;
    xasm->leaq_0x10rbp_rdx  = 0x10558d48;   /* leaq 0x10(%rbp),%rdx */
    xasm->movq_rax          = 0xb848;       /* movq <xCall>,%rax */
    xasm->xcall             = xCall;
    xasm->callq_rax         = 0xd0ff;       /* callq *%rax */
    xasm->movq_rbp_rsp[0]   = 0x48;         /* movq %rbp,%rsp */
    xasm->movq_rbp_rsp[1]   = 0x89;
    xasm->movq_rbp_rsp[2]   = 0xec;
    xasm->popq_rbp          = 0x5d;         /* popq %rbp */
    xasm->ret               = 0xc3;         /* ret */
    xasm->nop               = 0x90909090;   /* nop */
    proxy->lpvtbl[fdesc->oVft / sizeof(void *)] = xasm;

#else
    FIXME("not implemented on non i386\n");
    return E_FAIL;
#endif
    return S_OK;
}

static HRESULT WINAPI
PSFacBuf_CreateProxy(
    LPPSFACTORYBUFFER iface, IUnknown* pUnkOuter, REFIID riid,
    IRpcProxyBuffer **ppProxy, LPVOID *ppv)
{
    HRESULT	hres;
    ITypeInfo	*tinfo;
    unsigned int i, nroffuncs, vtbl_size;
    TMProxyImpl	*proxy;
    TYPEATTR	*typeattr;
    BOOL        defer_to_dispatch = FALSE;

    TRACE("(...%s...)\n",debugstr_guid(riid));
    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	ERR("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }

    hres = num_of_funcs(tinfo, &nroffuncs, &vtbl_size);
    TRACE("Got %d funcs, vtbl size %d\n", nroffuncs, vtbl_size);

    if (FAILED(hres)) {
        ERR("Cannot get number of functions for typeinfo %s\n",debugstr_guid(riid));
        ITypeInfo_Release(tinfo);
        return hres;
    }

    proxy = CoTaskMemAlloc(sizeof(TMProxyImpl));
    if (!proxy) return E_OUTOFMEMORY;

    proxy->dispatch = NULL;
    proxy->dispatch_proxy = NULL;
    proxy->outerunknown = pUnkOuter;
    proxy->asmstubs = VirtualAlloc(NULL, sizeof(TMAsmProxy) * nroffuncs, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!proxy->asmstubs) {
        ERR("Could not commit pages for proxy thunks\n");
        CoTaskMemFree(proxy);
        return E_OUTOFMEMORY;
    }
    proxy->IRpcProxyBuffer_iface.lpVtbl = &tmproxyvtable;
    /* one reference for the proxy */
    proxy->ref		= 1;
    proxy->tinfo	= tinfo;
    proxy->iid		= *riid;
    proxy->chanbuf      = 0;

    InitializeCriticalSection(&proxy->crit);
    proxy->crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TMProxyImpl.crit");

    proxy->lpvtbl = HeapAlloc(GetProcessHeap(), 0, vtbl_size);

    /* if we derive from IDispatch then defer to its proxy for its methods */
    hres = ITypeInfo_GetTypeAttr(tinfo, &typeattr);
    if (hres == S_OK)
    {
        if (typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE)
        {
            IPSFactoryBuffer *factory_buffer;
            hres = get_facbuf_for_iid(&IID_IDispatch, &factory_buffer);
            if (hres == S_OK)
            {
                hres = IPSFactoryBuffer_CreateProxy(factory_buffer, NULL,
                    &IID_IDispatch, &proxy->dispatch_proxy,
                    (void **)&proxy->dispatch);
                IPSFactoryBuffer_Release(factory_buffer);
            }
            if ((hres == S_OK) && (nroffuncs < 7))
            {
                ERR("nroffuncs calculated incorrectly (%d)\n", nroffuncs);
                hres = E_UNEXPECTED;
            }
            if (hres == S_OK)
            {
                defer_to_dispatch = TRUE;
            }
        }
        ITypeInfo_ReleaseTypeAttr(tinfo, typeattr);
    }

    for (i=0;i<nroffuncs;i++) {
	switch (i) {
	case 0:
		proxy->lpvtbl[i] = ProxyIUnknown_QueryInterface;
		break;
	case 1:
		proxy->lpvtbl[i] = ProxyIUnknown_AddRef;
		break;
	case 2:
		proxy->lpvtbl[i] = ProxyIUnknown_Release;
		break;
        case 3:
                if(!defer_to_dispatch) hres = init_proxy_entry_point(proxy, i);
                else proxy->lpvtbl[3] = ProxyIDispatch_GetTypeInfoCount;
                break;
        case 4:
                if(!defer_to_dispatch) hres = init_proxy_entry_point(proxy, i);
                else proxy->lpvtbl[4] = ProxyIDispatch_GetTypeInfo;
                break;
        case 5:
                if(!defer_to_dispatch) hres = init_proxy_entry_point(proxy, i);
                else proxy->lpvtbl[5] = ProxyIDispatch_GetIDsOfNames;
                break;
        case 6:
                if(!defer_to_dispatch) hres = init_proxy_entry_point(proxy, i);
                else proxy->lpvtbl[6] = ProxyIDispatch_Invoke;
                break;
	default:
                hres = init_proxy_entry_point(proxy, i);
	}
    }

    if (hres == S_OK)
    {
        *ppv = proxy;
        *ppProxy = &proxy->IRpcProxyBuffer_iface;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    else
        TMProxyImpl_Release(&proxy->IRpcProxyBuffer_iface);
    return hres;
}

typedef struct _TMStubImpl {
    IRpcStubBuffer              IRpcStubBuffer_iface;
    LONG			ref;

    LPUNKNOWN			pUnk;
    ITypeInfo			*tinfo;
    IID				iid;
    IRpcStubBuffer		*dispatch_stub;
    BOOL			dispatch_derivative;
} TMStubImpl;

static inline TMStubImpl *impl_from_IRpcStubBuffer(IRpcStubBuffer *iface)
{
    return CONTAINING_RECORD(iface, TMStubImpl, IRpcStubBuffer_iface);
}

static HRESULT WINAPI
TMStubImpl_QueryInterface(LPRPCSTUBBUFFER iface, REFIID riid, LPVOID *ppv)
{
    if (IsEqualIID(riid,&IID_IRpcStubBuffer)||IsEqualIID(riid,&IID_IUnknown)){
        *ppv = iface;
	IRpcStubBuffer_AddRef(iface);
	return S_OK;
    }
    FIXME("%s, not supported IID.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
TMStubImpl_AddRef(LPRPCSTUBBUFFER iface)
{
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI
TMStubImpl_Release(LPRPCSTUBBUFFER iface)
{
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount)
    {
        IRpcStubBuffer_Disconnect(iface);
        ITypeInfo_Release(This->tinfo);
        if (This->dispatch_stub)
            IRpcStubBuffer_Release(This->dispatch_stub);
        CoTaskMemFree(This);
    }
    return refCount;
}

static HRESULT WINAPI
TMStubImpl_Connect(LPRPCSTUBBUFFER iface, LPUNKNOWN pUnkServer)
{
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);

    TRACE("(%p)->(%p)\n", This, pUnkServer);

    IUnknown_AddRef(pUnkServer);
    This->pUnk = pUnkServer;

    if (This->dispatch_stub)
        IRpcStubBuffer_Connect(This->dispatch_stub, pUnkServer);

    return S_OK;
}

static void WINAPI
TMStubImpl_Disconnect(LPRPCSTUBBUFFER iface)
{
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);

    TRACE("(%p)->()\n", This);

    if (This->pUnk)
    {
        IUnknown_Release(This->pUnk);
        This->pUnk = NULL;
    }

    if (This->dispatch_stub)
        IRpcStubBuffer_Disconnect(This->dispatch_stub);
}

static HRESULT WINAPI
TMStubImpl_Invoke(
    LPRPCSTUBBUFFER iface, RPCOLEMESSAGE* xmsg,IRpcChannelBuffer*rpcchanbuf)
{
#if defined(__i386__) || defined(__x86_64__)
    int		i;
    const FUNCDESC *fdesc;
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);
    HRESULT	hres;
    DWORD_PTR   *args = NULL, *xargs;
    DWORD	res, nrofargs;
    marshal_state	buf;
    UINT	nrofnames = 0;
    BSTR	names[10];
    BSTR	iname = NULL;
    ITypeInfo 	*tinfo = NULL;

    TRACE("...\n");

    if (xmsg->iMethod < 3) {
        ERR("IUnknown methods cannot be marshaled by the typelib marshaler\n");
        return E_UNEXPECTED;
    }

    if (This->dispatch_derivative && xmsg->iMethod < sizeof(IDispatchVtbl)/sizeof(void *))
    {
        if (!This->dispatch_stub)
        {
            IPSFactoryBuffer *factory_buffer;
            hres = get_facbuf_for_iid(&IID_IDispatch, &factory_buffer);
            if (hres == S_OK)
            {
                hres = IPSFactoryBuffer_CreateStub(factory_buffer, &IID_IDispatch,
                    This->pUnk, &This->dispatch_stub);
                IPSFactoryBuffer_Release(factory_buffer);
            }
            if (hres != S_OK)
                return hres;
        }
        return IRpcStubBuffer_Invoke(This->dispatch_stub, xmsg, rpcchanbuf);
    }

    memset(&buf,0,sizeof(buf));
    buf.size	= xmsg->cbBuffer;
    buf.base	= HeapAlloc(GetProcessHeap(), 0, xmsg->cbBuffer);
    memcpy(buf.base, xmsg->Buffer, xmsg->cbBuffer);
    buf.curoff	= 0;

    hres = get_funcdesc(This->tinfo,xmsg->iMethod,&tinfo,&fdesc,&iname,NULL,NULL);
    if (hres) {
	ERR("GetFuncDesc on method %d failed with %x\n",xmsg->iMethod,hres);
	return hres;
    }

    if (iname && !lstrcmpW(iname, IDispatchW))
    {
        ERR("IDispatch cannot be marshaled by the typelib marshaler\n");
        hres = E_UNEXPECTED;
        SysFreeString (iname);
        goto exit;
    }

    SysFreeString (iname);

    /* Need them for hack below */
    memset(names,0,sizeof(names));
    ITypeInfo_GetNames(tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames);
    if (nrofnames > sizeof(names)/sizeof(names[0])) {
	ERR("Need more names!\n");
    }

    /*dump_FUNCDESC(fdesc);*/
    nrofargs = 0;
    for (i=0;i<fdesc->cParams;i++)
	nrofargs += _argsize(&fdesc->lprgelemdescParam[i].tdesc, tinfo);
    args = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nrofargs+1)*sizeof(DWORD_PTR));
    if (!args)
    {
        hres = E_OUTOFMEMORY;
        goto exit;
    }

    /* Allocate all stuff used by call. */
    xargs = args+1;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;

	hres = deserialize_param(
	   tinfo,
	   is_in_elem(elem),
	   FALSE,
	   TRUE,
	   &(elem->tdesc),
	   xargs,
	   &buf
	);
	xargs += _argsize(&elem->tdesc, tinfo);
	if (hres) {
	    ERR("Failed to deserialize param %s, hres %x\n",relaystr(names[i+1]),hres);
	    break;
	}
    }

    args[0] = (DWORD_PTR)This->pUnk;

    __TRY
    {
        res = _invoke(
            (*((FARPROC**)args[0]))[fdesc->oVft / sizeof(DWORD_PTR)],
            fdesc->callconv,
            (xargs-args),
            args
        );
    }
    __EXCEPT_ALL
    {
        DWORD dwExceptionCode = GetExceptionCode();
        ERR("invoke call failed with exception 0x%08x (%d)\n", dwExceptionCode, dwExceptionCode);
        if (FAILED(dwExceptionCode))
            hres = dwExceptionCode;
        else
            hres = HRESULT_FROM_WIN32(dwExceptionCode);
    }
    __ENDTRY

    if (hres != S_OK)
        goto exit;

    buf.curoff = 0;

    xargs = args+1;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	hres = serialize_param(
	   tinfo,
	   is_out_elem(elem),
	   FALSE,
	   TRUE,
	   &elem->tdesc,
	   xargs,
	   &buf
	);
	xargs += _argsize(&elem->tdesc, tinfo);
	if (hres) {
	    ERR("Failed to stuballoc param, hres %x\n",hres);
	    break;
	}
    }

    hres = xbuf_add (&buf, (LPBYTE)&res, sizeof(DWORD));

    if (hres != S_OK)
        goto exit;

    xmsg->cbBuffer	= buf.curoff;
    hres = IRpcChannelBuffer_GetBuffer(rpcchanbuf, xmsg, &This->iid);
    if (hres != S_OK)
        ERR("IRpcChannelBuffer_GetBuffer failed with error 0x%08x\n", hres);

    if (hres == S_OK)
        memcpy(xmsg->Buffer, buf.base, buf.curoff);

exit:
    for (i = 0; i < nrofnames; i++)
        SysFreeString(names[i]);

    ITypeInfo_Release(tinfo);
    HeapFree(GetProcessHeap(), 0, args);

    HeapFree(GetProcessHeap(), 0, buf.base);

    TRACE("returning\n");
    return hres;
#else
    FIXME( "not implemented on non-i386\n" );
    return E_FAIL;
#endif
}

static LPRPCSTUBBUFFER WINAPI
TMStubImpl_IsIIDSupported(LPRPCSTUBBUFFER iface, REFIID riid) {
    FIXME("Huh (%s)?\n",debugstr_guid(riid));
    return NULL;
}

static ULONG WINAPI
TMStubImpl_CountRefs(LPRPCSTUBBUFFER iface) {
    TMStubImpl *This = impl_from_IRpcStubBuffer(iface);

    FIXME("()\n");
    return This->ref; /*FIXME? */
}

static HRESULT WINAPI
TMStubImpl_DebugServerQueryInterface(LPRPCSTUBBUFFER iface, LPVOID *ppv) {
    return E_NOTIMPL;
}

static void WINAPI
TMStubImpl_DebugServerRelease(LPRPCSTUBBUFFER iface, LPVOID ppv) {
    return;
}

static const IRpcStubBufferVtbl tmstubvtbl = {
    TMStubImpl_QueryInterface,
    TMStubImpl_AddRef,
    TMStubImpl_Release,
    TMStubImpl_Connect,
    TMStubImpl_Disconnect,
    TMStubImpl_Invoke,
    TMStubImpl_IsIIDSupported,
    TMStubImpl_CountRefs,
    TMStubImpl_DebugServerQueryInterface,
    TMStubImpl_DebugServerRelease
};

static HRESULT WINAPI
PSFacBuf_CreateStub(
    LPPSFACTORYBUFFER iface, REFIID riid,IUnknown *pUnkServer,
    IRpcStubBuffer** ppStub
) {
    HRESULT hres;
    ITypeInfo	*tinfo;
    TMStubImpl	*stub;
    TYPEATTR *typeattr;
    IUnknown *obj;

    TRACE("(%s,%p,%p)\n",debugstr_guid(riid),pUnkServer,ppStub);

    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	ERR("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }

    /* FIXME: This is not exactly right. We should probably call QI later. */
    hres = IUnknown_QueryInterface(pUnkServer, riid, (void**)&obj);
    if (FAILED(hres)) {
        WARN("Could not get %s iface: %08x\n", debugstr_guid(riid), hres);
        obj = pUnkServer;
        IUnknown_AddRef(obj);
    }

    stub = CoTaskMemAlloc(sizeof(TMStubImpl));
    if (!stub) {
        IUnknown_Release(obj);
	return E_OUTOFMEMORY;
    }
    stub->IRpcStubBuffer_iface.lpVtbl = &tmstubvtbl;
    stub->ref		= 1;
    stub->tinfo		= tinfo;
    stub->dispatch_stub = NULL;
    stub->dispatch_derivative = FALSE;
    stub->iid		= *riid;
    hres = IRpcStubBuffer_Connect(&stub->IRpcStubBuffer_iface, obj);
    *ppStub = &stub->IRpcStubBuffer_iface;
    TRACE("IRpcStubBuffer: %p\n", stub);
    if (hres)
	ERR("Connect to pUnkServer failed?\n");

    /* if we derive from IDispatch then defer to its stub for some of its methods */
    hres = ITypeInfo_GetTypeAttr(tinfo, &typeattr);
    if (hres == S_OK)
    {
        if (typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE)
            stub->dispatch_derivative = TRUE;
        ITypeInfo_ReleaseTypeAttr(tinfo, typeattr);
    }

    IUnknown_Release(obj);
    return hres;
}

static const IPSFactoryBufferVtbl psfacbufvtbl = {
    PSFacBuf_QueryInterface,
    PSFacBuf_AddRef,
    PSFacBuf_Release,
    PSFacBuf_CreateProxy,
    PSFacBuf_CreateStub
};

static IPSFactoryBuffer psfac = { &psfacbufvtbl };

/***********************************************************************
 *           TMARSHAL_DllGetClassObject
 */
HRESULT TMARSHAL_DllGetClassObject(REFCLSID rclsid, REFIID iid, void **ppv)
{
    return IPSFactoryBuffer_QueryInterface(&psfac, iid, ppv);
}
