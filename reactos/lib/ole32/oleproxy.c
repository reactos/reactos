/*
 *	OLE32 proxy/stub handler
 *
 *  Copyright 2002  Marcus Meissner
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

/* Documentation on MSDN:
 *
 * (COM Proxy)
 * http://msdn.microsoft.com/library/en-us/com/comext_1q0p.asp
 *
 * (COM Stub)
 * http://msdn.microsoft.com/library/en-us/com/comext_1lia.asp
 *
 * (Marshal)
 * http://msdn.microsoft.com/library/en-us/com/comext_1gfn.asp
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wtypes.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

const CLSID CLSID_DfMarshal       = { 0x0000030b, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46} };
const CLSID CLSID_PSFactoryBuffer = { 0x00000320, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46} };

/* From: http://msdn.microsoft.com/library/en-us/com/cmi_m_4lda.asp
 *
 * The first time a client requests a pointer to an interface on a
 * particular object, COM loads an IClassFactory stub in the server
 * process and uses it to marshal the first pointer back to the
 * client. In the client process, COM loads the generic proxy for the
 * class factory object and calls its implementation of IMarshal to
 * unmarshal that first pointer. COM then creates the first interface
 * proxy and hands it a pointer to the RPC channel. Finally, COM returns
 * the IClassFactory pointer to the client, which uses it to call
 * IClassFactory::CreateInstance, passing it a reference to the interface.
 *
 * Back in the server process, COM now creates a new instance of the
 * object, along with a stub for the requested interface. This stub marshals
 * the interface pointer back to the client process, where another object
 * proxy is created, this time for the object itself. Also created is a
 * proxy for the requested interface, a pointer to which is returned to
 * the client. With subsequent calls to other interfaces on the object,
 * COM will load the appropriate interface stubs and proxies as needed.
 */
typedef struct _CFStub {
    ICOM_VTABLE(IRpcStubBuffer)	*lpvtbl;
    DWORD			ref;

    LPUNKNOWN			pUnkServer;
} CFStub;

