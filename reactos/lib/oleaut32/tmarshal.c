/*
 *	TYPELIB Marshaler
 *
 *	Copyright 2002	Marcus Meissner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"

#include "ole2.h"
#include "wine/unicode.h"
#include "ole2disp.h"
#include "typelib.h"
#include "wine/debug.h"
#include "winternl.h"

static const WCHAR riidW[5] = {'r','i','i','d',0};
static const WCHAR pdispparamsW[] = {'p','d','i','s','p','p','a','r','a','m','s',0};
static const WCHAR ppvObjectW[] = {'p','p','v','O','b','j','e','c','t',0};

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(olerelay);

typedef struct _marshal_state {
    LPBYTE	base;
    int		size;
    int		curoff;

    BOOL	thisisiid;
    IID		iid;	/* HACK: for VT_VOID */
} marshal_state;

static HRESULT
xbuf_add(marshal_state *buf, LPBYTE stuff, DWORD size) {
    while (buf->size - buf->curoff < size) {
	if (buf->base) {
	    buf->size += 100;
	    buf->base = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,buf->base,buf->size);
	    if (!buf->base)
		return E_OUTOFMEMORY;
	} else {
	    buf->base = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,32);
	    buf->size = 32;
	    if (!buf->base)
		return E_OUTOFMEMORY;
	}
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
    if (hres) return hres;
    if (xsize == 0) return S_OK;
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) {
	FIXME("Stream create failed %lx\n",hres);
	return hres;
    }
    hres = IStream_Write(pStm,buf->base+buf->curoff,xsize,&res);
    if (hres) { FIXME("stream write %lx\n",hres); return hres; }
    memset(&seekto,0,sizeof(seekto));
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    if (hres) { FIXME("Failed Seek %lx\n",hres); return hres;}
    hres = CoUnmarshalInterface(pStm,riid,(LPVOID*)pUnk);
    if (hres) {
	FIXME("Marshalling interface %s failed with %lx\n",debugstr_guid(riid),hres);
	return hres;
    }
    IStream_Release(pStm);
    return xbuf_skip(buf,xsize);
}

static HRESULT
_marshal_interface(marshal_state *buf, REFIID riid, LPUNKNOWN pUnk) {
    LPUNKNOWN		newiface;
    LPBYTE		tempbuf;
    IStream		*pStm;
    STATSTG		ststg;
    ULARGE_INTEGER	newpos;
    LARGE_INTEGER	seekto;
    ULONG		res;
    DWORD		xsize;
    HRESULT		hres;

    hres = S_OK;
    if (!pUnk)
	goto fail;

    TRACE("...%s...\n",debugstr_guid(riid));
    hres=IUnknown_QueryInterface(pUnk,riid,(LPVOID*)&newiface);
    if (hres) {
	TRACE("%p does not support iface %s\n",pUnk,debugstr_guid(riid));
	goto fail;
    }
    hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
    if (hres) {
	FIXME("Stream create failed %lx\n",hres);
	goto fail;
    }
    hres = CoMarshalInterface(pStm,riid,newiface,0,NULL,0);
    IUnknown_Release(newiface);
    if (hres) {
	FIXME("Marshalling interface %s failed with %lx\n",
		debugstr_guid(riid),hres
	);
	goto fail;
    }
    hres = IStream_Stat(pStm,&ststg,0);
    tempbuf = HeapAlloc(GetProcessHeap(), 0, ststg.cbSize.u.LowPart);
    memset(&seekto,0,sizeof(seekto));
    hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
    if (hres) { FIXME("Failed Seek %lx\n",hres); goto fail;}
    hres = IStream_Read(pStm,tempbuf,ststg.cbSize.u.LowPart,&res);
    if (hres) { FIXME("Failed Read %lx\n",hres); goto fail;}
    IStream_Release(pStm);
    xsize = ststg.cbSize.u.LowPart;
    xbuf_add(buf,(LPBYTE)&xsize,sizeof(xsize));
    hres = xbuf_add(buf,tempbuf,ststg.cbSize.u.LowPart);
    HeapFree(GetProcessHeap(),0,tempbuf);
    return hres;
fail:
    xsize = 0;
    xbuf_add(buf,(LPBYTE)&xsize,sizeof(xsize));
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
    DWORD	tlguidlen, verlen, type, tlfnlen;
    ITypeLib	*tl;

    sprintf( interfacekey, "Interface\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\Typelib",
	riid->Data1, riid->Data2, riid->Data3,
	riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
	riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]
    );

    if (RegOpenKeyA(HKEY_CLASSES_ROOT,interfacekey,&ikey)) {
	FIXME("No %s key found.\n",interfacekey);
       	return E_FAIL;
    }
    type = (1<<REG_SZ);
    tlguidlen = sizeof(tlguid);
    if (RegQueryValueExA(ikey,NULL,NULL,&type,tlguid,&tlguidlen)) {
	FIXME("Getting typelib guid failed.\n");
	RegCloseKey(ikey);
	return E_FAIL;
    }
    type = (1<<REG_SZ);
    verlen = sizeof(ver);
    if (RegQueryValueExA(ikey,"Version",NULL,&type,ver,&verlen)) {
	FIXME("Could not get version value?\n");
	RegCloseKey(ikey);
	return E_FAIL;
    }
    RegCloseKey(ikey);
    sprintf(typelibkey,"Typelib\\%s\\%s\\0\\win32",tlguid,ver);
    tlfnlen = sizeof(tlfn);
    if (RegQueryValueA(HKEY_CLASSES_ROOT,typelibkey,tlfn,&tlfnlen)) {
	FIXME("Could not get typelib fn?\n");
	return E_FAIL;
    }
    MultiByteToWideChar(CP_ACP, 0, tlfn, -1, tlfnW, -1);
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
    /* FIXME: do this?  ITypeLib_Release(tl); */
    return hres;
}

/* Determine nr of functions. Since we use the toplevel interface and all
 * inherited ones have lower numbers, we are ok to not to descent into
 * the inheritance tree I think.
 */
static int _nroffuncs(ITypeInfo *tinfo) {
    int 	n, max = 0;
    FUNCDESC	*fdesc;
    HRESULT	hres;

    n=0;
    while (1) {
	hres = ITypeInfo_GetFuncDesc(tinfo,n,&fdesc);
	if (hres)
	    return max+1;
	if (fdesc->oVft/4 > max)
	    max = fdesc->oVft/4;
	n++;
    }
    /*NOTREACHED*/
}

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

