/*
 *	Marshalling library
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wtypes.h"
#include "wine/unicode.h"
#include "wine/winbase16.h"
#include "compobj_private.h"
#include "ifs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

extern const CLSID CLSID_DfMarshal;

/* Marshalling just passes a unique identifier to the remote client,
 * that makes it possible to find the passed interface again.
 *
 * So basically we need a set of values that make it unique.
 *
 * 	Process Identifier, Object IUnknown ptr, IID
 *
 * Note that the IUnknown_QI(ob,xiid,&ppv) always returns the SAME ppv value!
 */

typedef struct _mid2unknown {
    wine_marshal_id	mid;
    LPUNKNOWN		pUnk;
} mid2unknown;

typedef struct _mid2stub {
    wine_marshal_id	mid;
    IRpcStubBuffer	*stub;
    LPUNKNOWN		pUnkServer;
} mid2stub;

static mid2stub *stubs = NULL;
static int nrofstubs = 0;

static mid2unknown *proxies = NULL;
static int nrofproxies = 0;

HRESULT
MARSHAL_Find_Stub_Server(wine_marshal_id *mid,LPUNKNOWN *punk) {
    int i;

    for (i=0;i<nrofstubs;i++) {
	if (MARSHAL_Compare_Mids_NoInterface(mid,&(stubs[i].mid))) {
	    *punk = stubs[i].pUnkServer;
	    IUnknown_AddRef((*punk));
	    return S_OK;
	}
    }
    return E_FAIL;
}

HRESULT
MARSHAL_Find_Stub_Buffer(wine_marshal_id *mid,IRpcStubBuffer **stub) {
    int i;

    for (i=0;i<nrofstubs;i++) {
	if (MARSHAL_Compare_Mids(mid,&(stubs[i].mid))) {
	    *stub = stubs[i].stub;
	    IUnknown_AddRef((*stub));
	    return S_OK;
	}
    }
    return E_FAIL;
}

HRESULT
MARSHAL_Find_Stub(wine_marshal_id *mid,LPUNKNOWN *pUnk) {
    int i;

    for (i=0;i<nrofstubs;i++) {
	if (MARSHAL_Compare_Mids(mid,&(stubs[i].mid))) {
	    *pUnk = stubs[i].pUnkServer;
	    IUnknown_AddRef((*pUnk));
	    return S_OK;
	}
    }
    return E_FAIL;
}

HRESULT
MARSHAL_Register_Stub(wine_marshal_id *mid,LPUNKNOWN pUnk,IRpcStubBuffer *stub) {
    LPUNKNOWN	xPunk;
    if (!MARSHAL_Find_Stub(mid,&xPunk)) {
	FIXME("Already have entry for (%lx/%s)!\n",mid->objectid,debugstr_guid(&(mid->iid)));
	return S_OK;
    }
    if (nrofstubs)
	stubs=HeapReAlloc(GetProcessHeap(),0,stubs,sizeof(stubs[0])*(nrofstubs+1));
    else
	stubs=HeapAlloc(GetProcessHeap(),0,sizeof(stubs[0]));
    if (!stubs) return E_OUTOFMEMORY;
    stubs[nrofstubs].stub = stub;
    stubs[nrofstubs].pUnkServer = pUnk;
    memcpy(&(stubs[nrofstubs].mid),mid,sizeof(*mid));
    nrofstubs++;
    return S_OK;
}

HRESULT
MARSHAL_Find_Proxy(wine_marshal_id *mid,LPUNKNOWN *punk) {
    int i;

    for (i=0;i<nrofproxies;i++)
	if (MARSHAL_Compare_Mids(mid,&(proxies[i].mid))) {
	    *punk = proxies[i].pUnk;
	    IUnknown_AddRef((*punk));
	    return S_OK;
	}
    return E_FAIL;
}

HRESULT
MARSHAL_Find_Proxy_Object(wine_marshal_id *mid,LPUNKNOWN *punk) {
    int i;

    for (i=0;i<nrofproxies;i++)
	if (MARSHAL_Compare_Mids_NoInterface(mid,&(proxies[i].mid))) {
	    *punk = proxies[i].pUnk;
	    IUnknown_AddRef((*punk));
	    return S_OK;
	}
    return E_FAIL;
}