static HRESULT WINAPI
CFStub_QueryInterface(LPRPCSTUBBUFFER iface, REFIID riid, LPVOID *ppv) {
    if (IsEqualIID(&IID_IUnknown,riid)||IsEqualIID(&IID_IRpcStubBuffer,riid)) {
	*ppv = (LPVOID)iface;
	IUnknown_AddRef(iface);
	return S_OK;
    }
    FIXME("(%s), interface not supported.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
CFStub_AddRef(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(CFStub,iface);

    This->ref++;
    return This->ref;
}

static ULONG WINAPI
CFStub_Release(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(CFStub,iface);

    This->ref--;
    if (This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI
CFStub_Connect(LPRPCSTUBBUFFER iface, IUnknown *pUnkServer) {
    ICOM_THIS(CFStub,iface);

    This->pUnkServer = pUnkServer;
    IUnknown_AddRef(pUnkServer);
    return S_OK;
}

static void WINAPI
CFStub_Disconnect(LPRPCSTUBBUFFER iface) {
    ICOM_THIS(CFStub,iface);

    IUnknown_Release(This->pUnkServer);
    This->pUnkServer = NULL;
}
static HRESULT WINAPI
CFStub_Invoke(
    LPRPCSTUBBUFFER iface,RPCOLEMESSAGE* msg,IRpcChannelBuffer* chanbuf
) {
    ICOM_THIS(CFStub,iface);
    HRESULT hres;

    if (msg->iMethod == 3) { /* CreateInstance */
	IID iid;
	IClassFactory	*classfac;
	IUnknown	*ppv;
	IStream		*pStm;
	STATSTG		ststg;
	ULARGE_INTEGER	newpos;
	LARGE_INTEGER	seekto;
	ULONG		res;

	if (msg->cbBuffer < sizeof(IID)) {
	    FIXME("Not enough bytes in buffer (%ld instead of %d)?\n",msg->cbBuffer,sizeof(IID));
	    return E_FAIL;
	}
	memcpy(&iid,msg->Buffer,sizeof(iid));
	TRACE("->CreateInstance(%s)\n",debugstr_guid(&iid));
	hres = IUnknown_QueryInterface(This->pUnkServer,&IID_IClassFactory,(LPVOID*)(char*)&classfac);
	if (hres) {
	    FIXME("Ole server does not provide a IClassFactory?\n");
	    return hres;
	}
	hres = IClassFactory_CreateInstance(classfac,NULL,&iid,(LPVOID*)(char*)&ppv);
	IClassFactory_Release(classfac);
	if (hres) {
	    msg->cbBuffer = 0;
	    FIXME("Failed to create an instance of %s\n",debugstr_guid(&iid));
	    return hres;
	}
	hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
	if (hres) {
	    FIXME("Failed to create stream on hglobal\n");
	    return hres;
	}
	hres = CoMarshalInterface(pStm,&iid,ppv,0,NULL,0);
	if (hres) {
	    FIXME("CoMarshalInterface failed, %lx!\n",hres);
	    msg->cbBuffer = 0;
	    return hres;
	}
	hres = IStream_Stat(pStm,&ststg,0);
	if (hres) {
	    FIXME("Stat failed.\n");
	    return hres;
	}

	msg->cbBuffer = ststg.cbSize.u.LowPart;

	if (msg->Buffer)
	    msg->Buffer = HeapReAlloc(GetProcessHeap(),0,msg->Buffer,ststg.cbSize.u.LowPart);
	else
	    msg->Buffer = HeapAlloc(GetProcessHeap(),0,ststg.cbSize.u.LowPart);

	seekto.u.LowPart = 0;seekto.u.HighPart = 0;
	hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
	if (hres) {
	    FIXME("IStream_Seek failed, %lx\n",hres);
	    return hres;
	}
	hres = IStream_Read(pStm,msg->Buffer,msg->cbBuffer,&res);
	if (hres) {
	    FIXME("Stream Read failed, %lx\n",hres);
	    return hres;
	}
	IStream_Release(pStm);
	return S_OK;
    }
    FIXME("(%p,%p), stub!\n",msg,chanbuf);
    FIXME("iMethod is %ld\n",msg->iMethod);
    FIXME("cbBuffer is %ld\n",msg->cbBuffer);
    return E_FAIL;
}

static LPRPCSTUBBUFFER WINAPI
CFStub_IsIIDSupported(LPRPCSTUBBUFFER iface,REFIID riid) {
    FIXME("(%s), stub!\n",debugstr_guid(riid));
    return NULL;
}

static ULONG WINAPI
CFStub_CountRefs(LPRPCSTUBBUFFER iface) {
    FIXME("(), stub!\n");
    return 1;
}

static HRESULT WINAPI
CFStub_DebugServerQueryInterface(LPRPCSTUBBUFFER iface,void** ppv) {
    FIXME("(%p), stub!\n",ppv);
    return E_FAIL;
}
static void    WINAPI
CFStub_DebugServerRelease(LPRPCSTUBBUFFER iface,void *pv) {
    FIXME("(%p), stub!\n",pv);
}

static ICOM_VTABLE(IRpcStubBuffer) cfstubvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    CFStub_QueryInterface,
    CFStub_AddRef,
    CFStub_Release,
    CFStub_Connect,
    CFStub_Disconnect,
    CFStub_Invoke,
    CFStub_IsIIDSupported,
    CFStub_CountRefs,
    CFStub_DebugServerQueryInterface,
    CFStub_DebugServerRelease
};

static HRESULT
CFStub_Construct(LPRPCSTUBBUFFER *ppv) {
    CFStub *cfstub;
    cfstub = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CFStub));
    if (!cfstub)
	return E_OUTOFMEMORY;
    *ppv = (LPRPCSTUBBUFFER)cfstub;
    cfstub->lpvtbl	= &cfstubvt;
    cfstub->ref		= 1;
    return S_OK;
}

/* Since we create proxy buffers and classfactory in a pair, there is
 * no need for 2 separate structs. Just put them in one, but remember
 * the refcount.
 */
typedef struct _CFProxy {
    ICOM_VTABLE(IClassFactory)		*lpvtbl_cf;
    ICOM_VTABLE(IRpcProxyBuffer)	*lpvtbl_proxy;
    DWORD				ref;

    IRpcChannelBuffer			*chanbuf;
} CFProxy;

static HRESULT WINAPI IRpcProxyBufferImpl_QueryInterface(LPRPCPROXYBUFFER iface,REFIID riid,LPVOID *ppv) {
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcProxyBuffer)||IsEqualIID(riid,&IID_IUnknown)) {
	IRpcProxyBuffer_AddRef(iface);
	*ppv = (LPVOID)iface;
	return S_OK;
    }
    FIXME("(%s), no interface.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IRpcProxyBufferImpl_AddRef(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_proxy,iface);
    return ++(This->ref);
}

