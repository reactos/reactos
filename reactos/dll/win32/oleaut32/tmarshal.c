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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"

#include "ole2.h"
#include "propidl.h" /* for LPSAFEARRAY_User* functions */
#include "typelib.h"
#include "variant.h"
#include "wine/debug.h"
#include "wine/exception.h"

static const WCHAR IDispatchW[] = { 'I','D','i','s','p','a','t','c','h',0};

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(olerelay);

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

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
        return hres;
    }
    
    memset(&seekto,0,sizeof(seekto));
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    if (hres) {
        ERR("Failed Seek %x\n",hres);
        return hres;
    }
    
    hres = CoUnmarshalInterface(pStm,riid,(LPVOID*)pUnk);
    if (hres) {
	ERR("Unmarshalling interface %s failed with %x\n",debugstr_guid(riid),hres);
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

    hres = E_FAIL;

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
    
    hres = IStream_Stat(pStm,&ststg,0);
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
    if (pStm) IUnknown_Release(pStm);
    HeapFree(GetProcessHeap(), 0, tempbuf);
    return hres;
}

/********************* OLE Proxy/Stub Factory ********************************/
static HRESULT WINAPI
PSFacBuf_QueryInterface(LPPSFACTORYBUFFER iface, REFIID iid, LPVOID *ppv) {
    if (IsEqualIID(iid,&IID_IPSFactoryBuffer)||IsEqualIID(iid,&IID_IUnknown)) {
	*ppv = (LPVOID)iface;
	/* No ref counting, static class */
	return S_OK;
    }
    FIXME("(%s) unknown IID?\n",debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PSFacBuf_AddRef(LPPSFACTORYBUFFER iface) { return 2; }
static ULONG WINAPI PSFacBuf_Release(LPPSFACTORYBUFFER iface) { return 1; }

static HRESULT
_get_typeinfo_for_iid(REFIID riid, ITypeInfo**ti) {
    HRESULT	hres;
    HKEY	ikey;
    char	tlguid[200],typelibkey[300],interfacekey[300],ver[100];
    char	tlfn[260];
    OLECHAR	tlfnW[260];
    DWORD	tlguidlen, verlen, type;
    LONG	tlfnlen;
    ITypeLib	*tl;

    sprintf( interfacekey, "Interface\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\Typelib",
	riid->Data1, riid->Data2, riid->Data3,
	riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
	riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]
    );

    if (RegOpenKeyA(HKEY_CLASSES_ROOT,interfacekey,&ikey)) {
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
    sprintf(typelibkey,"Typelib\\%s\\%s\\0\\win32",tlguid,ver);
    tlfnlen = sizeof(tlfn);
    if (RegQueryValueA(HKEY_CLASSES_ROOT,typelibkey,tlfn,&tlfnlen)) {
	ERR("Could not get typelib fn?\n");
	return E_FAIL;
    }
    MultiByteToWideChar(CP_ACP, 0, tlfn, -1, tlfnW, sizeof(tlfnW) / sizeof(tlfnW[0]));
    hres = LoadTypeLib(tlfnW,&tl);
    if (hres) {
	ERR("Failed to load typelib for %s, but it should be there.\n",debugstr_guid(riid));
	return hres;
    }
    hres = ITypeLib_GetTypeInfoOfGuid(tl,riid,ti);
    if (hres) {
	ERR("typelib does not contain info for %s?\n",debugstr_guid(riid));
	ITypeLib_Release(tl);
	return hres;
    }
    ITypeLib_Release(tl);
    return hres;
}

/*
 * Determine the number of functions including all inherited functions.
 * Note for non-dual dispinterfaces we simply return the size of IDispatch.
 */
static HRESULT num_of_funcs(ITypeInfo *tinfo, unsigned int *num)
{
    HRESULT hres;
    TYPEATTR *attr;
    ITypeInfo *tinfo2;

    *num = 0;
    hres = ITypeInfo_GetTypeAttr(tinfo, &attr);
    if (hres) {
        ERR("GetTypeAttr failed with %x\n",hres);
        return hres;
    }

    if(attr->typekind == TKIND_DISPATCH && (attr->wTypeFlags & TYPEFLAG_FDUAL))
    {
        HREFTYPE href;
        hres = ITypeInfo_GetRefTypeOfImplType(tinfo, -1, &href);
        if(FAILED(hres))
        {
            ERR("Unable to get interface href from dual dispinterface\n");
            goto end;
        }
        hres = ITypeInfo_GetRefTypeInfo(tinfo, href, &tinfo2);
        if(FAILED(hres))
        {
            ERR("Unable to get interface from dual dispinterface\n");
            goto end;
        }
        hres = num_of_funcs(tinfo2, num);
        ITypeInfo_Release(tinfo2);
    }
    else
    {
        *num = attr->cbSizeVft / 4;
    }

 end:
    ITypeInfo_ReleaseTypeAttr(tinfo, attr);
    return hres;
}

#ifdef __i386__

#include "pshpack1.h"

typedef struct _TMAsmProxy {
    BYTE	popleax;
    BYTE	pushlval;
    BYTE	nr;
    BYTE	pushleax;
    BYTE	lcall;
    DWORD	xcall;
    BYTE	lret;
    WORD	bytestopop;
} TMAsmProxy;

#include "poppack.h"

#else /* __i386__ */
# warning You need to implement stubless proxies for your architecture
typedef struct _TMAsmProxy {
} TMAsmProxy;
#endif

typedef struct _TMProxyImpl {
    LPVOID                             *lpvtbl;
    const IRpcProxyBufferVtbl          *lpvtbl2;
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

static HRESULT WINAPI
TMProxyImpl_QueryInterface(LPRPCPROXYBUFFER iface, REFIID riid, LPVOID *ppv)
{
    TRACE("()\n");
    if (IsEqualIID(riid,&IID_IUnknown)||IsEqualIID(riid,&IID_IRpcProxyBuffer)) {
        *ppv = (LPVOID)iface;
        IRpcProxyBuffer_AddRef(iface);
        return S_OK;
    }
    FIXME("no interface for %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
TMProxyImpl_AddRef(LPRPCPROXYBUFFER iface)
{
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI
TMProxyImpl_Release(LPRPCPROXYBUFFER iface)
{
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);
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
    ICOM_THIS_MULTI(TMProxyImpl, lpvtbl2, iface);

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
        return IRpcProxyBuffer_Connect(This->dispatch_proxy, pDelegateChannel);
    }

    return S_OK;
}

static void WINAPI
TMProxyImpl_Disconnect(LPRPCPROXYBUFFER iface)
{
    ICOM_THIS_MULTI(TMProxyImpl, lpvtbl2, iface);

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

/* how much space do we use on stack in DWORD steps. */
int
_argsize(DWORD vt) {
    switch (vt) {
    case VT_UI8:
	return 8/sizeof(DWORD);
    case VT_R8:
        return sizeof(double)/sizeof(DWORD);
    case VT_CY:
        return sizeof(CY)/sizeof(DWORD);
    case VT_DATE:
	return sizeof(DATE)/sizeof(DWORD);
    case VT_VARIANT:
	return (sizeof(VARIANT)+3)/sizeof(DWORD);
    default:
	return 1;
    }
}

static int
_xsize(const TYPEDESC *td) {
    switch (td->vt) {
    case VT_DATE:
	return sizeof(DATE);
    case VT_VARIANT:
	return sizeof(VARIANT)+3;
    case VT_CARRAY: {
	int i, arrsize = 1;
	const ARRAYDESC *adesc = td->u.lpadesc;

	for (i=0;i<adesc->cDims;i++)
	    arrsize *= adesc->rgbounds[i].cElements;
	return arrsize*_xsize(&adesc->tdescElem);
    }
    case VT_UI8:
    case VT_I8:
	return 8;
    case VT_UI2:
    case VT_I2:
	return 2;
    case VT_UI1:
    case VT_I1:
	return 1;
    default:
	return 4;
    }
}

static HRESULT
serialize_param(
    ITypeInfo		*tinfo,
    BOOL		writeit,
    BOOL		debugout,
    BOOL		dealloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf)
{
    HRESULT hres = S_OK;

    TRACE("(tdesc.vt %s)\n",debugstr_vt(tdesc->vt));

    switch (tdesc->vt) {
    case VT_EMPTY: /* nothing. empty variant for instance */
	return S_OK;
    case VT_I8:
    case VT_UI8:
    case VT_CY:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%x%x\n",arg[0],arg[1]);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,8);
	return hres;
    case VT_BOOL:
    case VT_ERROR:
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_R4:
    case VT_UI4:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%x\n",*arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_I2:
    case VT_UI2:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%04x\n",*arg & 0xffff);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_I1:
    case VT_UI1:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("%02x\n",*arg & 0xff);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_I4|VT_BYREF:
	hres = S_OK;
	if (debugout) TRACE_(olerelay)("&0x%x\n",*arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)(DWORD*)*arg,sizeof(DWORD));
	/* do not dealloc at this time */
	return hres;
    case VT_VARIANT: {
	TYPEDESC	tdesc2;
	VARIANT		*vt = (VARIANT*)arg;
	DWORD		vttype = V_VT(vt);

	if (debugout) TRACE_(olerelay)("Vt(%s%s)(",debugstr_vt(vttype),debugstr_vf(vttype));
	tdesc2.vt = vttype;
	if (writeit) {
	    hres = xbuf_add(buf,(LPBYTE)&vttype,sizeof(vttype));
	    if (hres) return hres;
	}
	/* need to recurse since we need to free the stuff */
	hres = serialize_param(tinfo,writeit,debugout,dealloc,&tdesc2,(DWORD*)&(V_I4(vt)),buf);
	if (debugout) TRACE_(olerelay)(")");
	return hres;
    }
    case VT_BSTR|VT_BYREF: {
	if (debugout) TRACE_(olerelay)("[byref]'%s'", *(BSTR*)*arg ? relaystr(*((BSTR*)*arg)) : "<bstr NULL>");
        if (writeit) {
            /* ptr to ptr to magic widestring, basically */
            BSTR *bstr = (BSTR *) *arg;
            DWORD len;
            if (!*bstr) {
                /* -1 means "null string" which is equivalent to empty string */
                len = -1;     
                hres = xbuf_add(buf, (LPBYTE)&len,sizeof(DWORD));
		if (hres) return hres;
            } else {
		len = *((DWORD*)*bstr-1)/sizeof(WCHAR);
		hres = xbuf_add(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) return hres;
		hres = xbuf_add(buf,(LPBYTE)*bstr,len * sizeof(WCHAR));
		if (hres) return hres;
            }
        }

        if (dealloc && arg) {
            BSTR *str = *((BSTR **)arg);
            SysFreeString(*str);
        }
        return S_OK;
    }
    
    case VT_BSTR: {
	if (debugout) {
	    if (*arg)
                   TRACE_(olerelay)("%s",relaystr((WCHAR*)*arg));
	    else
		    TRACE_(olerelay)("<bstr NULL>");
	}
	if (writeit) {
            BSTR bstr = (BSTR)*arg;
            DWORD len;
	    if (!bstr) {
		len = -1;
		hres = xbuf_add(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) return hres;
	    } else {
		len = *((DWORD*)bstr-1)/sizeof(WCHAR);
		hres = xbuf_add(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) return hres;
		hres = xbuf_add(buf,(LPBYTE)bstr,len * sizeof(WCHAR));
		if (hres) return hres;
	    }
	}

	if (dealloc && arg)
	    SysFreeString((BSTR)*arg);
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
	    case TKIND_ENUM:	/* confirmed */
	    case TKIND_RECORD:	/* FIXME: mostly untested */
		derefhere=TRUE;
		break;
	    case TKIND_ALIAS:	/* FIXME: untested */
	    case TKIND_DISPATCH:	/* will be done in VT_USERDEFINED case */
	    case TKIND_INTERFACE:	/* will be done in VT_USERDEFINED case */
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
	hres = serialize_param(tinfo,writeit,debugout,dealloc,tdesc->u.lptdesc,(DWORD*)*arg,buf);
	if (derefhere && dealloc) HeapFree(GetProcessHeap(),0,(LPVOID)*arg);
	return hres;
    }
    case VT_UNKNOWN:
	if (debugout) TRACE_(olerelay)("unk(0x%x)",*arg);
	if (writeit)
	    hres = _marshal_interface(buf,&IID_IUnknown,(LPUNKNOWN)*arg);
	if (dealloc && *(IUnknown **)arg)
	    IUnknown_Release((LPUNKNOWN)*arg);
	return hres;
    case VT_DISPATCH:
	if (debugout) TRACE_(olerelay)("idisp(0x%x)",*arg);
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
	case TKIND_RECORD: {
	    int i;
	    if (debugout) TRACE_(olerelay)("{");
	    for (i=0;i<tattr->cVars;i++) {
		VARDESC *vdesc;
		ELEMDESC *elem2;
		TYPEDESC *tdesc2;

		hres = ITypeInfo2_GetVarDesc(tinfo2, i, &vdesc);
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
		    (DWORD*)(((LPBYTE)arg)+vdesc->u.oInst),
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
	    if (debugout) TRACE_(olerelay)("%x",*arg);
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
	    hres = serialize_param(tinfo, writeit, debugout, dealloc, &adesc->tdescElem, (DWORD*)((LPBYTE)arg+i*_xsize(&adesc->tdescElem)), buf);
	    if (hres)
		return hres;
	    if (debugout && (i<arrsize-1)) TRACE_(olerelay)(",");
	}
	if (debugout) TRACE_(olerelay)("]");
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
    DWORD		*arg,
    marshal_state	*buf)
{
    HRESULT hres = S_OK;

    TRACE("vt %s at %p\n",debugstr_vt(tdesc->vt),arg);

    while (1) {
	switch (tdesc->vt) {
	case VT_EMPTY:
	    if (debugout) TRACE_(olerelay)("<empty>\n");
	    return S_OK;
	case VT_NULL:
	    if (debugout) TRACE_(olerelay)("<null>\n");
	    return S_OK;
	case VT_VARIANT: {
	    VARIANT	*vt = (VARIANT*)arg;

	    if (readit) {
		DWORD	vttype;
		TYPEDESC	tdesc2;
		hres = xbuf_get(buf,(LPBYTE)&vttype,sizeof(vttype));
		if (hres) {
		    FIXME("vt type not read?\n");
		    return hres;
		}
		memset(&tdesc2,0,sizeof(tdesc2));
		tdesc2.vt = vttype;
		V_VT(vt)  = vttype;
	        if (debugout) TRACE_(olerelay)("Vt(%s%s)(",debugstr_vt(vttype),debugstr_vf(vttype));
		hres = deserialize_param(tinfo, readit, debugout, alloc, &tdesc2, (DWORD*)&(V_I4(vt)), buf);
		TRACE_(olerelay)(")");
		return hres;
	    } else {
		VariantInit(vt);
		return S_OK;
	    }
	}
        case VT_I8:
        case VT_UI8:
        case VT_CY:
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)arg,8);
		if (hres) ERR("Failed to read integer 8 byte\n");
	    }
	    if (debugout) TRACE_(olerelay)("%x%x",arg[0],arg[1]);
	    return hres;
        case VT_ERROR:
	case VT_BOOL:
        case VT_I4:
        case VT_INT:
        case VT_UINT:
        case VT_R4:
        case VT_UI4:
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)arg,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
	    }
	    if (debugout) TRACE_(olerelay)("%x",*arg);
	    return hres;
        case VT_I2:
        case VT_UI2:
	    if (readit) {
		DWORD x;
		hres = xbuf_get(buf,(LPBYTE)&x,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
		memcpy(arg,&x,2);
	    }
	    if (debugout) TRACE_(olerelay)("%04x",*arg & 0xffff);
	    return hres;
        case VT_I1:
	case VT_UI1:
	    if (readit) {
		DWORD x;
		hres = xbuf_get(buf,(LPBYTE)&x,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
		memcpy(arg,&x,1);
	    }
	    if (debugout) TRACE_(olerelay)("%02x",*arg & 0xff);
	    return hres;
        case VT_I4|VT_BYREF:
	    hres = S_OK;
	    if (alloc)
		*arg = (DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DWORD));
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)*arg,sizeof(DWORD));
		if (hres) ERR("Failed to read integer 4 byte\n");
	    }
	    if (debugout) TRACE_(olerelay)("&0x%x",*(DWORD*)*arg);
	    return hres;
	case VT_BSTR|VT_BYREF: {
	    BSTR **bstr = (BSTR **)arg;
	    WCHAR	*str;
	    DWORD	len;

	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) {
		    ERR("failed to read bstr klen\n");
		    return hres;
		}
		if (len == -1) {
                    *bstr = CoTaskMemAlloc(sizeof(BSTR *));
		    **bstr = NULL;
		    if (debugout) TRACE_(olerelay)("<bstr NULL>");
		} else {
		    str  = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len+1)*sizeof(WCHAR));
		    hres = xbuf_get(buf,(LPBYTE)str,len*sizeof(WCHAR));
		    if (hres) {
			ERR("Failed to read BSTR.\n");
			return hres;
		    }
                    *bstr = CoTaskMemAlloc(sizeof(BSTR *));
		    **bstr = SysAllocStringLen(str,len);
		    if (debugout) TRACE_(olerelay)("%s",relaystr(str));
		    HeapFree(GetProcessHeap(),0,str);
		}
	    } else {
	        *bstr = NULL;
	    }
	    return S_OK;
	}
	case VT_BSTR: {
	    WCHAR	*str;
	    DWORD	len;

	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) {
		    ERR("failed to read bstr klen\n");
		    return hres;
		}
		if (len == -1) {
		    *arg = 0;
		    if (debugout) TRACE_(olerelay)("<bstr NULL>");
		} else {
		    str  = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len+1)*sizeof(WCHAR));
		    hres = xbuf_get(buf,(LPBYTE)str,len*sizeof(WCHAR));
		    if (hres) {
			ERR("Failed to read BSTR.\n");
			return hres;
		    }
		    *arg = (DWORD)SysAllocStringLen(str,len);
		    if (debugout) TRACE_(olerelay)("%s",relaystr(str));
		    HeapFree(GetProcessHeap(),0,str);
		}
	    } else {
	        *arg = 0;
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
		case TKIND_ENUM:	/* confirmed */
		case TKIND_RECORD:	/* FIXME: mostly untested */
		    derefhere=TRUE;
		    break;
		case TKIND_ALIAS:	/* FIXME: untested */
		case TKIND_DISPATCH:	/* will be done in VT_USERDEFINED case */
		case TKIND_INTERFACE:	/* will be done in VT_USERDEFINED case */
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
		    *arg=(DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_xsize(tdesc->u.lptdesc));
	    }
	    if (derefhere)
		return deserialize_param(tinfo, readit, debugout, alloc, tdesc->u.lptdesc, (LPDWORD)*arg, buf);
	    else
		return deserialize_param(tinfo, readit, debugout, alloc, tdesc->u.lptdesc, arg, buf);
        }
	case VT_UNKNOWN:
	    /* FIXME: UNKNOWN is unknown ..., but allocate 4 byte for it */
	    if (alloc)
	        *arg=(DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DWORD));
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
		case TKIND_RECORD: {
		    int i;

		    if (alloc)
			*arg = (DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,tattr->cbSizeInstance);

		    if (debugout) TRACE_(olerelay)("{");
		    for (i=0;i<tattr->cVars;i++) {
			VARDESC *vdesc;

			hres = ITypeInfo2_GetVarDesc(tinfo2, i, &vdesc);
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
			    (DWORD*)(((LPBYTE)*arg)+vdesc->u.oInst),
			    buf
			);
                        ITypeInfo2_ReleaseVarDesc(tinfo2, vdesc);
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
		    if (debugout) TRACE_(olerelay)("%x",*arg);
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
	    ARRAYDESC *adesc = tdesc->u.lpadesc;
	    int		arrsize,i;
	    arrsize = 1;
	    if (adesc->cDims > 1) FIXME("cDims > 1 in VT_CARRAY. Does it work?\n");
	    for (i=0;i<adesc->cDims;i++)
		arrsize *= adesc->rgbounds[i].cElements;
	    for (i=0;i<arrsize;i++)
		deserialize_param(
		    tinfo,
		    readit,
		    debugout,
		    alloc,
		    &adesc->tdescElem,
		    (DWORD*)((LPBYTE)(arg)+i*_xsize(&adesc->tdescElem)),
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

static DWORD
xCall(LPVOID retptr, int method, TMProxyImpl *tpinfo /*, args */)
{
    DWORD		*args = ((DWORD*)&tpinfo)+1, *xargs;
    const FUNCDESC	*fdesc;
    HRESULT		hres;
    int			i, relaydeb = TRACE_ON(olerelay);
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

    if (relaydeb) {
       TRACE_(olerelay)("->");
	if (iname)
	    TRACE_(olerelay)("%s:",relaystr(iname));
	if (fname)
	    TRACE_(olerelay)("%s(%d)",relaystr(fname),method);
	else
	    TRACE_(olerelay)("%d",method);
	TRACE_(olerelay)("(");
    }

    if (iname) SysFreeString(iname);
    if (fname) SysFreeString(fname);

    memset(&buf,0,sizeof(buf));

    /* normal typelib driven serializing */

    /* Need them for hack below */
    memset(names,0,sizeof(names));
    if (ITypeInfo_GetNames(tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames))
	nrofnames = 0;
    if (nrofnames > sizeof(names)/sizeof(names[0]))
	ERR("Need more names!\n");

    xargs = args;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	if (relaydeb) {
	    if (i) TRACE_(olerelay)(",");
	    if (i+1<nrofnames && names[i+1])
		TRACE_(olerelay)("%s=",relaystr(names[i+1]));
	}
	/* No need to marshal other data than FIN and any VT_PTR. */
	if (!is_in_elem(elem) && (elem->tdesc.vt != VT_PTR)) {
	    xargs+=_argsize(elem->tdesc.vt);
	    if (relaydeb) TRACE_(olerelay)("[out]");
	    continue;
	}
	hres = serialize_param(
	    tinfo,
	    is_in_elem(elem),
	    relaydeb,
	    FALSE,
	    &elem->tdesc,
	    xargs,
	    &buf
	);

	if (hres) {
	    ERR("Failed to serialize param, hres %x\n",hres);
	    break;
	}
	xargs+=_argsize(elem->tdesc.vt);
    }
    if (relaydeb) TRACE_(olerelay)(")");

    memset(&msg,0,sizeof(msg));
    msg.cbBuffer = buf.curoff;
    msg.iMethod  = method;
    hres = IRpcChannelBuffer_GetBuffer(chanbuf,&msg,&(tpinfo->iid));
    if (hres) {
	ERR("RpcChannelBuffer GetBuffer failed, %x\n",hres);
	goto exit;
    }
    memcpy(msg.Buffer,buf.base,buf.curoff);
    if (relaydeb) TRACE_(olerelay)("\n");
    hres = IRpcChannelBuffer_SendReceive(chanbuf,&msg,&status);
    if (hres) {
	ERR("RpcChannelBuffer SendReceive failed, %x\n",hres);
	goto exit;
    }

    if (relaydeb) TRACE_(olerelay)(" status = %08x (",status);
    if (buf.base)
	buf.base = HeapReAlloc(GetProcessHeap(),0,buf.base,msg.cbBuffer);
    else
	buf.base = HeapAlloc(GetProcessHeap(),0,msg.cbBuffer);
    buf.size = msg.cbBuffer;
    memcpy(buf.base,msg.Buffer,buf.size);
    buf.curoff = 0;

    /* generic deserializer using typelib description */
    xargs = args;
    status = S_OK;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;

	if (relaydeb) {
	    if (i) TRACE_(olerelay)(",");
	    if (i+1<nrofnames && names[i+1]) TRACE_(olerelay)("%s=",relaystr(names[i+1]));
	}
	/* No need to marshal other data than FOUT and any VT_PTR */
	if (!is_out_elem(elem) && (elem->tdesc.vt != VT_PTR)) {
	    xargs += _argsize(elem->tdesc.vt);
	    if (relaydeb) TRACE_(olerelay)("[in]");
	    continue;
	}
	hres = deserialize_param(
	    tinfo,
	    is_out_elem(elem),
	    relaydeb,
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
	xargs += _argsize(elem->tdesc.vt);
    }

    hres = xbuf_get(&buf, (LPBYTE)&remoteresult, sizeof(DWORD));
    if (hres != S_OK)
        goto exit;
    if (relaydeb) TRACE_(olerelay)(") = %08x\n", remoteresult);

    hres = remoteresult;

exit:
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
    const IRpcChannelBufferVtbl *lpVtbl;
    LONG                  refs;
    /* the IDispatch-derived interface we are handling */
	IID                   tmarshal_iid;
    IRpcChannelBuffer    *pDelegateChannel;
} TMarshalDispatchChannel;

static HRESULT WINAPI TMarshalDispatchChannel_QueryInterface(LPRPCCHANNELBUFFER iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown))
    {
        *ppv = (LPVOID)iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI TMarshalDispatchChannel_AddRef(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI TMarshalDispatchChannel_Release(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
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
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
    TRACE("(%p, %s)\n", olemsg, debugstr_guid(riid));
    /* Note: we are pretending to invoke a method on the interface identified
     * by tmarshal_iid so that we can re-use the IDispatch proxy/stub code
     * without the RPC runtime getting confused by not exporting an IDispatch interface */
    return IRpcChannelBuffer_GetBuffer(This->pDelegateChannel, olemsg, &This->tmarshal_iid);
}

static HRESULT WINAPI TMarshalDispatchChannel_SendReceive(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE *olemsg, ULONG *pstatus)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
    TRACE("(%p, %p)\n", olemsg, pstatus);
    return IRpcChannelBuffer_SendReceive(This->pDelegateChannel, olemsg, pstatus);
}

static HRESULT WINAPI TMarshalDispatchChannel_FreeBuffer(LPRPCCHANNELBUFFER iface, RPCOLEMESSAGE* olemsg)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
    TRACE("(%p)\n", olemsg);
    return IRpcChannelBuffer_FreeBuffer(This->pDelegateChannel, olemsg);
}