HRESULT
MARSHAL_Register_Proxy(wine_marshal_id *mid,LPUNKNOWN punk) {
    int i;

    for (i=0;i<nrofproxies;i++) {
	if (MARSHAL_Compare_Mids(mid,&(proxies[i].mid))) {
	    ERR("Already have mid?\n");
	    return E_FAIL;
	}
    }
    if (nrofproxies)
	proxies = HeapReAlloc(GetProcessHeap(),0,proxies,sizeof(proxies[0])*(nrofproxies+1));
    else
	proxies = HeapAlloc(GetProcessHeap(),0,sizeof(proxies[0]));
    memcpy(&(proxies[nrofproxies].mid),mid,sizeof(*mid));
    proxies[nrofproxies].pUnk = punk;
    nrofproxies++;
    IUnknown_AddRef(punk);
    return S_OK;
}

/********************** StdMarshal implementation ****************************/
typedef struct _StdMarshalImpl {
  ICOM_VTABLE(IMarshal)	*lpvtbl;
  DWORD			ref;

  IID			iid;
  DWORD			dwDestContext;
  LPVOID		pvDestContext;
  DWORD			mshlflags;
} StdMarshalImpl;

HRESULT WINAPI
StdMarshalImpl_QueryInterface(LPMARSHAL iface,REFIID riid,LPVOID *ppv) {
  *ppv = NULL;
  if (IsEqualIID(&IID_IUnknown,riid) || IsEqualIID(&IID_IMarshal,riid)) {
    *ppv = iface;
    IUnknown_AddRef(iface);
    return S_OK;
  }
  FIXME("No interface for %s.\n",debugstr_guid(riid));
  return E_NOINTERFACE;
}

ULONG WINAPI
StdMarshalImpl_AddRef(LPMARSHAL iface) {
  ICOM_THIS(StdMarshalImpl,iface);
  This->ref++;
  return This->ref;
}

ULONG WINAPI
StdMarshalImpl_Release(LPMARSHAL iface) {
  ICOM_THIS(StdMarshalImpl,iface);
  This->ref--;

  if (This->ref)
    return This->ref;
  HeapFree(GetProcessHeap(),0,This);
  return 0;
}

HRESULT WINAPI
StdMarshalImpl_GetUnmarshalClass(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid
) {
  memcpy(pCid,&CLSID_DfMarshal,sizeof(CLSID_DfMarshal));
  return S_OK;
}

HRESULT WINAPI
StdMarshalImpl_GetMarshalSizeMax(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize
) {
  *pSize = sizeof(wine_marshal_id)+sizeof(wine_marshal_data);
  return S_OK;
}

HRESULT WINAPI
StdMarshalImpl_MarshalInterface(
  LPMARSHAL iface, IStream *pStm,REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags
) {
  wine_marshal_id	mid;
  wine_marshal_data 	md;
  IUnknown		*pUnk;
  ULONG			res;
  HRESULT		hres;
  IRpcStubBuffer	*stub;
  IPSFactoryBuffer	*psfacbuf;

  TRACE("(...,%s,...)\n",debugstr_guid(riid));
  IUnknown_QueryInterface((LPUNKNOWN)pv,&IID_IUnknown,(LPVOID*)&pUnk);
  mid.processid = GetCurrentProcessId();
  mid.objectid = (DWORD)pUnk; /* FIXME */
  IUnknown_Release(pUnk);
  memcpy(&mid.iid,riid,sizeof(mid.iid));
  md.dwDestContext	= dwDestContext;
  md.mshlflags		= mshlflags;
  hres = IStream_Write(pStm,&mid,sizeof(mid),&res);
  if (hres) return hres;
  hres = IStream_Write(pStm,&md,sizeof(md),&res);
  if (hres) return hres;

  if (SUCCEEDED(MARSHAL_Find_Stub(&mid,&pUnk))) {
      IUnknown_Release(pUnk);
      return S_OK;
  }
  hres = get_facbuf_for_iid(riid,&psfacbuf);
  if (hres) return hres;
  hres = IPSFactoryBuffer_CreateStub(psfacbuf,riid,pv,&stub);
  IPSFactoryBuffer_Release(psfacbuf);
  if (hres) {
    FIXME("Failed to create a stub for %s\n",debugstr_guid(riid));
    return hres;
  }
  IUnknown_QueryInterface((LPUNKNOWN)pv,riid,(LPVOID*)&pUnk);
  MARSHAL_Register_Stub(&mid,pUnk,stub);
  IUnknown_Release(pUnk);
  return S_OK;
}