static ULONG WINAPI IRpcProxyBufferImpl_Release(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_proxy,iface);

    if (!--(This->ref)) {
	IRpcChannelBuffer_Release(This->chanbuf);This->chanbuf = NULL;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }
    return This->ref;
}

static HRESULT WINAPI IRpcProxyBufferImpl_Connect(LPRPCPROXYBUFFER iface,IRpcChannelBuffer* pRpcChannelBuffer) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_proxy,iface);

    This->chanbuf = pRpcChannelBuffer;
    IRpcChannelBuffer_AddRef(This->chanbuf);
    return S_OK;
}
static void WINAPI IRpcProxyBufferImpl_Disconnect(LPRPCPROXYBUFFER iface) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_proxy,iface);
    if (This->chanbuf) {
	IRpcChannelBuffer_Release(This->chanbuf);
	This->chanbuf = NULL;
    }
}

static HRESULT WINAPI
CFProxy_QueryInterface(LPCLASSFACTORY iface,REFIID riid, LPVOID *ppv) {
    *ppv = NULL;
    if (IsEqualIID(&IID_IClassFactory,riid) || IsEqualIID(&IID_IUnknown,riid)) {
	*ppv = (LPVOID)iface;
	IClassFactory_AddRef(iface);
	return S_OK;
    }
    if (IsEqualIID(riid,&IID_IMarshal)) /* just to avoid debug output */
	return E_NOINTERFACE;
    FIXME("Unhandled interface: %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG   WINAPI CFProxy_AddRef(LPCLASSFACTORY iface) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_cf,iface);
    This->ref++;
    return This->ref;
}

static ULONG   WINAPI CFProxy_Release(LPCLASSFACTORY iface) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_cf,iface);
    This->ref--;
    if (This->ref)
	return This->ref;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI CFProxy_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,/* [in] */
    REFIID riid,	/* [in] */
    LPVOID *ppv		/* [out] */
) {
    ICOM_THIS_MULTI(CFProxy,lpvtbl_cf,iface);
    HRESULT		hres;
    LPSTREAM		pStream;
    HGLOBAL		hGlobal;
    ULONG		srstatus;
    RPCOLEMESSAGE	msg;

    TRACE("(%p,%s,%p)\n",pUnkOuter,debugstr_guid(riid),ppv);

    /* Send CreateInstance to the remote classfactory.
     *
     * Data: Only the 'IID'.
     */
    msg.iMethod  = 3;
    msg.cbBuffer = sizeof(*riid);
    msg.Buffer	 = NULL;
    hres = IRpcChannelBuffer_GetBuffer(This->chanbuf,&msg,&IID_IClassFactory);
    if (hres) {
	FIXME("IRpcChannelBuffer_GetBuffer failed with %lx?\n",hres);
	return hres;
    }
    memcpy(msg.Buffer,riid,sizeof(*riid));
    hres = IRpcChannelBuffer_SendReceive(This->chanbuf,&msg,&srstatus);
    if (hres) {
	FIXME("IRpcChannelBuffer_SendReceive failed with %lx?\n",hres);
	return hres;
    }

    if (!msg.cbBuffer) /* interface not found on remote */
	return srstatus;

    /* We got back: [Marshalled Interface data] */
    TRACE("got %ld bytes data.\n",msg.cbBuffer);
    hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_NODISCARD|GMEM_SHARE,msg.cbBuffer);
    memcpy(GlobalLock(hGlobal),msg.Buffer,msg.cbBuffer);
    hres = CreateStreamOnHGlobal(hGlobal,TRUE,&pStream);
    if (hres) {
	FIXME("CreateStreamOnHGlobal failed with %lx\n",hres);
	return hres;
    }
    hres = CoUnmarshalInterface(
	    pStream,
	    riid,
	    ppv
    );
    IStream_Release(pStream); /* Does GlobalFree hGlobal too. */
    if (hres) {
	FIXME("CoMarshalInterface failed, %lx\n",hres);
	return hres;
    }
    return S_OK;
}

