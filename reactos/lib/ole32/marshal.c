/*
 *	Marshalling library
 *
 * Copyright 2002 Marcus Meissner
 * Copyright 2004 Mike Hearn, for CodeWeavers
 * Copyright 2004 Rob Shearman, for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wtypes.h"
#include "wine/unicode.h"

#include "compobj_private.h"

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
 *
 * In Windows, a different triple is used: OXID (apt id), OID (stub
 * manager id), IPID (interface ptr/stub id).
 *
 * OXIDs identify an apartment and are network scoped
 * OIDs identify a stub manager and are apartment scoped
 * IPIDs identify an interface stub and are apartment scoped
 */

inline static HRESULT
get_facbuf_for_iid(REFIID riid,IPSFactoryBuffer **facbuf) {
    HRESULT       hres;
    CLSID         pxclsid;

    if ((hres = CoGetPSClsid(riid,&pxclsid)))
	return hres;
    return CoGetClassObject(&pxclsid,CLSCTX_INPROC_SERVER,NULL,&IID_IPSFactoryBuffer,(LPVOID*)facbuf);
}

typedef struct _wine_marshal_data {
    DWORD	dwDestContext;
    DWORD	mshlflags;
} wine_marshal_data;

IRpcStubBuffer *mid_to_stubbuffer(wine_marshal_id *mid)
{
    IRpcStubBuffer *ret;
    struct stub_manager *m;

    if (!(m = get_stub_manager(mid->oxid, mid->oid)))
    {
        WARN("unknown OID %s\n", wine_dbgstr_longlong(mid->oid));
        return NULL;
    }

    ret = stub_manager_ipid_to_stubbuffer(m, &mid->ipid);

    stub_manager_int_release(m);
    return ret;
}

/* creates a new stub manager and sets mid->oid when mid->oid == 0 */
static HRESULT register_ifstub(wine_marshal_id *mid, REFIID riid, IUnknown *obj, IRpcStubBuffer *stub, BOOL tablemarshal)
{
    struct stub_manager *manager = NULL;
    struct ifstub       *ifstub;

    /* mid->oid of zero means create a new stub manager */
    
    if (mid->oid && (manager = get_stub_manager(mid->oxid, mid->oid)))
    {
        TRACE("registering new ifstub on pre-existing manager\n");
    }
    else
    {
        struct apartment *apt;
        
        TRACE("constructing new stub manager\n");

        apt = COM_ApartmentFromOXID(mid->oxid, TRUE);
        manager = new_stub_manager(apt, obj);
        COM_ApartmentRelease(apt);
        if (!manager) return E_OUTOFMEMORY;

        mid->oid = manager->oid;
    }

    ifstub = stub_manager_new_ifstub(manager, stub, obj, riid, tablemarshal);
    if (!ifstub)
    {
        stub_manager_int_release(manager);
        /* FIXME: should we do another release to completely destroy the
         * stub manager? */
        return E_OUTOFMEMORY;
    }

    if (!tablemarshal) stub_manager_ext_addref(manager, 1);

    mid->ipid = ifstub->ipid;

    stub_manager_int_release(manager);
    return S_OK;
}



/* Client-side identity of the server object */

static void proxy_manager_destroy(struct proxy_manager * This);
static HRESULT proxy_manager_find_ifproxy(struct proxy_manager * This, REFIID riid, struct ifproxy ** ifproxy_found);

static HRESULT WINAPI ClientIdentity_QueryInterface(IInternalUnknown * iface, REFIID riid, void ** ppv)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    HRESULT hr;
    struct ifproxy * ifproxy;

    TRACE("%s\n", debugstr_guid(riid));

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInternalUnknown))
    {
        *ppv = (void *)iface;
        IInternalUnknown_AddRef(iface);
        return S_OK;
    }

    hr = proxy_manager_find_ifproxy(This, riid, &ifproxy);
    if (hr == S_OK)
    {
        *ppv = ifproxy->iface;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    FIXME("interface not found %s\n", debugstr_guid(riid));

    /* FIXME: call IRemUnknown::RemQueryInterface */
    return E_NOINTERFACE;
}

static ULONG WINAPI ClientIdentity_AddRef(IInternalUnknown * iface)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    TRACE("%p - before %ld\n", iface, This->refs);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI ClientIdentity_Release(IInternalUnknown * iface)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    TRACE("%p - after %ld\n", iface, refs);
    if (!refs)
        proxy_manager_destroy(This);
    return refs;
}