HRESULT WINAPI
StdMarshalImpl_UnmarshalInterface(
  LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv
) {
  wine_marshal_id       mid;
  wine_marshal_data     md;
  ULONG			res;
  HRESULT		hres;
  IPSFactoryBuffer	*psfacbuf;
  IRpcProxyBuffer	*rpcproxy;
  IRpcChannelBuffer	*chanbuf;

  TRACE("(...,%s,....)\n",debugstr_guid(riid));
  hres = IStream_Read(pStm,&mid,sizeof(mid),&res);
  if (hres) return hres;
  hres = IStream_Read(pStm,&md,sizeof(md),&res);
  if (hres) return hres;
  if (SUCCEEDED(MARSHAL_Find_Stub(&mid,(LPUNKNOWN*)ppv))) {
      FIXME("Calling back to ourselves for %s!\n",debugstr_guid(riid));
      return S_OK;
  }
  hres = get_facbuf_for_iid(riid,&psfacbuf);
  if (hres) return hres;
  hres = IPSFactoryBuffer_CreateProxy(psfacbuf,NULL,riid,&rpcproxy,ppv);
  if (hres) {
    FIXME("Failed to create a proxy for %s\n",debugstr_guid(riid));
    return hres;
  }
  hres = PIPE_GetNewPipeBuf(&mid,&chanbuf);
  IPSFactoryBuffer_Release(psfacbuf);
  if (hres) {
    ERR("Failed to get an rpc channel buffer for %s\n",debugstr_guid(riid));
  } else {
    /* Connect the channel buffer to the proxy and release the no longer
     * needed proxy.
     * NOTE: The proxy should have taken an extra reference because it also
     * aggregates the object, so we can safely release our reference to it. */
    IRpcProxyBuffer_Connect(rpcproxy,chanbuf);
    IRpcProxyBuffer_Release(rpcproxy);
    /* IRpcProxyBuffer takes a reference on the channel buffer and
     * we no longer need it, so release it */
    IRpcChannelBuffer_Release(chanbuf);
  }
  return hres;
}

HRESULT WINAPI
StdMarshalImpl_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm) {
  FIXME("(), stub!\n");
  return S_OK;
}

HRESULT WINAPI
StdMarshalImpl_DisconnectObject(LPMARSHAL iface, DWORD dwReserved) {
  FIXME("(), stub!\n");
  return S_OK;
}

ICOM_VTABLE(IMarshal) stdmvtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    StdMarshalImpl_QueryInterface,
    StdMarshalImpl_AddRef,
    StdMarshalImpl_Release,
    StdMarshalImpl_GetUnmarshalClass,
    StdMarshalImpl_GetMarshalSizeMax,
    StdMarshalImpl_MarshalInterface,
    StdMarshalImpl_UnmarshalInterface,
    StdMarshalImpl_ReleaseMarshalData,
    StdMarshalImpl_DisconnectObject
};

/***********************************************************************
 *		CoGetStandardMarshal	[OLE32.@]
 *
 * When the COM library in the client process receives a marshalled
 * interface pointer, it looks for a CLSID to be used in creating a proxy
 * for the purposes of unmarshalling the packet. If the packet does not
 * contain a CLSID for the proxy, COM calls CoGetStandardMarshal, passing a
 * NULL pUnk value.
 * This function creates a standard proxy in the client process and returns
 * a pointer to that proxy's implementation of IMarshal.
 * COM uses this pointer to call CoUnmarshalInterface to retrieve the pointer
 * to the requested interface.
 */
HRESULT WINAPI
CoGetStandardMarshal(
  REFIID riid,IUnknown *pUnk,DWORD dwDestContext,LPVOID pvDestContext,
  DWORD mshlflags, LPMARSHAL *pMarshal
) {
  StdMarshalImpl *dm;

  if (pUnk == NULL) {
    FIXME("(%s,NULL,%lx,%p,%lx,%p), unimplemented yet.\n",
      debugstr_guid(riid),dwDestContext,pvDestContext,mshlflags,pMarshal
    );
    return E_FAIL;
  }
  TRACE("(%s,%p,%lx,%p,%lx,%p)\n",
    debugstr_guid(riid),pUnk,dwDestContext,pvDestContext,mshlflags,pMarshal
  );
  *pMarshal = HeapAlloc(GetProcessHeap(),0,sizeof(StdMarshalImpl));
  dm = (StdMarshalImpl*) *pMarshal;
  if (!dm) return E_FAIL;
  dm->lpvtbl		= &stdmvtbl;
  dm->ref		= 1;

  memcpy(&dm->iid,riid,sizeof(dm->iid));
  dm->dwDestContext	= dwDestContext;
  dm->pvDestContext	= pvDestContext;
  dm->mshlflags		= mshlflags;
  return S_OK;
}