typedef struct _TMProxyImpl {
    DWORD				*lpvtbl;
    ICOM_VTABLE(IRpcProxyBuffer)	*lpvtbl2;
    DWORD				ref;

    TMAsmProxy				*asmstubs;
    ITypeInfo*				tinfo;
    IRpcChannelBuffer*			chanbuf;
    IID					iid;
} TMProxyImpl;

static HRESULT WINAPI
TMProxyImpl_QueryInterface(LPRPCPROXYBUFFER iface, REFIID riid, LPVOID *ppv) {
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
TMProxyImpl_AddRef(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);

    TRACE("()\n");
    This->ref++;
    return This->ref;
}

static ULONG WINAPI
TMProxyImpl_Release(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);

    TRACE("()\n");
    This->ref--;
    if (This->ref) return This->ref;
    if (This->chanbuf) IRpcChannelBuffer_Release(This->chanbuf);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI
TMProxyImpl_Connect(
    LPRPCPROXYBUFFER iface,IRpcChannelBuffer* pRpcChannelBuffer
) {
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);

    TRACE("(%p)\n",pRpcChannelBuffer);
    This->chanbuf = pRpcChannelBuffer;
    IRpcChannelBuffer_AddRef(This->chanbuf);
    return S_OK;
}

static void WINAPI
TMProxyImpl_Disconnect(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(TMProxyImpl,lpvtbl2,iface);

    FIXME("()\n");
    IRpcChannelBuffer_Release(This->chanbuf);
    This->chanbuf = NULL;
}


static ICOM_VTABLE(IRpcProxyBuffer) tmproxyvtable = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    TMProxyImpl_QueryInterface,
    TMProxyImpl_AddRef,
    TMProxyImpl_Release,
    TMProxyImpl_Connect,
    TMProxyImpl_Disconnect
};

/* how much space do we use on stack in DWORD steps. */
int const
_argsize(DWORD vt) {
    switch (vt) {
    case VT_DATE:
	return sizeof(DATE)/sizeof(DWORD);
    case VT_VARIANT:
	return (sizeof(VARIANT)+3)/sizeof(DWORD);
    default:
	return 1;
    }
}