static HRESULT WINAPI TMarshalDispatchChannel_GetDestCtx(LPRPCCHANNELBUFFER iface, DWORD* pdwDestContext, void** ppvDestContext)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
    TRACE("(%p,%p)\n", pdwDestContext, ppvDestContext);
    return IRpcChannelBuffer_GetDestCtx(This->pDelegateChannel, pdwDestContext, ppvDestContext);
}

static HRESULT WINAPI TMarshalDispatchChannel_IsConnected(LPRPCCHANNELBUFFER iface)
{
    TMarshalDispatchChannel *This = (TMarshalDispatchChannel *)iface;
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

    This->lpVtbl = &TMarshalDispatchChannelVtbl;
    This->refs = 1;
    IRpcChannelBuffer_AddRef(pDelegateChannel);
    This->pDelegateChannel = pDelegateChannel;
    This->tmarshal_iid = *tmarshal_riid;

    *ppChannel = (IRpcChannelBuffer *)&This->lpVtbl;
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
    /* nrofargs without This */
    int nrofargs;
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
    nrofargs = 0;
    for (j=0;j<fdesc->cParams;j++)
        nrofargs += _argsize(fdesc->lprgelemdescParam[j].tdesc.vt);

#ifdef __i386__
    if (fdesc->callconv != CC_STDCALL) {
        ERR("calling convention is not stdcall????\n");
        return E_FAIL;
    }
/* popl %eax	-	return ptr
 * pushl <nr>
 * pushl %eax
 * call xCall
 * lret <nr> (+4)
 *
 *
 * arg3 arg2 arg1 <method> <returnptr>
 */
    xasm->popleax       = 0x58;
    xasm->pushlval      = 0x6a;
    xasm->nr            = num;
    xasm->pushleax      = 0x50;
    xasm->lcall         = 0xe8; /* relative jump */
    xasm->xcall         = (DWORD)xCall;
    xasm->xcall        -= (DWORD)&(xasm->lret);
    xasm->lret          = 0xc2;
    xasm->bytestopop    = (nrofargs+2)*4; /* pop args, This, iMethod */
    proxy->lpvtbl[num]  = xasm;
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
    unsigned int i, nroffuncs;
    TMProxyImpl	*proxy;
    TYPEATTR	*typeattr;
    BOOL        defer_to_dispatch = FALSE;

    TRACE("(...%s...)\n",debugstr_guid(riid));
    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	ERR("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }

    hres = num_of_funcs(tinfo, &nroffuncs);
    if (FAILED(hres)) {
        ERR("Cannot get number of functions for typeinfo %s\n",debugstr_guid(riid));
        ITypeInfo_Release(tinfo);
        return hres;
    }

    proxy = CoTaskMemAlloc(sizeof(TMProxyImpl));
    if (!proxy) return E_OUTOFMEMORY;

    assert(sizeof(TMAsmProxy) == 12);

    proxy->dispatch = NULL;
    proxy->dispatch_proxy = NULL;
    proxy->outerunknown = pUnkOuter;
    proxy->asmstubs = VirtualAlloc(NULL, sizeof(TMAsmProxy) * nroffuncs, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!proxy->asmstubs) {
        ERR("Could not commit pages for proxy thunks\n");
        CoTaskMemFree(proxy);
        return E_OUTOFMEMORY;
    }
    proxy->lpvtbl2	= &tmproxyvtable;
    /* one reference for the proxy */
    proxy->ref		= 1;
    proxy->tinfo	= tinfo;
    memcpy(&proxy->iid,riid,sizeof(*riid));
    proxy->chanbuf      = 0;

    InitializeCriticalSection(&proxy->crit);
    proxy->crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TMProxyImpl.crit");

    proxy->lpvtbl = HeapAlloc(GetProcessHeap(),0,sizeof(LPBYTE)*nroffuncs);

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
                if(!defer_to_dispatch)
                {
                    hres = init_proxy_entry_point(proxy, i);
                    if(FAILED(hres)) return hres;
                }
                else proxy->lpvtbl[3] = ProxyIDispatch_GetTypeInfoCount;
                break;
        case 4:
                if(!defer_to_dispatch)
                {
                    hres = init_proxy_entry_point(proxy, i);
                    if(FAILED(hres)) return hres;
                }
                else proxy->lpvtbl[4] = ProxyIDispatch_GetTypeInfo;
                break;
        case 5:
                if(!defer_to_dispatch)
                {
                    hres = init_proxy_entry_point(proxy, i);
                    if(FAILED(hres)) return hres;
                }
                else proxy->lpvtbl[5] = ProxyIDispatch_GetIDsOfNames;
                break;
        case 6:
                if(!defer_to_dispatch)
                {
                    hres = init_proxy_entry_point(proxy, i);
                    if(FAILED(hres)) return hres;
                }
                else proxy->lpvtbl[6] = ProxyIDispatch_Invoke;
                break;
	default:
                hres = init_proxy_entry_point(proxy, i);
                if(FAILED(hres)) return hres;
	}
    }

    if (hres == S_OK)
    {
        *ppv		= (LPVOID)proxy;
        *ppProxy		= (IRpcProxyBuffer *)&(proxy->lpvtbl2);
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    else
        TMProxyImpl_Release((IRpcProxyBuffer *)&proxy->lpvtbl2);
    return hres;
}