static HRESULT WINAPI CFProxy_LockServer(LPCLASSFACTORY iface,BOOL fLock) {
    /*ICOM_THIS_MULTI(CFProxy,lpvtbl_cf,iface);*/
    FIXME("(%d), stub!\n",fLock);
    /* basically: write BOOL, read empty */
    return S_OK;
}

static ICOM_VTABLE(IRpcProxyBuffer) pspbvtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IRpcProxyBufferImpl_QueryInterface,
    IRpcProxyBufferImpl_AddRef,
    IRpcProxyBufferImpl_Release,
    IRpcProxyBufferImpl_Connect,
    IRpcProxyBufferImpl_Disconnect
};
static ICOM_VTABLE(IClassFactory) cfproxyvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    CFProxy_QueryInterface,
    CFProxy_AddRef,
    CFProxy_Release,
    CFProxy_CreateInstance,
    CFProxy_LockServer
};

static HRESULT
CFProxy_Construct(LPVOID *ppv,LPVOID *ppProxy) {
    CFProxy *cf;

    cf = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CFProxy));
    if (!cf)
	return E_OUTOFMEMORY;

    cf->lpvtbl_cf	= &cfproxyvt;
    cf->lpvtbl_proxy	= &pspbvtbl;
    cf->ref		= 2; /* we return 2 references to the object! */
    *ppv		= &(cf->lpvtbl_cf);
    *ppProxy		= &(cf->lpvtbl_proxy);
    return S_OK;
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

static HRESULT WINAPI
PSFacBuf_CreateProxy(
    LPPSFACTORYBUFFER iface, IUnknown* pUnkOuter, REFIID riid,
    IRpcProxyBuffer **ppProxy, LPVOID *ppv
) {
    if (IsEqualIID(&IID_IClassFactory,riid) ||
	IsEqualIID(&IID_IUnknown,riid)
    )
	return CFProxy_Construct(ppv,(LPVOID*)ppProxy);
    FIXME("proxying not implemented for (%s) yet!\n",debugstr_guid(riid));
    return E_FAIL;
}

static HRESULT WINAPI
PSFacBuf_CreateStub(
    LPPSFACTORYBUFFER iface, REFIID riid,IUnknown *pUnkServer,
    IRpcStubBuffer** ppStub
) {
    HRESULT hres;

    TRACE("(%s,%p,%p)\n",debugstr_guid(riid),pUnkServer,ppStub);

    if (IsEqualIID(&IID_IClassFactory,riid) ||
        IsEqualIID(&IID_IUnknown,riid)
    ) {
	hres = CFStub_Construct(ppStub);
	if (!hres)
	    IRpcStubBuffer_Connect((*ppStub),pUnkServer);
	return hres;
    }
    FIXME("stubbing not implemented for (%s) yet!\n",debugstr_guid(riid));
    return E_FAIL;
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
 *           DllGetClassObject [OLE32.@]
 */
HRESULT WINAPI OLE32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(rclsid,&CLSID_PSFactoryBuffer)) {
	*ppv = &lppsfac;
	/* If we create a ps factory, we might need a stub manager later
	 * anyway
	 */
	STUBMGR_Start();
	return S_OK;
    }
    if (IsEqualIID(rclsid,&CLSID_DfMarshal)&&(
		IsEqualIID(iid,&IID_IClassFactory) ||
		IsEqualIID(iid,&IID_IUnknown)
	)
    )
	return MARSHAL_GetStandardMarshalCF(ppv);
    if (IsEqualIID(rclsid,&CLSID_StdGlobalInterfaceTable) && (IsEqualIID(iid,&IID_IClassFactory) || IsEqualIID(iid,&IID_IUnknown)))
        return StdGlobalInterfaceTable_GetFactory(ppv);

    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