static int
_xsize(TYPEDESC *td) {
    switch (td->vt) {
    case VT_DATE:
	return sizeof(DATE);
    case VT_VARIANT:
	return sizeof(VARIANT)+3;
    case VT_CARRAY: {
	int i, arrsize = 1;
	ARRAYDESC *adesc = td->u.lpadesc;

	for (i=0;i<adesc->cDims;i++)
	    arrsize *= adesc->rgbounds[i].cElements;
	return arrsize*_xsize(&adesc->tdescElem);
    }
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
    marshal_state	*buf
) {
    HRESULT hres = S_OK;

    TRACE("(tdesc.vt %d)\n",tdesc->vt);

    switch (tdesc->vt) {
    case VT_EMPTY: /* nothing. empty variant for instance */
	return S_OK;
    case VT_BOOL:
    case VT_ERROR:
    case VT_UI4:
    case VT_UINT:
    case VT_I4:
    case VT_R4:
    case VT_UI2:
    case VT_UI1:
	hres = S_OK;
	if (debugout) MESSAGE("%lx",*arg);
	if (writeit)
	    hres = xbuf_add(buf,(LPBYTE)arg,sizeof(DWORD));
	return hres;
    case VT_VARIANT: {
	TYPEDESC	tdesc2;
	VARIANT		*vt = (VARIANT*)arg;
	DWORD		vttype = V_VT(vt);

	if (debugout) MESSAGE("Vt(%ld)(",vttype);
	tdesc2.vt = vttype;
	if (writeit) {
	    hres = xbuf_add(buf,(LPBYTE)&vttype,sizeof(vttype));
	    if (hres) return hres;
	}
	/* need to recurse since we need to free the stuff */
	hres = serialize_param(tinfo,writeit,debugout,dealloc,&tdesc2,&(V_I4(vt)),buf);
	if (debugout) MESSAGE(")");
	return hres;
    }
    case VT_BSTR: {
	if (debugout) {
	    if (arg)
		    MESSAGE("%s",debugstr_w((BSTR)*arg));
	    else
		    MESSAGE("<bstr NULL>");
	}
	if (writeit) {
	    if (!*arg) {
		DWORD fakelen = -1;
		hres = xbuf_add(buf,(LPBYTE)&fakelen,4);
		if (hres)
		    return hres;
	    } else {
		DWORD *bstr = ((DWORD*)(*arg))-1;

		hres = xbuf_add(buf,(LPBYTE)bstr,bstr[0]+4);
		if (hres)
		    return hres;
	    }
	}

	if (dealloc && arg)
	    SysFreeString((BSTR)*arg);
	return S_OK;
    }
    case VT_PTR: {
	DWORD cookie;

	if (debugout) MESSAGE("*");
	if (writeit) {
	    cookie = *arg ? 0x42424242 : 0;
	    hres = xbuf_add(buf,(LPBYTE)&cookie,sizeof(cookie));
	    if (hres)
		return hres;
	}
	if (!*arg) {
	    if (debugout) MESSAGE("NULL");
	    return S_OK;
	}
	hres = serialize_param(tinfo,writeit,debugout,dealloc,tdesc->u.lptdesc,(DWORD*)*arg,buf);
	if (dealloc) HeapFree(GetProcessHeap(),0,(LPVOID)arg);
	return hres;
    }
    case VT_UNKNOWN:
	if (debugout) MESSAGE("unk(0x%lx)",*arg);
	if (writeit)
	    hres = _marshal_interface(buf,&IID_IUnknown,(LPUNKNOWN)*arg);
	return hres;
    case VT_DISPATCH:
	if (debugout) MESSAGE("idisp(0x%lx)",*arg);
	if (writeit)
	    hres = _marshal_interface(buf,&IID_IDispatch,(LPUNKNOWN)*arg);
	return hres;
    case VT_VOID:
	if (debugout) MESSAGE("<void>");
	return S_OK;
    case VT_USERDEFINED: {
	ITypeInfo	*tinfo2;
	TYPEATTR	*tattr;

	hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
	if (hres) {
	    FIXME("Could not get typeinfo of hreftype %lx for VT_USERDEFINED.\n",tdesc->u.hreftype);
	    return hres;
	}
	ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	switch (tattr->typekind) {
	case TKIND_DISPATCH:
	case TKIND_INTERFACE:
	    if (writeit)
	       hres=_marshal_interface(buf,&(tattr->guid),(LPUNKNOWN)arg);
	    break;
	case TKIND_RECORD: {
	    int i;
	    if (debugout) MESSAGE("{");
	    for (i=0;i<tattr->cVars;i++) {
		VARDESC *vdesc;
		ELEMDESC *elem2;
		TYPEDESC *tdesc2;

		hres = ITypeInfo2_GetVarDesc(tinfo2, i, &vdesc);
		if (hres) {
		    FIXME("Could not get vardesc of %d\n",i);
		    return hres;
		}
		/* Need them for hack below */
		/*
		memset(names,0,sizeof(names));
		hres = ITypeInfo_GetNames(tinfo2,vdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames);
		if (nrofnames > sizeof(names)/sizeof(names[0])) {
		    ERR("Need more names!\n");
		}
		if (!hres && debugout)
		    MESSAGE("%s=",debugstr_w(names[0]));
		*/
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
		if (hres!=S_OK)
		    return hres;
		if (debugout && (i<(tattr->cVars-1)))
		    MESSAGE(",");
	    }
	    if (buf->thisisiid && (tattr->cbSizeInstance==sizeof(GUID)))
		memcpy(&(buf->iid),arg,sizeof(buf->iid));
	    if (debugout) MESSAGE("}");
	    break;
	}
	default:
	    FIXME("Unhandled typekind %d\n",tattr->typekind);
	    hres = E_FAIL;
	    break;
	}
	ITypeInfo_Release(tinfo2);
	return hres;
    }
    case VT_CARRAY: {
	ARRAYDESC *adesc = tdesc->u.lpadesc;
	int i, arrsize = 1;

	if (debugout) MESSAGE("carr");
	for (i=0;i<adesc->cDims;i++) {
	    if (debugout) MESSAGE("[%ld]",adesc->rgbounds[i].cElements);
	    arrsize *= adesc->rgbounds[i].cElements;
	}
	if (debugout) MESSAGE("[");
	for (i=0;i<arrsize;i++) {
	    hres = serialize_param(tinfo, writeit, debugout, dealloc, &adesc->tdescElem, (DWORD*)((LPBYTE)arg+i*_xsize(&adesc->tdescElem)), buf);
	    if (hres)
		return hres;
	    if (debugout && (i<arrsize-1)) MESSAGE(",");
	}
	if (debugout) MESSAGE("]");
	return S_OK;
    }
    default:
	ERR("Unhandled marshal type %d.\n",tdesc->vt);
	return S_OK;
    }
}

static HRESULT
serialize_LPVOID_ptr(
    ITypeInfo		*tinfo,
    BOOL		writeit,
    BOOL		debugout,
    BOOL		dealloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf
) {
    HRESULT	hres;
    DWORD	cookie;

    if ((tdesc->vt != VT_PTR)			||
	(tdesc->u.lptdesc->vt != VT_PTR)	||
	(tdesc->u.lptdesc->u.lptdesc->vt != VT_VOID)
    ) {
	FIXME("ppvObject not expressed as VT_PTR -> VT_PTR -> VT_VOID?\n");
	return E_FAIL;
    }
    cookie = (*arg) ? 0x42424242: 0x0;
    if (writeit) {
	hres = xbuf_add(buf, (LPVOID)&cookie, sizeof(cookie));
	if (hres)
	    return hres;
    }
    if (!*arg) {
	if (debugout) MESSAGE("<lpvoid NULL>");
	return S_OK;
    }
    if (debugout)
	MESSAGE("ppv(%p)",*(LPUNKNOWN*)*arg);
    if (writeit) {
	hres = _marshal_interface(buf,&(buf->iid),*(LPUNKNOWN*)*arg);
	if (hres)
	    return hres;
    }
    if (dealloc)
	HeapFree(GetProcessHeap(),0,(LPVOID)*arg);
    return S_OK;
}

static HRESULT
serialize_DISPPARAM_ptr(
    ITypeInfo		*tinfo,
    BOOL		writeit,
    BOOL		debugout,
    BOOL		dealloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf
) {
    DWORD	cookie;
    HRESULT	hres;
    DISPPARAMS	*disp;
    int		i;

    if ((tdesc->vt != VT_PTR) || (tdesc->u.lptdesc->vt != VT_USERDEFINED)) {
	FIXME("DISPPARAMS not expressed as VT_PTR -> VT_USERDEFINED?\n");
	return E_FAIL;
    }

    cookie = *arg ? 0x42424242 : 0x0;
    if (writeit) {
	hres = xbuf_add(buf,(LPBYTE)&cookie,sizeof(cookie));
	if (hres)
	    return hres;
    }
    if (!*arg) {
	if (debugout) MESSAGE("<DISPPARAMS NULL>");
	return S_OK;
    }
    disp = (DISPPARAMS*)*arg;
    if (writeit) {
	hres = xbuf_add(buf,(LPBYTE)&disp->cArgs,sizeof(disp->cArgs));
	if (hres)
	    return hres;
    }
    if (debugout) MESSAGE("D{");
    for (i=0;i<disp->cArgs;i++) {
	TYPEDESC	vtdesc;

	vtdesc.vt = VT_VARIANT;
	serialize_param(
	    tinfo,
	    writeit,
	    debugout,
	    dealloc,
	    &vtdesc,
	    (DWORD*)(disp->rgvarg+i),
	    buf
        );
	if (debugout && (i<disp->cArgs-1))
	    MESSAGE(",");
    }
    if (dealloc)
	HeapFree(GetProcessHeap(),0,disp->rgvarg);
    if (writeit) {
	hres = xbuf_add(buf,(LPBYTE)&disp->cNamedArgs,sizeof(disp->cNamedArgs));
	if (hres)
	    return hres;
    }
    if (debugout) MESSAGE("}{");
    for (i=0;i<disp->cNamedArgs;i++) {
	TYPEDESC	vtdesc;

	vtdesc.vt = VT_UINT;
	serialize_param(
	    tinfo,
	    writeit,
	    debugout,
	    dealloc,
	    &vtdesc,
	    (DWORD*)(disp->rgdispidNamedArgs+i),
	    buf
	);
	if (debugout && (i<disp->cNamedArgs-1))
	    MESSAGE(",");
    }
    if (debugout) MESSAGE("}");
    if (dealloc) {
	HeapFree(GetProcessHeap(),0,disp->rgdispidNamedArgs);
	HeapFree(GetProcessHeap(),0,disp);
    }
    return S_OK;
}

static HRESULT
deserialize_param(
    ITypeInfo		*tinfo,
    BOOL		readit,
    BOOL		debugout,
    BOOL		alloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf
) {
    HRESULT hres = S_OK;

    TRACE("vt %d at %p\n",tdesc->vt,arg);

    while (1) {
	switch (tdesc->vt) {
	case VT_EMPTY:
	    if (debugout) MESSAGE("<empty>");
	    return S_OK;
	case VT_NULL:
	    if (debugout) MESSAGE("<null>");
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
	        if (debugout) MESSAGE("Vt(%ld)(",vttype);
		hres = deserialize_param(tinfo, readit, debugout, alloc, &tdesc2, &(V_I4(vt)), buf);
		MESSAGE(")");
		return hres;
	    } else {
		VariantInit(vt);
		return S_OK;
	    }
	}
        case VT_ERROR:
	case VT_BOOL: case VT_I4: case VT_UI4: case VT_UINT: case VT_R4:
        case VT_UI2:
	case VT_UI1:
	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)arg,sizeof(DWORD));
		if (hres) FIXME("Failed to read integer 4 byte\n");
	    }
	    if (debugout) MESSAGE("%lx",*arg);
	    return hres;
	case VT_BSTR: {
	    WCHAR	*str;
	    DWORD	len;

	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)&len,sizeof(DWORD));
		if (hres) {
		    FIXME("failed to read bstr klen\n");
		    return hres;
		}
		if (len == -1) {
		    *arg = 0;
		    if (debugout) MESSAGE("<bstr NULL>");
		} else {
		    str  = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,len+sizeof(WCHAR));
		    hres = xbuf_get(buf,(LPBYTE)str,len);
		    if (hres) {
			FIXME("Failed to read BSTR.\n");
			return hres;
		    }
		    *arg = (DWORD)SysAllocStringLen(str,len);
		    if (debugout) MESSAGE("%s",debugstr_w(str));
		    HeapFree(GetProcessHeap(),0,str);
		}
	    } else {
	        *arg = 0;
	    }
	    return S_OK;
	}
	case VT_PTR: {
	    DWORD	cookie;
	    BOOL	derefhere = 0;

	    derefhere = (tdesc->u.lptdesc->vt != VT_USERDEFINED);

	    if (readit) {
		hres = xbuf_get(buf,(LPBYTE)&cookie,sizeof(cookie));
		if (hres) {
		    FIXME("Failed to load pointer cookie.\n");
		    return hres;
		}
		if (cookie != 0x42424242) {
		    if (debugout) MESSAGE("NULL");
		    *arg = 0;
		    return S_OK;
		}
		if (debugout) MESSAGE("*");
	    }
	    if (alloc) {
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
		MESSAGE("unk(%p)",arg);
	    return hres;
	case VT_DISPATCH:
	    hres = S_OK;
	    if (readit)
		hres = _unmarshal_interface(buf,&IID_IDispatch,(LPUNKNOWN*)arg);
	    if (debugout)
		MESSAGE("idisp(%p)",arg);
	    return hres;
	case VT_VOID:
	    if (debugout) MESSAGE("<void>");
	    return S_OK;
	case VT_USERDEFINED: {
	    ITypeInfo	*tinfo2;
	    TYPEATTR	*tattr;

	    hres = ITypeInfo_GetRefTypeInfo(tinfo,tdesc->u.hreftype,&tinfo2);
	    if (hres) {
		FIXME("Could not get typeinfo of hreftype %lx for VT_USERDEFINED.\n",tdesc->u.hreftype);
		return hres;
	    }
	    hres = ITypeInfo_GetTypeAttr(tinfo2,&tattr);
	    if (hres) {
		FIXME("Could not get typeattr in VT_USERDEFINED.\n");
	    } else {
		if (alloc)
		    *arg = (DWORD)HeapAlloc(GetProcessHeap(),0,tattr->cbSizeInstance);
		switch (tattr->typekind) {
		case TKIND_DISPATCH:
		case TKIND_INTERFACE:
		    if (readit)
			hres = _unmarshal_interface(buf,&(tattr->guid),(LPUNKNOWN*)arg);
		    break;
		case TKIND_RECORD: {
		    int i;

		    if (debugout) MESSAGE("{");
		    for (i=0;i<tattr->cVars;i++) {
			VARDESC *vdesc;

			hres = ITypeInfo2_GetVarDesc(tinfo2, i, &vdesc);
			if (hres) {
			    FIXME("Could not get vardesc of %d\n",i);
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
		        if (debugout && (i<tattr->cVars-1)) MESSAGE(",");
		    }
		    if (buf->thisisiid && (tattr->cbSizeInstance==sizeof(GUID)))
			memcpy(&(buf->iid),(LPBYTE)*arg,sizeof(buf->iid));
		    if (debugout) MESSAGE("}");
		    break;
		}
		default:
		    ERR("Unhandled typekind %d\n",tattr->typekind);
		    hres = E_FAIL;
		    break;
		}
	    }
	    if (hres)
		FIXME("failed to stuballoc in TKIND_RECORD.\n");
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
	default:
	    ERR("No handler for VT type %d!\n",tdesc->vt);
	    return S_OK;
	}
    }
}

static HRESULT
deserialize_LPVOID_ptr(
    ITypeInfo		*tinfo,
    BOOL		readit,
    BOOL		debugout,
    BOOL		alloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf
) {
    HRESULT	hres;
    DWORD	cookie;

    if ((tdesc->vt != VT_PTR)			||
	(tdesc->u.lptdesc->vt != VT_PTR)	||
	(tdesc->u.lptdesc->u.lptdesc->vt != VT_VOID)
    ) {
	FIXME("ppvObject not expressed as VT_PTR -> VT_PTR -> VT_VOID?\n");
	return E_FAIL;
    }
    if (alloc)
	*arg=(DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(LPVOID));
    if (readit) {
	hres = xbuf_get(buf, (LPVOID)&cookie, sizeof(cookie));
	if (hres)
	    return hres;
	if (cookie != 0x42424242) {
	    *(DWORD*)*arg = 0;
	    if (debugout) MESSAGE("<lpvoid NULL>");
	    return S_OK;
	}
    }
    if (readit) {
	hres = _unmarshal_interface(buf,&buf->iid,(LPUNKNOWN*)*arg);
	if (hres)
	    return hres;
    }
    if (debugout) MESSAGE("ppv(%p)",(LPVOID)*arg);
    return S_OK;
}

static HRESULT
deserialize_DISPPARAM_ptr(
    ITypeInfo		*tinfo,
    BOOL		readit,
    BOOL		debugout,
    BOOL		alloc,
    TYPEDESC		*tdesc,
    DWORD		*arg,
    marshal_state	*buf
) {
    DWORD	cookie;
    DISPPARAMS	*disps;
    HRESULT	hres;
    int		i;

    if ((tdesc->vt != VT_PTR) || (tdesc->u.lptdesc->vt != VT_USERDEFINED)) {
	FIXME("DISPPARAMS not expressed as VT_PTR -> VT_USERDEFINED?\n");
	return E_FAIL;
    }
    if (readit) {
	hres = xbuf_get(buf,(LPBYTE)&cookie,sizeof(cookie));
	if (hres)
	    return hres;
	if (cookie == 0) {
	    *arg = 0;
	    if (debugout) MESSAGE("<DISPPARAMS NULL>");
	    return S_OK;
	}
    }
    if (alloc)
	*arg = (DWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DISPPARAMS));
    disps = (DISPPARAMS*)*arg;
    if (!readit)
	return S_OK;
    hres = xbuf_get(buf, (LPBYTE)&disps->cArgs, sizeof(disps->cArgs));
    if (hres)
	return hres;
    if (alloc)
        disps->rgvarg = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(VARIANT)*disps->cArgs);
    if (debugout) MESSAGE("D{");
    for (i=0; i< disps->cArgs; i++) {
        TYPEDESC vdesc;

	vdesc.vt = VT_VARIANT;
	hres = deserialize_param(
	    tinfo,
	    readit,
	    debugout,
	    alloc,
	    &vdesc,
	    (DWORD*)(disps->rgvarg+i),
	    buf
	);
    }
    if (debugout) MESSAGE("}{");
    hres = xbuf_get(buf, (LPBYTE)&disps->cNamedArgs, sizeof(disps->cNamedArgs));
    if (hres)
	return hres;
    if (disps->cNamedArgs) {
	if (alloc)
	    disps->rgdispidNamedArgs = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DISPID)*disps->cNamedArgs);
	for (i=0; i< disps->cNamedArgs; i++) {
	    TYPEDESC vdesc;

	    vdesc.vt = VT_UINT;
	    hres = deserialize_param(
		tinfo,
		readit,
		debugout,
		alloc,
		&vdesc,
		(DWORD*)(disps->rgdispidNamedArgs+i),
		buf
	    );
	    if (debugout && i<(disps->cNamedArgs-1)) MESSAGE(",");
	}
    }
    if (debugout) MESSAGE("}");
    return S_OK;
}