typedef struct _TMStubImpl {
    const IRpcStubBufferVtbl   *lpvtbl;
    LONG			ref;

    LPUNKNOWN			pUnk;
    ITypeInfo			*tinfo;
    IID				iid;
    IRpcStubBuffer		*dispatch_stub;
    BOOL			dispatch_derivative;
} TMStubImpl;

static HRESULT WINAPI
TMStubImpl_QueryInterface(LPRPCSTUBBUFFER iface, REFIID riid, LPVOID *ppv)
{
    if (IsEqualIID(riid,&IID_IRpcStubBuffer)||IsEqualIID(riid,&IID_IUnknown)){
	*ppv = (LPVOID)iface;
	IRpcStubBuffer_AddRef(iface);
	return S_OK;
    }
    FIXME("%s, not supported IID.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
TMStubImpl_AddRef(LPRPCSTUBBUFFER iface)
{
    TMStubImpl *This = (TMStubImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);
        
    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI
TMStubImpl_Release(LPRPCSTUBBUFFER iface)
{
    TMStubImpl *This = (TMStubImpl *)iface;
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
    TMStubImpl *This = (TMStubImpl *)iface;

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
    TMStubImpl *This = (TMStubImpl *)iface;

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
    int		i;
    const FUNCDESC *fdesc;
    TMStubImpl *This = (TMStubImpl *)iface;
    HRESULT	hres;
    DWORD	*args = NULL, res, *xargs, nrofargs;
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

    if (iname) SysFreeString (iname);

    /* Need them for hack below */
    memset(names,0,sizeof(names));
    ITypeInfo_GetNames(tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames);
    if (nrofnames > sizeof(names)/sizeof(names[0])) {
	ERR("Need more names!\n");
    }

    /*dump_FUNCDESC(fdesc);*/
    nrofargs = 0;
    for (i=0;i<fdesc->cParams;i++)
	nrofargs += _argsize(fdesc->lprgelemdescParam[i].tdesc.vt);
    args = HeapAlloc(GetProcessHeap(),0,(nrofargs+1)*sizeof(DWORD));
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
	xargs += _argsize(elem->tdesc.vt);
	if (hres) {
	    ERR("Failed to deserialize param %s, hres %x\n",relaystr(names[i+1]),hres);
	    break;
	}
    }

    args[0] = (DWORD)This->pUnk;

    __TRY
    {
        res = _invoke(
            (*((FARPROC**)args[0]))[fdesc->oVft/4],
            fdesc->callconv,
            (xargs-args),
            args
        );
    }
    __EXCEPT(NULL)
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
	xargs += _argsize(elem->tdesc.vt);
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
}

static LPRPCSTUBBUFFER WINAPI
TMStubImpl_IsIIDSupported(LPRPCSTUBBUFFER iface, REFIID riid) {
    FIXME("Huh (%s)?\n",debugstr_guid(riid));
    return NULL;
}

static ULONG WINAPI
TMStubImpl_CountRefs(LPRPCSTUBBUFFER iface) {
    TMStubImpl *This = (TMStubImpl *)iface;

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

    TRACE("(%s,%p,%p)\n",debugstr_guid(riid),pUnkServer,ppStub);

    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	ERR("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }

    stub = CoTaskMemAlloc(sizeof(TMStubImpl));
    if (!stub)
	return E_OUTOFMEMORY;
    stub->lpvtbl	= &tmstubvtbl;
    stub->ref		= 1;
    stub->tinfo		= tinfo;
    stub->dispatch_stub = NULL;
    stub->dispatch_derivative = FALSE;
    memcpy(&(stub->iid),riid,sizeof(*riid));
    hres = IRpcStubBuffer_Connect((LPRPCSTUBBUFFER)stub,pUnkServer);
    *ppStub 		= (LPRPCSTUBBUFFER)stub;
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

    return hres;
}

static const IPSFactoryBufferVtbl psfacbufvtbl = {
    PSFacBuf_QueryInterface,
    PSFacBuf_AddRef,
    PSFacBuf_Release,
    PSFacBuf_CreateProxy,
    PSFacBuf_CreateStub
};

/* This is the whole PSFactoryBuffer object, just the vtableptr */
static const IPSFactoryBufferVtbl *lppsfac = &psfacbufvtbl;

/***********************************************************************
 *           TMARSHAL_DllGetClassObject
 */
HRESULT TMARSHAL_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    if (IsEqualIID(iid,&IID_IPSFactoryBuffer)) {
	*ppv = &lppsfac;
	return S_OK;
    }
    return E_NOINTERFACE;
}