static HRESULT WINAPI ClientIdentity_QueryInternalInterface(IInternalUnknown * iface, REFIID riid, void ** ppv)
{
    FIXME("(%s, %p): stub!\n", debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IInternalUnknownVtbl ClientIdentity_Vtbl =
{
    ClientIdentity_QueryInterface,
    ClientIdentity_AddRef,
    ClientIdentity_Release,
    ClientIdentity_QueryInternalInterface
};

static HRESULT ifproxy_get_public_ref(struct ifproxy * This)
{
    /* FIXME: call IRemUnknown::RemAddRef if necessary */
    return S_OK;
}

static HRESULT ifproxy_release_public_refs(struct ifproxy * This)
{
    /* FIXME: call IRemUnknown::RemRelease */
    return S_OK;
}

static void ifproxy_disconnect(struct ifproxy * This)
{
    IRpcProxyBuffer_Disconnect(This->proxy);
}

static void ifproxy_destroy(struct ifproxy * This)
{
    /* release public references to this object so that the stub can know
     * when to destroy itself */
    ifproxy_release_public_refs(This);

    list_remove(&This->entry);

    if (This->proxy) IRpcProxyBuffer_Release(This->proxy);
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT proxy_manager_construct(APARTMENT * apt, OXID oxid, OID oid, IRpcChannelBuffer * channel, struct proxy_manager ** proxy_manager)
{
    struct proxy_manager * This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &ClientIdentity_Vtbl;

    list_init(&This->entry);
    list_init(&This->interfaces);

    InitializeCriticalSection(&This->cs);

    /* the apartment the object was unmarshaled into */
    This->parent = apt;

    /* the source apartment and id of the object */
    This->oxid = oxid;
    This->oid = oid;

    This->refs = 0; /* will be incremented on creation of first proxy */

    This->chan = channel; /* FIXME: we should take the binding strings and construct the channel in this function */

    EnterCriticalSection(&apt->cs);
    list_add_head(&apt->proxies, &This->entry);
    LeaveCriticalSection(&apt->cs);

    *proxy_manager = This;
    return S_OK;
}

static HRESULT proxy_manager_create_ifproxy(struct proxy_manager * This, IPID ipid, REFIID riid, ULONG cPublicRefs, struct ifproxy ** iif_out)
{
    HRESULT hr;
    IPSFactoryBuffer * psfb;
    struct ifproxy * ifproxy = HeapAlloc(GetProcessHeap(), 0, sizeof(*ifproxy));
    if (!ifproxy) return E_OUTOFMEMORY;

    list_init(&ifproxy->entry);

    ifproxy->ipid = ipid;
    ifproxy->iid = *riid;
    ifproxy->refs = cPublicRefs;
    ifproxy->proxy = NULL;

    hr = get_facbuf_for_iid(riid, &psfb);
    if (hr == S_OK)
    {
        /* important note: the outer unknown is set to the proxy manager.
         * This ensures the COM identity rules are not violated, by having a
         * one-to-one mapping of objects on the proxy side to objects on the
         * stub side, no matter which interface you view the object through */
        hr = IPSFactoryBuffer_CreateProxy(psfb, (IUnknown *)&This->lpVtbl, riid,
                                          &ifproxy->proxy, &ifproxy->iface);
        IPSFactoryBuffer_Release(psfb);
    }

    if (hr == S_OK)
        hr = IRpcProxyBuffer_Connect(ifproxy->proxy, This->chan);

    /* get at least one external reference to the object to keep it alive */
    if (hr == S_OK)
        hr = ifproxy_get_public_ref(ifproxy);

    if (hr == S_OK)
    {
        EnterCriticalSection(&This->cs);
        list_add_tail(&This->interfaces, &ifproxy->entry);
        LeaveCriticalSection(&This->cs);

        *iif_out = ifproxy;
    }
    else
        ifproxy_destroy(ifproxy);

    return hr;
}

static HRESULT proxy_manager_find_ifproxy(struct proxy_manager * This, REFIID riid, struct ifproxy ** ifproxy_found)
{
    HRESULT hr = E_NOINTERFACE; /* assume not found */
    struct list * cursor;

    EnterCriticalSection(&This->cs);
    LIST_FOR_EACH(cursor, &This->interfaces)
    {
        struct ifproxy * ifproxy = LIST_ENTRY(cursor, struct ifproxy, entry);
        if (IsEqualIID(riid, &ifproxy->iid))
        {
            *ifproxy_found = ifproxy;
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static void proxy_manager_disconnect(struct proxy_manager * This)
{
    struct list * cursor;

    EnterCriticalSection(&This->cs);

    LIST_FOR_EACH(cursor, &This->interfaces)
    {
        struct ifproxy * ifproxy = LIST_ENTRY(cursor, struct ifproxy, entry);
        ifproxy_disconnect(ifproxy);
    }

    /* apartment is being destroyed so don't keep a pointer around to it */
    This->parent = NULL;

    LeaveCriticalSection(&This->cs);
}

static void proxy_manager_destroy(struct proxy_manager * This)
{
    struct list * cursor;

    if (This->parent)
    {
        EnterCriticalSection(&This->parent->cs);

        /* remove ourself from the list of proxy objects in the apartment */
        LIST_FOR_EACH(cursor, &This->parent->proxies)
        {
            if (cursor == &This->entry)
            {
                list_remove(&This->entry);
                break;
            }
        }

        LeaveCriticalSection(&This->parent->cs);
    }

    /* destroy all of the interface proxies */
    while (!(cursor = list_head(&This->interfaces)))
    {
        struct ifproxy * ifproxy = LIST_ENTRY(cursor, struct ifproxy, entry);
        ifproxy_destroy(ifproxy);
    }

    IRpcChannelBuffer_Release(This->chan);

    DeleteCriticalSection(&This->cs);

    HeapFree(GetProcessHeap(), 0, This);
}

static BOOL find_proxy_manager(APARTMENT * apt, OXID oxid, OID oid, struct proxy_manager ** proxy_found)
{
    BOOL found = FALSE;
    struct list * cursor;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH(cursor, &apt->proxies)
    {
        struct proxy_manager * proxy = LIST_ENTRY(cursor, struct proxy_manager, entry);
        if ((oxid == proxy->oxid) && (oid == proxy->oid))
        {
            *proxy_found = proxy;
            found = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);
    return found;
}

HRESULT MARSHAL_Disconnect_Proxies(APARTMENT *apt)
{
    struct list * cursor;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH(cursor, &apt->proxies)
    {
        struct proxy_manager * proxy = LIST_ENTRY(cursor, struct proxy_manager, entry);
        proxy_manager_disconnect(proxy);
    }
    LeaveCriticalSection(&apt->cs);

    return S_OK;
}

/********************** StdMarshal implementation ****************************/
typedef struct _StdMarshalImpl {
  IMarshalVtbl	*lpvtbl;
  DWORD			ref;

  IID			iid;
  DWORD			dwDestContext;
  LPVOID		pvDestContext;
  DWORD			mshlflags;
} StdMarshalImpl;

static HRESULT WINAPI
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

static ULONG WINAPI
StdMarshalImpl_AddRef(LPMARSHAL iface) {
  StdMarshalImpl *This = (StdMarshalImpl *)iface;
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI
StdMarshalImpl_Release(LPMARSHAL iface) {
  StdMarshalImpl *This = (StdMarshalImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  if (!ref) HeapFree(GetProcessHeap(),0,This);
  return ref;
}

static HRESULT WINAPI
StdMarshalImpl_GetUnmarshalClass(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid
) {
  memcpy(pCid,&CLSID_DfMarshal,sizeof(CLSID_DfMarshal));
  return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_GetMarshalSizeMax(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize
) {
  *pSize = sizeof(wine_marshal_id)+sizeof(wine_marshal_data);
  return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_MarshalInterface(
  LPMARSHAL iface, IStream *pStm,REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags
) {
  wine_marshal_id       mid;
  wine_marshal_data     md;
  IUnknown             *pUnk;  
  ULONG                 res;
  HRESULT               hres;
  IRpcStubBuffer       *stubbuffer;
  IPSFactoryBuffer     *psfacbuf;
  BOOL                  tablemarshal;
  struct stub_manager  *manager;
    
  TRACE("(...,%s,...)\n",debugstr_guid(riid));

  start_apartment_listener_thread(); /* just to be sure we have one running. */

  hres = get_facbuf_for_iid(riid,&psfacbuf);
  if (hres) return hres;

  hres = IPSFactoryBuffer_CreateStub(psfacbuf,riid,pv,&stubbuffer);
  IPSFactoryBuffer_Release(psfacbuf);
  if (hres) {
    FIXME("Failed to create an RpcStubBuffer from PSFactory for %s\n",debugstr_guid(riid));
    return hres;
  }

  tablemarshal = ((mshlflags & MSHLFLAGS_TABLESTRONG) || (mshlflags & MSHLFLAGS_TABLEWEAK));
  if (tablemarshal) FIXME("table marshalling unimplemented\n");

  /* now fill out the MID */
  mid.oxid = COM_CurrentApt()->oxid;
 
  IUnknown_QueryInterface((LPUNKNOWN)pv, riid, (LPVOID*)&pUnk);
 
  if ((manager = get_stub_manager_from_object(mid.oxid, pUnk)))
  {
      mid.oid = manager->oid;
      stub_manager_int_release(manager);
  }
  else
  {
      mid.oid = 0;              /* will be set by register_ifstub */
  }
 
  hres = register_ifstub(&mid, riid, pUnk, stubbuffer, tablemarshal);
  
  IUnknown_Release(pUnk);
  
  if (hres)
  {
    FIXME("Failed to create ifstub, hres=0x%lx\n", hres);
    return hres;
  }

  hres = IStream_Write(pStm,&mid,sizeof(mid),&res);
  if (hres) return hres;

  /* and then the marshal data */
  md.dwDestContext      = dwDestContext;
  md.mshlflags          = mshlflags;
  hres = IStream_Write(pStm,&md,sizeof(md),&res);
  if (hres) return hres;
   
  return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_UnmarshalInterface(LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv)
{
  struct stub_manager  *stubmgr;
  wine_marshal_id       mid;
  wine_marshal_data     md;
  ULONG			res;
  HRESULT		hres;
  IRpcChannelBuffer	*chanbuf;
  struct proxy_manager *proxy_manager;
  APARTMENT *apt = COM_CurrentApt();

  TRACE("(...,%s,....)\n",debugstr_guid(riid));

  if (!apt) return CO_E_NOTINITIALIZED;

  hres = IStream_Read(pStm,&mid,sizeof(mid),&res);
  if (hres) return hres;
  hres = IStream_Read(pStm,&md,sizeof(md),&res);
  if (hres) return hres;
  
  /* check if we're marshalling back to ourselves */
  /* FIXME: commented out until we can get the tests passing with it uncommented. */
  if (/*(apt->oxid == mid.oxid) &&*/ (stubmgr = get_stub_manager(mid.oxid, mid.oid)))
  {
      TRACE("Unmarshalling object marshalled in same apartment for iid %s, returning original object %p\n", debugstr_guid(riid), stubmgr->object);
    
      hres = IUnknown_QueryInterface(stubmgr->object, riid, ppv);
      if ((md.mshlflags & MSHLFLAGS_TABLESTRONG) || (md.mshlflags & MSHLFLAGS_TABLEWEAK))
          FIXME("table marshalling unimplemented\n");
      
      /* unref the ifstub. FIXME: only do this on success? */
      stub_manager_ext_release(stubmgr, 1);

      stub_manager_int_release(stubmgr);
      return hres;
  }

  if (!find_proxy_manager(apt, mid.oxid, mid.oid, &proxy_manager))
  {
    hres = PIPE_GetNewPipeBuf(&mid,&chanbuf);
    if (hres == S_OK)
      hres = proxy_manager_construct(apt, mid.oxid, mid.oid, chanbuf, &proxy_manager);
  }

  if (hres == S_OK)
  {
    struct ifproxy * ifproxy;
    hres = proxy_manager_find_ifproxy(proxy_manager, riid, &ifproxy);
    if (hres == S_OK)
      IUnknown_AddRef((IUnknown *)ifproxy->iface);
    else if (hres == E_NOINTERFACE)
      hres = proxy_manager_create_ifproxy(proxy_manager, mid.ipid, riid, 1, &ifproxy);

    if (hres == S_OK)
      *ppv = ifproxy->iface; /* AddRef'd above */
  }

  return hres;
}

static HRESULT WINAPI
StdMarshalImpl_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm) {
    wine_marshal_id      mid;
    ULONG                res;
    HRESULT              hres;
    struct stub_manager *stubmgr;

    TRACE("iface=%p, pStm=%p\n", iface, pStm);
    
    hres = IStream_Read(pStm,&mid,sizeof(mid),&res);
    if (hres) return hres;

    if (!(stubmgr = get_stub_manager(mid.oxid, mid.oid)))
    {
        ERR("could not map MID to stub manager, oxid=%s, oid=%s\n",
            wine_dbgstr_longlong(mid.oxid), wine_dbgstr_longlong(mid.oid));
        return RPC_E_INVALID_OBJREF;
    }
    
    stub_manager_ext_release(stubmgr, 1);

    stub_manager_int_release(stubmgr);

    return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_DisconnectObject(LPMARSHAL iface, DWORD dwReserved) {
  FIXME("(), stub!\n");
  return S_OK;
}

IMarshalVtbl stdmvtbl = {
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

static HRESULT StdMarshalImpl_Construct(REFIID riid, void** ppvObject)
{
    StdMarshalImpl * pStdMarshal = 
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StdMarshalImpl));
    if (!pStdMarshal)
        return E_OUTOFMEMORY;
    pStdMarshal->lpvtbl = &stdmvtbl;
    pStdMarshal->ref = 0;
    return IMarshal_QueryInterface((IMarshal*)pStdMarshal, riid, ppvObject);
}

/***********************************************************************
 *		CoGetStandardMarshal	[OLE32.@]
 *
 * Gets or creates a standard marshal object.
 *
 * PARAMS
 *  riid          [I] Interface identifier of the pUnk object.
 *  pUnk          [I] Optional. Object to get the marshal object for.
 *  dwDestContext [I] Destination. Used to enable or disable optimizations.
 *  pvDestContext [I] Reserved. Must be NULL.
 *  mshlflags     [I] Flags affecting the marshaling process.
 *  ppMarshal     [O] Address where marshal object will be stored.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *
 * The function retrieves the IMarshal object associated with an object if
 * that object is currently an active stub, otherwise a new marshal object is
 * created.
 */
HRESULT WINAPI CoGetStandardMarshal(REFIID riid, IUnknown *pUnk,
                                    DWORD dwDestContext, LPVOID pvDestContext,
                                    DWORD mshlflags, LPMARSHAL *ppMarshal)
{
  StdMarshalImpl *dm;

  if (pUnk == NULL) {
    FIXME("(%s,NULL,%lx,%p,%lx,%p), unimplemented yet.\n",
      debugstr_guid(riid),dwDestContext,pvDestContext,mshlflags,ppMarshal
    );
    return E_FAIL;
  }
  TRACE("(%s,%p,%lx,%p,%lx,%p)\n",
    debugstr_guid(riid),pUnk,dwDestContext,pvDestContext,mshlflags,ppMarshal
  );
  *ppMarshal = HeapAlloc(GetProcessHeap(),0,sizeof(StdMarshalImpl));
  dm = (StdMarshalImpl*) *ppMarshal;
  if (!dm) return E_FAIL;
  dm->lpvtbl		= &stdmvtbl;
  dm->ref		= 1;

  memcpy(&dm->iid,riid,sizeof(dm->iid));
  dm->dwDestContext	= dwDestContext;
  dm->pvDestContext	= pvDestContext;
  dm->mshlflags		= mshlflags;
  return S_OK;
}

/***********************************************************************
 *		get_marshaler	[internal]
 *
 * Retrieves an IMarshal interface for an object.
 */
static HRESULT get_marshaler(REFIID riid, IUnknown *pUnk, DWORD dwDestContext,
                             void *pvDestContext, DWORD mshlFlags,
                             LPMARSHAL *pMarshal)
{
    HRESULT hr;

    if (!pUnk)
        return E_POINTER;
    hr = IUnknown_QueryInterface(pUnk, &IID_IMarshal, (LPVOID*)pMarshal);
    if (hr)
        hr = CoGetStandardMarshal(riid, pUnk, dwDestContext, pvDestContext,
                                  mshlFlags, pMarshal);
    return hr;
}

/***********************************************************************
 *		get_unmarshaler_from_stream	[internal]
 *
 * Creates an IMarshal* object according to the data marshaled to the stream.
 * The function leaves the stream pointer at the start of the data written
 * to the stream by the IMarshal* object.
 */
static HRESULT get_unmarshaler_from_stream(IStream *stream, IMarshal **marshal)
{
    HRESULT hr;
    ULONG res;
    OBJREF objref;

    /* read common OBJREF header */
    hr = IStream_Read(stream, &objref, FIELD_OFFSET(OBJREF, u_objref), &res);
    if (hr || (res != FIELD_OFFSET(OBJREF, u_objref)))
    {
        ERR("Failed to read common OBJREF header, 0x%08lx\n", hr);
        return STG_E_READFAULT;
    }

    /* sanity check on header */
    if (objref.signature != OBJREF_SIGNATURE)
    {
        ERR("Bad OBJREF signature 0x%08lx\n", objref.signature);
        return RPC_E_INVALID_OBJREF;
    }

    /* FIXME: handler marshaling */
    if (objref.flags & OBJREF_STANDARD)
    {
        TRACE("Using standard unmarshaling\n");
        hr = StdMarshalImpl_Construct(&IID_IMarshal, (LPVOID*)marshal);
    }
    else if (objref.flags & OBJREF_CUSTOM)
    {
        ULONG custom_header_size = FIELD_OFFSET(OBJREF, u_objref.u_custom.size) - 
                                   FIELD_OFFSET(OBJREF, u_objref.u_custom);
        TRACE("Using custom unmarshaling\n");
        /* read constant sized OR_CUSTOM data from stream */
        hr = IStream_Read(stream, &objref.u_objref.u_custom,
                          custom_header_size, &res);
        if (hr || (res != custom_header_size))
        {
            ERR("Failed to read OR_CUSTOM header, 0x%08lx\n", hr);
            return STG_E_READFAULT;
        }
        /* now create the marshaler specified in the stream */
        hr = CoCreateInstance(&objref.u_objref.u_custom.clsid, NULL,
                              CLSCTX_INPROC_SERVER, &IID_IMarshal,
                              (LPVOID*)marshal);
    }
    else
    {
        FIXME("Invalid or unimplemented marshaling type specified: %lx\n",
            objref.flags);
        return RPC_E_INVALID_OBJREF;
    }

    if (hr)
        ERR("Failed to create marshal, 0x%08lx\n", hr);

    return hr;
}

/***********************************************************************
 *		CoGetMarshalSizeMax	[OLE32.@]
 *
 * Gets the maximum amount of data that will be needed by a marshal.
 *
 * PARAMS
 *  pulSize       [O] Address where maximum marshal size will be stored.
 *  riid          [I] Identifier of the interface to marshal.
 *  pUnk          [I] Pointer to the object to marshal.
 *  dwDestContext [I] Destination. Used to enable or disable optimizations.
 *  pvDestContext [I] Reserved. Must be NULL.
 *  mshlFlags     [I] Flags that affect the marshaling. See CoMarshalInterface().
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoMarshalInterface().
 */
HRESULT WINAPI CoGetMarshalSizeMax(ULONG *pulSize, REFIID riid, IUnknown *pUnk,
                                   DWORD dwDestContext, void *pvDestContext,
                                   DWORD mshlFlags)
{
    HRESULT hr;
    LPMARSHAL pMarshal;
    CLSID marshaler_clsid;

    hr = get_marshaler(riid, pUnk, dwDestContext, pvDestContext, mshlFlags, &pMarshal);
    if (hr)
        return hr;

    hr = IMarshal_GetUnmarshalClass(pMarshal, riid, pUnk, dwDestContext,
                                    pvDestContext, mshlFlags, &marshaler_clsid);
    if (hr)
    {
        ERR("IMarshal::GetUnmarshalClass failed, 0x%08lx\n", hr);
        IMarshal_Release(pMarshal);
        return hr;
    }

    hr = IMarshal_GetMarshalSizeMax(pMarshal, riid, pUnk, dwDestContext,
                                    pvDestContext, mshlFlags, pulSize);
    /* add on the size of the common header */
    *pulSize += FIELD_OFFSET(OBJREF, u_objref);

    /* if custom marshaling, add on size of custom header */
    if (!IsEqualCLSID(&marshaler_clsid, &CLSID_DfMarshal))
        *pulSize += FIELD_OFFSET(OBJREF, u_objref.u_custom.size) - 
                    FIELD_OFFSET(OBJREF, u_objref.u_custom);

    IMarshal_Release(pMarshal);
    return hr;
}


/***********************************************************************
 *		CoMarshalInterface	[OLE32.@]
 *
 * Marshals an interface into a stream so that the object can then be
 * unmarshaled from another COM apartment and used remotely.
 *
 * PARAMS
 *  pStream       [I] Stream the object will be marshaled into.
 *  riid          [I] Identifier of the interface to marshal.
 *  pUnk          [I] Pointer to the object to marshal.
 *  dwDestContext [I] Destination. Used to enable or disable optimizations.
 *  pvDestContext [I] Reserved. Must be NULL.
 *  mshlFlags     [I] Flags that affect the marshaling. See notes.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *
 * The mshlFlags parameter can take one or more of the following flags:
 *| MSHLFLAGS_NORMAL - Unmarshal once, releases stub on last proxy release.
 *| MSHLFLAGS_TABLESTRONG - Unmarshal many, release when CoReleaseMarshalData() called.
 *| MSHLFLAGS_TABLEWEAK - Unmarshal many, releases stub on last proxy release.
 *| MSHLFLAGS_NOPING - No automatic garbage collection (and so reduces network traffic).
 *
 * If a marshaled object is not unmarshaled, then CoReleaseMarshalData() must
 * be called in order to release the resources used in the marshaling.
 *
 * SEE ALSO
 *  CoUnmarshalInterface(), CoReleaseMarshalData().
 */
HRESULT WINAPI CoMarshalInterface(IStream *pStream, REFIID riid, IUnknown *pUnk,
                                  DWORD dwDestContext, void *pvDestContext,
                                  DWORD mshlFlags)
{
    HRESULT	hr;
    CLSID marshaler_clsid;
    OBJREF objref;
    IStream * pMarshalStream = NULL;
    LPMARSHAL pMarshal;

    TRACE("(%p, %s, %p, %lx, %p, %lx)\n", pStream, debugstr_guid(riid), pUnk,
        dwDestContext, pvDestContext, mshlFlags);

    if (pUnk == NULL)
        return E_INVALIDARG;

    objref.signature = OBJREF_SIGNATURE;
    objref.iid = *riid;

    /* get the marshaler for the specified interface */
    hr = get_marshaler(riid, pUnk, dwDestContext, pvDestContext, mshlFlags, &pMarshal);
    if (hr)
    {
        ERR("Failed to get marshaller, 0x%08lx\n", hr);
        return hr;
    }

    hr = IMarshal_GetUnmarshalClass(pMarshal, riid, pUnk, dwDestContext,
                                    pvDestContext, mshlFlags, &marshaler_clsid);
    if (hr)
    {
        ERR("IMarshal::GetUnmarshalClass failed, 0x%08lx\n", hr);
        goto cleanup;
    }

    /* FIXME: implement handler marshaling too */
    if (IsEqualCLSID(&marshaler_clsid, &CLSID_DfMarshal))
    {
        TRACE("Using standard marshaling\n");
        objref.flags = OBJREF_STANDARD;
        pMarshalStream = pStream;
    }
    else
    {
        TRACE("Using custom marshaling\n");
        objref.flags = OBJREF_CUSTOM;
        /* we do custom marshaling into a memory stream so that we know what
         * size to write into the OR_CUSTOM header */
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pMarshalStream);
        if (hr)
        {
            ERR("CreateStreamOnHGLOBAL failed with 0x%08lx\n", hr);
            goto cleanup;
        }
    }

    /* write the common OBJREF header to the stream */
    hr = IStream_Write(pStream, &objref, FIELD_OFFSET(OBJREF, u_objref), NULL);
    if (hr)
    {
        ERR("Failed to write OBJREF header to stream, 0x%08lx\n", hr);
        goto cleanup;
    }

    TRACE("Calling IMarshal::MarshalInterace\n");
    /* call helper object to do the actual marshaling */
    hr = IMarshal_MarshalInterface(pMarshal, pMarshalStream, riid, pUnk, dwDestContext,
                                   pvDestContext, mshlFlags);

    if (hr)
    {
        ERR("Failed to marshal the interface %s, %lx\n", debugstr_guid(riid), hr);
        goto cleanup;
    }

    if (objref.flags & OBJREF_CUSTOM)
    {
        ULONG custom_header_size = FIELD_OFFSET(OBJREF, u_objref.u_custom.size) - 
                                   FIELD_OFFSET(OBJREF, u_objref.u_custom);
        HGLOBAL hGlobal;
        LPVOID data;
        hr = GetHGlobalFromStream(pMarshalStream, &hGlobal);
        if (hr)
        {
            ERR("Couldn't get HGLOBAL from stream\n");
            hr = E_UNEXPECTED;
            goto cleanup;
        }
        objref.u_objref.u_custom.cbExtension = 0;
        objref.u_objref.u_custom.size = GlobalSize(hGlobal);
        /* write constant sized OR_CUSTOM data into stream */
        hr = IStream_Write(pStream, &objref.u_objref.u_custom,
                          custom_header_size, NULL);
        if (hr)
        {
            ERR("Failed to write OR_CUSTOM header to stream with 0x%08lx\n", hr);
            goto cleanup;
        }

        data = GlobalLock(hGlobal);
        if (!data)
        {
            ERR("GlobalLock failed\n");
            hr = E_UNEXPECTED;
            goto cleanup;
        }
        /* write custom marshal data */
        hr = IStream_Write(pStream, data, objref.u_objref.u_custom.size, NULL);
        if (hr)
        {
            ERR("Failed to write custom marshal data with 0x%08lx\n", hr);
            goto cleanup;
        }
        GlobalUnlock(hGlobal);
    }

cleanup:
    if (pMarshalStream && (objref.flags & OBJREF_CUSTOM))
        IStream_Release(pMarshalStream);
    IMarshal_Release(pMarshal);
    return hr;
}

/***********************************************************************
 *		CoUnmarshalInterface	[OLE32.@]
 *
 * Unmarshals an object from a stream by creating a proxy to the remote
 * object, if necessary.
 *
 * PARAMS
 *
 *  pStream [I] Stream containing the marshaled object.
 *  riid    [I] Interface identifier of the object to create a proxy to.
 *  ppv     [O] Address where proxy will be stored.
 *
 * RETURNS
 *
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoMarshalInterface().
 */
HRESULT WINAPI CoUnmarshalInterface(IStream *pStream, REFIID riid, LPVOID *ppv)
{
    HRESULT	hr;
    LPMARSHAL pMarshal;

    TRACE("(%p, %s, %p)\n", pStream, debugstr_guid(riid), ppv);

    hr = get_unmarshaler_from_stream(pStream, &pMarshal);
    if (hr != S_OK)
        return hr;

    /* call the helper object to do the actual unmarshaling */
    hr = IMarshal_UnmarshalInterface(pMarshal, pStream, riid, ppv);
    if (hr)
        ERR("IMarshal::UnmarshalInterface failed, 0x%08lx\n", hr);

    IMarshal_Release(pMarshal);
    return hr;
}

/***********************************************************************
 *		CoReleaseMarshalData	[OLE32.@]
 *
 * Releases resources associated with an object that has been marshaled into
 * a stream.
 *
 * PARAMS
 *
 *  pStream [I] The stream that the object has been marshaled into.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT error code.
 *
 * NOTES
 * 
 * Call this function to release resources associated with a normal or
 * table-weak marshal that will not be unmarshaled, and all table-strong
 * marshals when they are no longer needed.
 *
 * SEE ALSO
 *  CoMarshalInterface(), CoUnmarshalInterface().
 */
HRESULT WINAPI CoReleaseMarshalData(IStream *pStream)
{
    HRESULT	hr;
    LPMARSHAL pMarshal;

    TRACE("(%p)\n", pStream);

    hr = get_unmarshaler_from_stream(pStream, &pMarshal);
    if (hr != S_OK)
        return hr;

    /* call the helper object to do the releasing of marshal data */
    hr = IMarshal_ReleaseMarshalData(pMarshal, pStream);
    if (hr)
        ERR("IMarshal::ReleaseMarshalData failed with error 0x%08lx\n", hr);

    IMarshal_Release(pMarshal);
    return hr;
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
 * SEE ALSO
 *   CoMarshalInterface(), CoUnmarshalInterface() and CoGetInterfaceAndReleaseStream()
 */
HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(
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
 * SEE ALSO
 *  CoMarshalInterThreadInterfaceInStream() and CoUnmarshalInteface()
 */
HRESULT WINAPI CoGetInterfaceAndReleaseStream(LPSTREAM pStm, REFIID riid,
                                              LPVOID *ppv)
{
    HRESULT hres;

    TRACE("(%p, %s, %p)\n", pStm, debugstr_guid(riid), ppv);

    hres = CoUnmarshalInterface(pStm, riid, ppv);
    IStream_Release(pStm);
    return hres;
}

static HRESULT WINAPI StdMarshalCF_QueryInterface(LPCLASSFACTORY iface,
                                                  REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = (LPVOID)iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI StdMarshalCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI StdMarshalCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI StdMarshalCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
  if (IsEqualIID(riid,&IID_IMarshal))
    return StdMarshalImpl_Construct(riid, ppv);

  FIXME("(%s), not supported.\n",debugstr_guid(riid));
  return E_NOINTERFACE;
}

static HRESULT WINAPI StdMarshalCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static IClassFactoryVtbl StdMarshalCFVtbl =
{
    StdMarshalCF_QueryInterface,
    StdMarshalCF_AddRef,
    StdMarshalCF_Release,
    StdMarshalCF_CreateInstance,
    StdMarshalCF_LockServer
};
static IClassFactoryVtbl *StdMarshalCF = &StdMarshalCFVtbl;

HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv)
{
    *ppv = &StdMarshalCF;
    return S_OK;
}

/***********************************************************************
 *		CoMarshalHresult	[OLE32.@]
 *
 * Marshals an HRESULT value into a stream.
 *
 * PARAMS
 *  pStm    [I] Stream that hresult will be marshalled into.
 *  hresult [I] HRESULT to be marshalled.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A COM error code
 *
 * SEE ALSO
 *  CoUnmarshalHresult().
 */
HRESULT WINAPI CoMarshalHresult(LPSTREAM pStm, HRESULT hresult)
{
    return IStream_Write(pStm, &hresult, sizeof(hresult), NULL);
}

/***********************************************************************
 *		CoUnmarshalHresult	[OLE32.@]
 *
 * Unmarshals an HRESULT value from a stream.
 *
 * PARAMS
 *  pStm     [I] Stream that hresult will be unmarshalled from.
 *  phresult [I] Pointer to HRESULT where the value will be unmarshalled to.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A COM error code
 *
 * SEE ALSO
 *  CoMarshalHresult().
 */
HRESULT WINAPI CoUnmarshalHresult(LPSTREAM pStm, HRESULT * phresult)
{
    return IStream_Read(pStm, phresult, sizeof(*phresult), NULL);
}