/* Searches function, also in inherited interfaces */
static HRESULT
_get_funcdesc(
    ITypeInfo *tinfo, int iMethod, FUNCDESC **fdesc, BSTR *iname, BSTR *fname
) {
    int i = 0, j = 0;
    HRESULT hres;

    if (fname) *fname = NULL;
    if (iname) *iname = NULL;

    while (1) {
	hres = ITypeInfo_GetFuncDesc(tinfo, i, fdesc);
	if (hres) {
	    ITypeInfo	*tinfo2;
	    HREFTYPE	href;
	    TYPEATTR	*attr;

	    hres = ITypeInfo_GetTypeAttr(tinfo, &attr);
	    if (hres) {
		FIXME("GetTypeAttr failed with %lx\n",hres);
		return hres;
	    }
	    /* Not found, so look in inherited ifaces. */
	    for (j=0;j<attr->cImplTypes;j++) {
		hres = ITypeInfo_GetRefTypeOfImplType(tinfo, j, &href);
		if (hres) {
		    FIXME("Did not find a reftype for interface offset %d?\n",j);
		    break;
		}
		hres = ITypeInfo_GetRefTypeInfo(tinfo, href, &tinfo2);
		if (hres) {
		    FIXME("Did not find a typeinfo for reftype %ld?\n",href);
		    continue;
		}
		hres = _get_funcdesc(tinfo2,iMethod,fdesc,iname,fname);
		ITypeInfo_Release(tinfo2);
		if (!hres) return S_OK;
	    }
	    return E_FAIL;
	}
	if (((*fdesc)->oVft/4) == iMethod) {
	    if (fname)
		ITypeInfo_GetDocumentation(tinfo,(*fdesc)->memid,fname,NULL,NULL,NULL);
	    if (iname)
		ITypeInfo_GetDocumentation(tinfo,-1,iname,NULL,NULL,NULL);
	    return S_OK;
	}
	i++;
    }
    return E_FAIL;
}