/* Helper function for getting Marshaler */
static HRESULT WINAPI
_GetMarshaller(REFIID riid, IUnknown *pUnk,DWORD dwDestContext,
  void *pvDestContext, DWORD mshlFlags, LPMARSHAL *pMarshal
) {
  HRESULT hres;

  if (!pUnk)
      return E_POINTER;
  hres = IUnknown_QueryInterface(pUnk,&IID_IMarshal,(LPVOID*)pMarshal);
  if (hres)
    hres = CoGetStandardMarshal(riid,pUnk,dwDestContext,pvDestContext,mshlFlags,pMarshal);
  return hres;
}

/***********************************************************************
 *		CoGetMarshalSizeMax	[OLE32.@]
 */
HRESULT WINAPI
CoGetMarshalSizeMax(ULONG *pulSize, REFIID riid, IUnknown *pUnk,
  DWORD dwDestContext, void *pvDestContext, DWORD mshlFlags
) {
  HRESULT	hres;
  LPMARSHAL	pMarshal;

  hres = _GetMarshaller(riid,pUnk,dwDestContext,pvDestContext,mshlFlags,&pMarshal);
  if (hres)
    return hres;
  hres = IMarshal_GetMarshalSizeMax(pMarshal,riid,pUnk,dwDestContext,pvDestContext,mshlFlags,pulSize);
  *pulSize += sizeof(wine_marshal_id)+sizeof(wine_marshal_data)+sizeof(CLSID);
  IMarshal_Release(pMarshal);
  return hres;
}


/***********************************************************************
 *		CoMarshalInterface	[OLE32.@]
 */
HRESULT WINAPI
CoMarshalInterface( IStream *pStm, REFIID riid, IUnknown *pUnk,
  DWORD dwDestContext, void *pvDestContext, DWORD mshlflags
) {
  HRESULT 		hres;
  LPMARSHAL		pMarshal;
  CLSID			xclsid;
  ULONG			writeres;
  wine_marshal_id	mid;
  wine_marshal_data 	md;
  ULONG			res;
  IUnknown		*pUnknown;

  TRACE("(%p, %s, %p, %lx, %p, %lx)\n",
    pStm,debugstr_guid(riid),pUnk,dwDestContext,pvDestContext,mshlflags
  );
  STUBMGR_Start(); /* Just to be sure we have one running. */
  mid.processid = GetCurrentProcessId();
  IUnknown_QueryInterface(pUnk,&IID_IUnknown,(LPVOID*)&pUnknown);
  mid.objectid = (DWORD)pUnknown;
  IUnknown_Release(pUnknown);
  memcpy(&mid.iid,riid,sizeof(mid.iid));
  md.dwDestContext	= dwDestContext;
  md.mshlflags		= mshlflags;
  hres = IStream_Write(pStm,&mid,sizeof(mid),&res);
  if (hres) return hres;
  hres = IStream_Write(pStm,&md,sizeof(md),&res);
  if (hres) return hres;
  hres = _GetMarshaller(riid,pUnk,dwDestContext,pvDestContext,mshlflags,&pMarshal);
  if (hres) {
    FIXME("Failed to get marshaller, %lx?\n",hres);
    return hres;
  }
  hres = IMarshal_GetUnmarshalClass(pMarshal,riid,pUnk,dwDestContext,pvDestContext,mshlflags,&xclsid);
  if (hres) {
    FIXME("IMarshal:GetUnmarshalClass failed, %lx\n",hres);
    goto release_marshal;
  }
  hres = IStream_Write(pStm,&xclsid,sizeof(xclsid),&writeres);
  if (hres) {
    FIXME("Stream write failed, %lx\n",hres);
    goto release_marshal;
  }
  hres = IMarshal_MarshalInterface(pMarshal,pStm,riid,pUnk,dwDestContext,pvDestContext,mshlflags);
  if (hres) {
    if (IsEqualGUID(riid,&IID_IClassFactory)) {
	MESSAGE("\nERROR: You need to merge the 'winedefault.reg' file into your\n");
	MESSAGE("       Wine registry by running: `regedit winedefault.reg'\n\n");
    } else {
    	if (IsEqualGUID(riid,&IID_IOleObject)) {
	    ERR("WINE currently cannot marshal IOleObject interfaces. This means you cannot embed/link OLE objects between applications.\n");
	} else {
	    FIXME("Failed to marshal the interface %s, %lx?\n",debugstr_guid(riid),hres);
	}
    }
    goto release_marshal;
  }
release_marshal:
  IMarshal_Release(pMarshal);
  return hres;
}