static DWORD
xCall(LPVOID retptr, int method, TMProxyImpl *tpinfo /*, args */) {
    DWORD		*args = ((DWORD*)&tpinfo)+1, *xargs;
    FUNCDESC		*fdesc;
    HRESULT		hres;
    int			i, relaydeb = TRACE_ON(olerelay);
    marshal_state	buf;
    RPCOLEMESSAGE	msg;
    ULONG		status;
    BSTR		fname,iname;
    BSTR		names[10];
    int			nrofnames;

    hres = _get_funcdesc(tpinfo->tinfo,method,&fdesc,&iname,&fname);
    if (hres) {
	ERR("Did not find typeinfo/funcdesc entry for method %d!\n",method);
	return 0;
    }

    /*dump_FUNCDESC(fdesc);*/
    if (relaydeb) {
	TRACE_(olerelay)("->");
	if (iname)
	    MESSAGE("%s:",debugstr_w(iname));
	if (fname)
	    MESSAGE("%s(%d)",debugstr_w(fname),method);
	else
	    MESSAGE("%d",method);
	MESSAGE("(");
	if (iname) SysFreeString(iname);
	if (fname) SysFreeString(fname);
    }
    /* Need them for hack below */
    memset(names,0,sizeof(names));
    if (ITypeInfo_GetNames(tpinfo->tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames))
	nrofnames = 0;
    if (nrofnames > sizeof(names)/sizeof(names[0]))
	ERR("Need more names!\n");

    memset(&buf,0,sizeof(buf));
    buf.iid = IID_IUnknown;
    if (method == 0) {
	xbuf_add(&buf,(LPBYTE)args[0],sizeof(IID));
	if (relaydeb) MESSAGE("riid=%s,[out]",debugstr_guid((REFIID)args[0]));
    } else {
	xargs = args;
	for (i=0;i<fdesc->cParams;i++) {
	    ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	    BOOL	isserialized = FALSE;
	    if (relaydeb) {
		if (i) MESSAGE(",");
		if (i+1<nrofnames && names[i+1])
		    MESSAGE("%s=",debugstr_w(names[i+1]));
	    }
	    /* No need to marshal other data than FIN */
	    if (!(elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN)) {
		xargs+=_argsize(elem->tdesc.vt);
		if (relaydeb) MESSAGE("[out]");
		continue;
	    }
	    if (((i+1)<nrofnames) && !IsBadStringPtrW(names[i+1],1)) {
		/* If the parameter is 'riid', we use it as interface IID
		 * for a later ppvObject serialization.
		 */
		buf.thisisiid = !lstrcmpW(names[i+1],riidW);

		/* DISPPARAMS* needs special serializer */
		if (!lstrcmpW(names[i+1],pdispparamsW)) {
		    hres = serialize_DISPPARAM_ptr(
			tpinfo->tinfo,
			elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN,
			relaydeb,
			FALSE,
			&elem->tdesc,
			xargs,
			&buf
		    );
		    isserialized = TRUE;
		}
		if (!lstrcmpW(names[i+1],ppvObjectW)) {
		    hres = serialize_LPVOID_ptr(
			tpinfo->tinfo,
			elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN,
			relaydeb,
			FALSE,
			&elem->tdesc,
			xargs,
			&buf
		    );
		    if (hres == S_OK)
		        isserialized = TRUE;
		}
	    }
	    if (!isserialized)
		hres = serialize_param(
		    tpinfo->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN,
		    relaydeb,
		    FALSE,
		    &elem->tdesc,
		    xargs,
		    &buf
		);

	    if (hres) {
		FIXME("Failed to serialize param, hres %lx\n",hres);
		break;
	    }
	    xargs+=_argsize(elem->tdesc.vt);
	}
    }
    if (relaydeb) MESSAGE(")");
    memset(&msg,0,sizeof(msg));
    msg.cbBuffer = buf.curoff;
    msg.iMethod  = method;
    hres = IRpcChannelBuffer_GetBuffer(tpinfo->chanbuf,&msg,&(tpinfo->iid));
    if (hres) {
	FIXME("RpcChannelBuffer GetBuffer failed, %lx\n",hres);
	return hres;
    }
    memcpy(msg.Buffer,buf.base,buf.curoff);
    if (relaydeb) MESSAGE("\n");
    hres = IRpcChannelBuffer_SendReceive(tpinfo->chanbuf,&msg,&status);
    if (hres) {
	FIXME("RpcChannelBuffer SendReceive failed, %lx\n",hres);
	return hres;
    }
    relaydeb = TRACE_ON(olerelay);
    if (relaydeb) MESSAGE(" = %08lx (",status);
    if (buf.base)
	buf.base = HeapReAlloc(GetProcessHeap(),0,buf.base,msg.cbBuffer);
    else
	buf.base = HeapAlloc(GetProcessHeap(),0,msg.cbBuffer);
    buf.size = msg.cbBuffer;
    memcpy(buf.base,msg.Buffer,buf.size);
    buf.curoff = 0;
    if (method == 0) {
	_unmarshal_interface(&buf,(REFIID)args[0],(LPUNKNOWN*)args[1]);
	if (relaydeb) MESSAGE("[in],%p",*((DWORD**)args[1]));
    } else {
	xargs = args;
	for (i=0;i<fdesc->cParams;i++) {
	    ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	    BOOL	isdeserialized = FALSE;

	    if (relaydeb) {
		if (i) MESSAGE(",");
		if (i+1<nrofnames && names[i+1]) MESSAGE("%s=",debugstr_w(names[i+1]));
	    }
	    /* No need to marshal other data than FOUT I think */
	    if (!(elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT)) {
		xargs += _argsize(elem->tdesc.vt);
		if (relaydeb) MESSAGE("[in]");
		continue;
	    }
	    if (((i+1)<nrofnames) && !IsBadStringPtrW(names[i+1],1)) {
		/* If the parameter is 'riid', we use it as interface IID
		 * for a later ppvObject serialization.
		 */
		buf.thisisiid = !lstrcmpW(names[i+1],riidW);

		/* deserialize DISPPARAM */
		if (!lstrcmpW(names[i+1],pdispparamsW)) {
		    hres = deserialize_DISPPARAM_ptr(
			tpinfo->tinfo,
			elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
			relaydeb,
			FALSE,
			&(elem->tdesc),
			xargs,
			&buf
		    );
		    if (hres) {
			FIXME("Failed to deserialize DISPPARAM*, hres %lx\n",hres);
			break;
		    }
		    isdeserialized = TRUE;
		}
		if (!lstrcmpW(names[i+1],ppvObjectW)) {
		    hres = deserialize_LPVOID_ptr(
			tpinfo->tinfo,
			elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
			relaydeb,
			FALSE,
			&elem->tdesc,
			xargs,
			&buf
		    );
		    if (hres == S_OK)
			isdeserialized = TRUE;
		}
	    }
	    if (!isdeserialized)
		hres = deserialize_param(
		    tpinfo->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
		    relaydeb,
		    FALSE,
		    &(elem->tdesc),
		    xargs,
		    &buf
		);
	    if (hres) {
		FIXME("Failed to unmarshall param, hres %lx\n",hres);
		break;
	    }
	    xargs += _argsize(elem->tdesc.vt);
	}
    }
    if (relaydeb) MESSAGE(")\n\n");
    HeapFree(GetProcessHeap(),0,buf.base);
    return status;
}

static HRESULT WINAPI
PSFacBuf_CreateProxy(
    LPPSFACTORYBUFFER iface, IUnknown* pUnkOuter, REFIID riid,
    IRpcProxyBuffer **ppProxy, LPVOID *ppv
) {
    HRESULT	hres;
    ITypeInfo	*tinfo;
    int		i, nroffuncs;
    FUNCDESC	*fdesc;
    TMProxyImpl	*proxy;

    TRACE("(...%s...)\n",debugstr_guid(riid));
    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	FIXME("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }
    nroffuncs = _nroffuncs(tinfo);
    proxy = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TMProxyImpl));
    if (!proxy) return E_OUTOFMEMORY;
    proxy->asmstubs=HeapAlloc(GetProcessHeap(),0,sizeof(TMAsmProxy)*nroffuncs);

    assert(sizeof(TMAsmProxy) == 12);

    proxy->lpvtbl = HeapAlloc(GetProcessHeap(),0,sizeof(LPBYTE)*nroffuncs);
    for (i=0;i<nroffuncs;i++) {
	int		nrofargs;
	TMAsmProxy	*xasm = proxy->asmstubs+i;

	/* nrofargs without This */
	switch (i) {
	case 0: nrofargs = 2;
		break;
	case 1: case 2: nrofargs = 0;
		break;
	default: {
		int j;
		hres = _get_funcdesc(tinfo,i,&fdesc,NULL,NULL);
		if (hres) {
		    FIXME("GetFuncDesc %lx should not fail here.\n",hres);
		    return hres;
		}
		/* some args take more than 4 byte on the stack */
		nrofargs = 0;
		for (j=0;j<fdesc->cParams;j++)
		    nrofargs += _argsize(fdesc->lprgelemdescParam[j].tdesc.vt);

		if (fdesc->callconv != CC_STDCALL) {
		    ERR("calling convention is not stdcall????\n");
		    return E_FAIL;
		}
		break;
	    }
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
	xasm->popleax	= 0x58;
	xasm->pushlval	= 0x6a;
	xasm->nr	= i;
	xasm->pushleax	= 0x50;
	xasm->lcall	= 0xe8;	/* relative jump */
	xasm->xcall	= (DWORD)xCall;
	xasm->xcall     -= (DWORD)&(xasm->lret);
	xasm->lret	= 0xc2;
	xasm->bytestopop= (nrofargs+2)*4; /* pop args, This, iMethod */
	proxy->lpvtbl[i] = (DWORD)xasm;
    }
    proxy->lpvtbl2	= &tmproxyvtable;
    proxy->ref		= 2;
    proxy->tinfo	= tinfo;
    memcpy(&proxy->iid,riid,sizeof(*riid));
    *ppv		= (LPVOID)proxy;
    *ppProxy		= (IRpcProxyBuffer *)&(proxy->lpvtbl2);
    return S_OK;
}