/***********************************************************************
 *		CoUnmarshalInterface	[OLE32.@]
 */
HRESULT WINAPI
CoUnmarshalInterface(IStream *pStm, REFIID riid, LPVOID *ppv) {
  HRESULT 		hres;
  wine_marshal_id	mid;
  wine_marshal_data	md;
  ULONG			res;
  LPMARSHAL		pMarshal;
  LPUNKNOWN		pUnk;
  CLSID			xclsid;

  TRACE("(%p,%s,%p)\n",pStm,debugstr_guid(riid),ppv);

  hres = IStream_Read(pStm,&mid,sizeof(mid),&res);
  if (hres) {
      FIXME("Stream read 1 failed, %lx, (%ld of %d)\n",hres,res,sizeof(mid));
      return hres;
  }
  hres = IStream_Read(pStm,&md,sizeof(md),&res);
  if (hres) {
      FIXME("Stream read 2 failed, %lx, (%ld of %d)\n",hres,res,sizeof(md));
      return hres;
  }
  hres = IStream_Read(pStm,&xclsid,sizeof(xclsid),&res);
  if (hres) {
      FIXME("Stream read 3 failed, %lx, (%ld of %d)\n",hres,res,sizeof(xclsid));
      return hres;
  }
  hres=CoCreateInstance(&xclsid,NULL,CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,&IID_IMarshal,(void**)&pUnk);
  if (hres) {
      FIXME("Failed to create instance of unmarshaller %s.\n",debugstr_guid(&xclsid));
      return hres;
  }
  hres = _GetMarshaller(riid,pUnk,md.dwDestContext,NULL,md.mshlflags,&pMarshal);
  if (hres) {
      FIXME("Failed to get unmarshaller, %lx?\n",hres);
      return hres;
  }
  hres = IMarshal_UnmarshalInterface(pMarshal,pStm,riid,ppv);
  if (hres) {
    FIXME("Failed to Unmarshal the interface, %lx?\n",hres);
    goto release_marshal;
  }
release_marshal:
  IMarshal_Release(pMarshal);
  return hres;
}

/***********************************************************************
 *		CoReleaseMarshalData	[OLE32.@]
 */
HRESULT WINAPI
CoReleaseMarshalData(IStream *pStm) {
  HRESULT 		hres;
  wine_marshal_id	mid;
  wine_marshal_data	md;
  ULONG			res;
  LPMARSHAL		pMarshal;
  LPUNKNOWN		pUnk;
  CLSID			xclsid;

  TRACE("(%p)\n",pStm);

  hres = IStream_Read(pStm,&mid,sizeof(mid),&res);
  if (hres) {
      FIXME("Stream read 1 failed, %lx, (%ld of %d)\n",hres,res,sizeof(mid));
      return hres;
  }
  hres = IStream_Read(pStm,&md,sizeof(md),&res);
  if (hres) {
      FIXME("Stream read 2 failed, %lx, (%ld of %d)\n",hres,res,sizeof(md));
      return hres;
  }
  hres = IStream_Read(pStm,&xclsid,sizeof(xclsid),&res);
  if (hres) {
      FIXME("Stream read 3 failed, %lx, (%ld of %d)\n",hres,res,sizeof(xclsid));
      return hres;
  }
  hres=CoCreateInstance(&xclsid,NULL,CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,&IID_IMarshal,(void**)(char*)&pUnk);
  if (hres) {
      FIXME("Failed to create instance of unmarshaller %s.\n",debugstr_guid(&xclsid));
      return hres;
  }
  hres = IUnknown_QueryInterface(pUnk,&IID_IMarshal,(LPVOID*)(char*)&pMarshal);
  if (hres) {
      FIXME("Failed to get IMarshal iface, %lx?\n",hres);
      return hres;
  }
  hres = IMarshal_ReleaseMarshalData(pMarshal,pStm);
  if (hres) {
    FIXME("Failed to releasemarshaldata the interface, %lx?\n",hres);
  }
  IMarshal_Release(pMarshal);
  IUnknown_Release(pUnk);
  return hres;
}