typedef struct _TMStubImpl {
    ICOM_VTABLE(IRpcStubBuffer)	*lpvtbl;
    DWORD			ref;

    LPUNKNOWN			pUnk;
    ITypeInfo			*tinfo;
    IID				iid;
} TMStubImpl;

static HRESULT WINAPI
TMStubImpl_QueryInterface(LPRPCSTUBBUFFER iface, REFIID riid, LPVOID *ppv) {
    if (IsEqualIID(riid,&IID_IRpcStubBuffer)||IsEqualIID(riid,&IID_IUnknown)){
	*ppv = (LPVOID)iface;
	IRpcStubBuffer_AddRef(iface);
	return S_OK;
    }
    FIXME("%s, not supported IID.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
TMStubImpl_AddRef(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(TMStubImpl,iface);

    This->ref++;
    return This->ref;
}

static ULONG WINAPI
TMStubImpl_Release(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(TMStubImpl,iface);

    This->ref--;
    if (This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI
TMStubImpl_Connect(LPRPCSTUBBUFFER iface, LPUNKNOWN pUnkServer) {
    ICOM_THIS(TMStubImpl,iface);

    IUnknown_AddRef(pUnkServer);
    This->pUnk = pUnkServer;
    return S_OK;
}

static void WINAPI
TMStubImpl_Disconnect(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(TMStubImpl,iface);

    IUnknown_Release(This->pUnk);
    This->pUnk = NULL;
    return;
}

static HRESULT WINAPI
TMStubImpl_Invoke(
    LPRPCSTUBBUFFER iface, RPCOLEMESSAGE* xmsg,IRpcChannelBuffer*rpcchanbuf
) {
    int		i;
    FUNCDESC	*fdesc;
    ICOM_THIS(TMStubImpl,iface);
    HRESULT	hres;
    DWORD	*args, res, *xargs, nrofargs;
    marshal_state	buf;
    int		nrofnames;
    BSTR	names[10];

    memset(&buf,0,sizeof(buf));
    buf.size	= xmsg->cbBuffer;
    buf.base	= xmsg->Buffer;
    buf.curoff	= 0;
    buf.iid	= IID_IUnknown;

    TRACE("...\n");
    if (xmsg->iMethod == 0) { /* QI */
	IID		xiid;
	/* in: IID, out: <iface> */

	xbuf_get(&buf,(LPBYTE)&xiid,sizeof(xiid));
	buf.curoff = 0;
	hres = _marshal_interface(&buf,&xiid,This->pUnk);
	xmsg->Buffer	= buf.base; /* Might have been reallocated */
	xmsg->cbBuffer	= buf.size;
	return hres;
    }
    hres = _get_funcdesc(This->tinfo,xmsg->iMethod,&fdesc,NULL,NULL);
    if (hres) {
	FIXME("GetFuncDesc on method %ld failed with %lx\n",xmsg->iMethod,hres);
	return hres;
    }
    /* Need them for hack below */
    memset(names,0,sizeof(names));
    ITypeInfo_GetNames(This->tinfo,fdesc->memid,names,sizeof(names)/sizeof(names[0]),&nrofnames);
    if (nrofnames > sizeof(names)/sizeof(names[0])) {
	ERR("Need more names!\n");
    }

    /*dump_FUNCDESC(fdesc);*/
    nrofargs = 0;
    for (i=0;i<fdesc->cParams;i++)
	nrofargs += _argsize(fdesc->lprgelemdescParam[i].tdesc.vt);
    args = HeapAlloc(GetProcessHeap(),0,(nrofargs+1)*sizeof(DWORD));
    if (!args) return E_OUTOFMEMORY;

    /* Allocate all stuff used by call. */
    xargs = args+1;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	BOOL		isdeserialized = FALSE;

	if (((i+1)<nrofnames) && !IsBadStringPtrW(names[i+1],1)) {
	    /* If the parameter is 'riid', we use it as interface IID
	     * for a later ppvObject serialization.
	     */
	    buf.thisisiid = !lstrcmpW(names[i+1],riidW);

	    /* deserialize DISPPARAM */
	    if (!lstrcmpW(names[i+1],pdispparamsW)) {
		hres = deserialize_DISPPARAM_ptr(
		    This->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN,
		    FALSE,
		    TRUE,
		    &(elem->tdesc),
		    xargs,
		    &buf
		);
		if (hres) {
		    FIXME("Failed to deserialize DISPPARAM*, hres %lx\n",hres);
		    break;
		}
		isdeserialized = TRUE;
	    }
	    if (!lstrcmpW(names[i+1],ppvObjectW)) {
		hres = deserialize_LPVOID_ptr(
		    This->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
		    FALSE,
		    TRUE,
		    &elem->tdesc,
		    xargs,
		    &buf
		);
		if (hres == S_OK)
		    isdeserialized = TRUE;
	    }
	}
	if (!isdeserialized)
	    hres = deserialize_param(
		This->tinfo,
		elem->u.paramdesc.wParamFlags & PARAMFLAG_FIN,
		FALSE,
		TRUE,
		&(elem->tdesc),
		xargs,
		&buf
	    );
	xargs += _argsize(elem->tdesc.vt);
	if (hres) {
	    FIXME("Failed to deserialize param %s, hres %lx\n",debugstr_w(names[i+1]),hres);
	    break;
	}
    }
    hres = IUnknown_QueryInterface(This->pUnk,&(This->iid),(LPVOID*)&(args[0]));
    if (hres) {
	ERR("Does not support iface %s\n",debugstr_guid(&(This->iid)));
	return hres;
    }
    res = _invoke(
	    (*((FARPROC**)args[0]))[fdesc->oVft/4],
	    fdesc->callconv,
	    (xargs-args),
	    args
    );
    IUnknown_Release((LPUNKNOWN)args[0]);
    buf.curoff = 0;
    xargs = args+1;
    for (i=0;i<fdesc->cParams;i++) {
	ELEMDESC	*elem = fdesc->lprgelemdescParam+i;
	BOOL		isserialized = FALSE;

	if (((i+1)<nrofnames) && !IsBadStringPtrW(names[i+1],1)) {
	    /* If the parameter is 'riid', we use it as interface IID
	     * for a later ppvObject serialization.
	     */
	    buf.thisisiid = !lstrcmpW(names[i+1],riidW);

	    /* DISPPARAMS* needs special serializer */
	    if (!lstrcmpW(names[i+1],pdispparamsW)) {
		hres = serialize_DISPPARAM_ptr(
		    This->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
		    FALSE,
		    TRUE,
		    &elem->tdesc,
		    xargs,
		    &buf
		);
		isserialized = TRUE;
	    }
	    if (!lstrcmpW(names[i+1],ppvObjectW)) {
		hres = serialize_LPVOID_ptr(
		    This->tinfo,
		    elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
		    FALSE,
		    TRUE,
		    &elem->tdesc,
		    xargs,
		    &buf
		);
		if (hres == S_OK)
		    isserialized = TRUE;
	    }
	}
	if (!isserialized)
	    hres = serialize_param(
	       This->tinfo,
	       elem->u.paramdesc.wParamFlags & PARAMFLAG_FOUT,
	       FALSE,
	       TRUE,
	       &elem->tdesc,
	       xargs,
	       &buf
	    );
	xargs += _argsize(elem->tdesc.vt);
	if (hres) {
	    FIXME("Failed to stuballoc param, hres %lx\n",hres);
	    break;
	}
    }
    /* might need to use IRpcChannelBuffer_GetBuffer ? */
    xmsg->cbBuffer	= buf.curoff;
    xmsg->Buffer	= buf.base;
    HeapFree(GetProcessHeap(),0,args);
    return res;
}

static LPRPCSTUBBUFFER WINAPI
TMStubImpl_IsIIDSupported(LPRPCSTUBBUFFER iface, REFIID riid) {
    FIXME("Huh (%s)?\n",debugstr_guid(riid));
    return NULL;
}

static ULONG WINAPI
TMStubImpl_CountRefs(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(TMStubImpl,iface);

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

ICOM_VTABLE(IRpcStubBuffer) tmstubvtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

    TRACE("(%s,%p,%p)\n",debugstr_guid(riid),pUnkServer,ppStub);
    hres = _get_typeinfo_for_iid(riid,&tinfo);
    if (hres) {
	FIXME("No typeinfo for %s?\n",debugstr_guid(riid));
	return hres;
    }
    stub = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TMStubImpl));
    if (!stub)
	return E_OUTOFMEMORY;
    stub->lpvtbl	= &tmstubvtbl;
    stub->ref		= 1;
    stub->tinfo		= tinfo;
    memcpy(&(stub->iid),riid,sizeof(*riid));
    hres = IRpcStubBuffer_Connect((LPRPCSTUBBUFFER)stub,pUnkServer);
    *ppStub 		= (LPRPCSTUBBUFFER)stub;
    if (hres)
	FIXME("Connect to pUnkServer failed?\n");
    return hres;
}

static ICOM_VTABLE(IPSFactoryBuffer) psfacbufvtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    PSFacBuf_QueryInterface,
    PSFacBuf_AddRef,
    PSFacBuf_Release,
    PSFacBuf_CreateProxy,
    PSFacBuf_CreateStub
};

/* This is the whole PSFactoryBuffer object, just the vtableptr */
static ICOM_VTABLE(IPSFactoryBuffer) *lppsfac = &psfacbufvtbl;

/***********************************************************************
 *           DllGetClassObject [OLE32.63]
 */
HRESULT WINAPI
TypeLibFac_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    if (IsEqualIID(iid,&IID_IPSFactoryBuffer)) {
	*ppv = &lppsfac;
	return S_OK;
    }
    return E_NOINTERFACE;
}