/***********************************************************************
 *		CoMarshalInterThreadInterfaceInStream	[OLE32.@]
 *
 * Marshal an interface across threads in the same process.
 *
 * PARAMS
 *  riid  [I] Identifier of the interface to be marshalled.
 *  pUnk  [I] Pointer to IUnknown-derived interface that will be marshalled.
 *  ppStm [O] Pointer to IStream object that is created and then used to store the marshalled inteface.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: E_OUTOFMEMORY and other COM error codes
 *
 * SEE
 *   CoMarshalInterface(), CoUnmarshalInterface() and CoGetInterfaceAndReleaseStream()
 */
HRESULT WINAPI
CoMarshalInterThreadInterfaceInStream(
  REFIID riid, LPUNKNOWN pUnk, LPSTREAM * ppStm)
{
    ULARGE_INTEGER	xpos;
    LARGE_INTEGER		seekto;
    HRESULT		hres;

    TRACE("(%s, %p, %p)\n",debugstr_guid(riid), pUnk, ppStm);

    hres = CreateStreamOnHGlobal(0, TRUE, ppStm);
    if (FAILED(hres)) return hres;
    hres = CoMarshalInterface(*ppStm, riid, pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);

    /* FIXME: is this needed? */
    memset(&seekto,0,sizeof(seekto));
    IStream_Seek(*ppStm,seekto,SEEK_SET,&xpos);

    return hres;
}

/***********************************************************************
 *		CoGetInterfaceAndReleaseStream	[OLE32.@]
 *
 * Unmarshalls an inteface from a stream and then releases the stream.
 *
 * PARAMS
 *  pStm [I] Stream that contains the marshalled inteface.
 *  riid [I] Interface identifier of the object to unmarshall.
 *  ppv  [O] Address of pointer where the requested interface object will be stored.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A COM error code
 *
 * SEE
 *  CoMarshalInterThreadInterfaceInStream() and CoUnmarshalInteface()
 */
HRESULT WINAPI
CoGetInterfaceAndReleaseStream(LPSTREAM pStm,REFIID riid, LPVOID *ppv)
{
    HRESULT hres;

    TRACE("(%p, %s, %p)\n", pStm, debugstr_guid(riid), ppv);

    hres = CoUnmarshalInterface(pStm, riid, ppv);
    IStream_Release(pStm);
    return hres;
}

static HRESULT WINAPI
SMCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid, LPVOID *ppv) {
  *ppv = NULL;
  if (IsEqualIID(riid,&IID_IUnknown) || IsEqualIID(riid,&IID_IClassFactory)) {
    *ppv = (LPVOID)iface;
    return S_OK;
  }
  return E_NOINTERFACE;
}
static ULONG WINAPI SMCF_AddRef(LPCLASSFACTORY iface) { return 2; }
static ULONG WINAPI SMCF_Release(LPCLASSFACTORY iface) { return 1; }

static HRESULT WINAPI
SMCF_CreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv
) {
  if (IsEqualIID(riid,&IID_IMarshal)) {
      StdMarshalImpl	*dm;
      dm=(StdMarshalImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(StdMarshalImpl));
      if (!dm)
	  return E_FAIL;
      dm->lpvtbl	= &stdmvtbl;
      dm->ref		= 1;
      *ppv = (LPVOID)dm;
      return S_OK;
  }
  FIXME("(%s), not supported.\n",debugstr_guid(riid));
  return E_NOINTERFACE;
}

static HRESULT WINAPI
SMCF_LockServer(LPCLASSFACTORY iface, BOOL fLock) {
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) dfmarshalcfvtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    SMCF_QueryInterface,
    SMCF_AddRef,
    SMCF_Release,
    SMCF_CreateInstance,
    SMCF_LockServer
};
static ICOM_VTABLE(IClassFactory) *pdfmarshalcfvtbl = &dfmarshalcfvtbl;

HRESULT
MARSHAL_GetStandardMarshalCF(LPVOID *ppv) {
  *ppv = &pdfmarshalcfvtbl;
  return S_OK;
}
