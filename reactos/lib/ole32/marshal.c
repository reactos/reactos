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

/* number of refs given out for normal marshaling */
#define NORMALEXTREFS 1 /* FIXME: this should be 5, but we have to wait for IRemUnknown support first */

/* private flag indicating that the caller does not want to notify the stub
 * when the proxy disconnects or is destroyed */
#define SORFP_NOLIFETIMEMGMT SORF_OXRES1

static HRESULT unmarshal_object(const STDOBJREF *stdobjref, APARTMENT *apt, REFIID riid, void **object);

/* Marshalling just passes a unique identifier to the remote client,
 * that makes it possible to find the passed interface again.
 *
 * So basically we need a set of values that make it unique.
 *
 * Note that the IUnknown_QI(ob,xiid,&ppv) always returns the SAME ppv value!
 *
 * A triple is used: OXID (apt id), OID (stub manager id),
 * IPID (interface ptr/stub id).
 *
 * OXIDs identify an apartment and are network scoped
 * OIDs identify a stub manager and are apartment scoped
 * IPIDs identify an interface stub and are apartment scoped
 */

inline static HRESULT get_facbuf_for_iid(REFIID riid, IPSFactoryBuffer **facbuf)
{
    HRESULT       hr;
    CLSID         clsid;

    if ((hr = CoGetPSClsid(riid, &clsid)))
        return hr;
    return CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL,
        &IID_IPSFactoryBuffer, (LPVOID*)facbuf);
}

/* creates a new stub manager */
HRESULT marshal_object(APARTMENT *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *object, MSHLFLAGS mshlflags)
{
    struct stub_manager *manager;
    struct ifstub       *ifstub;
    BOOL                 tablemarshal;
    IRpcStubBuffer      *stub = NULL;
    HRESULT              hr;
    IUnknown            *iobject = NULL; /* object of type riid */

    hr = apartment_getoxid(apt, &stdobjref->oxid);
    if (hr != S_OK)
        return hr;

    hr = IUnknown_QueryInterface(object, riid, (void **)&iobject);
    if (hr != S_OK)
    {
        ERR("object doesn't expose interface %s, failing with error 0x%08lx\n",
            debugstr_guid(riid), hr);
        return E_NOINTERFACE;
    }
  
    /* IUnknown doesn't require a stub buffer, because it never goes out on
     * the wire */
    if (!IsEqualIID(riid, &IID_IUnknown))
    {
        IPSFactoryBuffer *psfb;

        hr = get_facbuf_for_iid(riid, &psfb);
        if (hr != S_OK)
        {
            ERR("couldn't get IPSFactory buffer for interface %s\n", debugstr_guid(riid));
            IUnknown_Release(iobject);
            return hr;
        }
    
        hr = IPSFactoryBuffer_CreateStub(psfb, riid, iobject, &stub);
        IPSFactoryBuffer_Release(psfb);
        if (hr != S_OK)
        {
            ERR("Failed to create an IRpcStubBuffer from IPSFactory for %s\n", debugstr_guid(riid));
            IUnknown_Release(iobject);
            return hr;
        }
    }

    if (mshlflags & MSHLFLAGS_NOPING)
        stdobjref->flags = SORF_NOPING;
    else
        stdobjref->flags = SORF_NULL;

    /* FIXME: what happens if we register an interface twice with different
     * marshaling flags? */
    if ((manager = get_stub_manager_from_object(apt, object)))
        TRACE("registering new ifstub on pre-existing manager\n");
    else
    {
        TRACE("constructing new stub manager\n");

        manager = new_stub_manager(apt, object, mshlflags);
        if (!manager)
        {
            if (stub) IRpcStubBuffer_Release(stub);
            IUnknown_Release(iobject);
            return E_OUTOFMEMORY;
        }
    }
    stdobjref->oid = manager->oid;

    tablemarshal = ((mshlflags & MSHLFLAGS_TABLESTRONG) || (mshlflags & MSHLFLAGS_TABLEWEAK));

    ifstub = stub_manager_new_ifstub(manager, stub, iobject, riid);
    IUnknown_Release(iobject);
    if (stub) IRpcStubBuffer_Release(stub);
    if (!ifstub)
    {
        stub_manager_int_release(manager);
        /* FIXME: should we do another release to completely destroy the
         * stub manager? */
        return E_OUTOFMEMORY;
    }

    if (!tablemarshal)
    {
        stdobjref->cPublicRefs = NORMALEXTREFS;
        stub_manager_ext_addref(manager, stdobjref->cPublicRefs);
    }
    else
    {
        stdobjref->cPublicRefs = 0;
        if (mshlflags & MSHLFLAGS_TABLESTRONG)
            stub_manager_ext_addref(manager, 1);
    }

    /* FIXME: check return value */
    RPC_RegisterInterface(riid);

    stdobjref->ipid = ifstub->ipid;

    stub_manager_int_release(manager);
    return S_OK;
}



/* Client-side identity of the server object */

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk);
static void proxy_manager_destroy(struct proxy_manager * This);
static HRESULT proxy_manager_find_ifproxy(struct proxy_manager * This, REFIID riid, struct ifproxy ** ifproxy_found);
static HRESULT proxy_manager_query_local_interface(struct proxy_manager * This, REFIID riid, void ** ppv);

static HRESULT WINAPI ClientIdentity_QueryInterface(IMultiQI * iface, REFIID riid, void ** ppv)
{
    HRESULT hr;
    MULTI_QI mqi;

    TRACE("%s\n", debugstr_guid(riid));

    mqi.pIID = riid;
    hr = IMultiQI_QueryMultipleInterfaces(iface, 1, &mqi);
    *ppv = (void *)mqi.pItf;

    return hr;
}

static ULONG WINAPI ClientIdentity_AddRef(IMultiQI * iface)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    TRACE("%p - before %ld\n", iface, This->refs);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI ClientIdentity_Release(IMultiQI * iface)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    TRACE("%p - after %ld\n", iface, refs);
    if (!refs)
        proxy_manager_destroy(This);
    return refs;
}

static HRESULT WINAPI ClientIdentity_QueryMultipleInterfaces(IMultiQI *iface, ULONG cMQIs, MULTI_QI *pMQIs)
{
    struct proxy_manager * This = (struct proxy_manager *)iface;
    REMQIRESULT *qiresults = NULL;
    ULONG nonlocal_mqis = 0;
    ULONG i;
    ULONG successful_mqis = 0;
    IID *iids = HeapAlloc(GetProcessHeap(), 0, cMQIs * sizeof(*iids));
    /* mapping of RemQueryInterface index to QueryMultipleInterfaces index */
    ULONG *mapping = HeapAlloc(GetProcessHeap(), 0, cMQIs * sizeof(*mapping));

    TRACE("cMQIs: %ld\n", cMQIs);

    /* try to get a local interface - this includes already active proxy
     * interfaces and also interfaces exposed by the proxy manager */
    for (i = 0; i < cMQIs; i++)
    {
        TRACE("iid[%ld] = %s\n", i, debugstr_guid(pMQIs[i].pIID));
        pMQIs[i].hr = proxy_manager_query_local_interface(This, pMQIs[i].pIID, (void **)&pMQIs[i].pItf);
        if (pMQIs[i].hr == S_OK)
            successful_mqis++;
        else
        {
            iids[nonlocal_mqis] = *pMQIs[i].pIID;
            mapping[nonlocal_mqis] = i;
            nonlocal_mqis++;
        }
    }

    TRACE("%ld interfaces not found locally\n", nonlocal_mqis);

    /* if we have more than one interface not found locally then we must try
     * to query the remote object for it */
    if (nonlocal_mqis != 0)
    {
        IRemUnknown *remunk;
        HRESULT hr;
        IPID *ipid;

        /* get the ipid of the first entry */
        /* FIXME: should we implement ClientIdentity on the ifproxies instead
         * of the proxy_manager so we use the correct ipid here? */
        ipid = &LIST_ENTRY(list_head(&This->interfaces), struct ifproxy, entry)->ipid;

        /* get IRemUnknown proxy so we can communicate with the remote object */
        hr = proxy_manager_get_remunknown(This, &remunk);

        if (hr == S_OK)
        {
            hr = IRemUnknown_RemQueryInterface(remunk, ipid, NORMALEXTREFS,
                                               nonlocal_mqis, iids, &qiresults);
            if (FAILED(hr))
                ERR("IRemUnknown_RemQueryInterface failed with error 0x%08lx\n", hr);
        }

        /* IRemUnknown_RemQueryInterface can return S_FALSE if only some of
         * the interfaces were returned */
        if (SUCCEEDED(hr))
        {
            /* try to unmarshal each object returned to us */
            for (i = 0; i < nonlocal_mqis; i++)
            {
                ULONG index = mapping[i];
                HRESULT hrobj = qiresults[i].hResult;
                if (hrobj == S_OK)
                    hrobj = unmarshal_object(&qiresults[i].std, This->parent,
                                             pMQIs[index].pIID,
                                             (void **)&pMQIs[index].pItf);

                if (hrobj == S_OK)
                    successful_mqis++;
                else
                    ERR("Failed to get pointer to interface %s\n", debugstr_guid(pMQIs[index].pIID));
                pMQIs[index].hr = hrobj;
            }
        }

        /* free the memory allocated by the proxy */
        CoTaskMemFree(qiresults);
    }

    TRACE("%ld/%ld successfully queried\n", successful_mqis, cMQIs);

    HeapFree(GetProcessHeap(), 0, iids);
    HeapFree(GetProcessHeap(), 0, mapping);

    if (successful_mqis == cMQIs)
        return S_OK; /* we got all requested interfaces */
    else if (successful_mqis == 0)
        return E_NOINTERFACE; /* we didn't get any interfaces */
    else
        return S_FALSE; /* we got some interfaces */
}

static const IMultiQIVtbl ClientIdentity_Vtbl =
{
    ClientIdentity_QueryInterface,
    ClientIdentity_AddRef,
    ClientIdentity_Release,
    ClientIdentity_QueryMultipleInterfaces
};

static HRESULT ifproxy_get_public_ref(struct ifproxy * This)
{
    HRESULT hr = S_OK;

    if (WAIT_OBJECT_0 != WaitForSingleObject(This->parent->remoting_mutex, INFINITE))
    {
        ERR("Wait failed for ifproxy %p\n", This);
        return E_UNEXPECTED;
    }

    if (This->refs == 0)
    {
        IRemUnknown *remunk = NULL;

        TRACE("getting public ref for ifproxy %p\n", This);

        hr = proxy_manager_get_remunknown(This->parent, &remunk);
        if (hr == S_OK)
        {
            HRESULT hrref;
            REMINTERFACEREF rif;
            rif.ipid = This->ipid;
            rif.cPublicRefs = NORMALEXTREFS;
            rif.cPrivateRefs = 0;
            hr = IRemUnknown_RemAddRef(remunk, 1, &rif, &hrref);
            if (hr == S_OK && hrref == S_OK)
                This->refs += NORMALEXTREFS;
            else
                ERR("IRemUnknown_RemAddRef returned with 0x%08lx, hrref = 0x%08lx\n", hr, hrref);
        }
    }
    ReleaseMutex(This->parent->remoting_mutex);

    return hr;
}

static HRESULT ifproxy_release_public_refs(struct ifproxy * This)
{
    HRESULT hr = S_OK;

    if (WAIT_OBJECT_0 != WaitForSingleObject(This->parent->remoting_mutex, INFINITE))
    {
        ERR("Wait failed for ifproxy %p\n", This);
        return E_UNEXPECTED;
    }

    if (This->refs > 0)
    {
        IRemUnknown *remunk = NULL;

        TRACE("releasing %ld refs\n", This->refs);

        hr = proxy_manager_get_remunknown(This->parent, &remunk);
        if (hr == S_OK)
        {
            REMINTERFACEREF rif;
            rif.ipid = This->ipid;
            rif.cPublicRefs = This->refs;
            rif.cPrivateRefs = 0;
            hr = IRemUnknown_RemRelease(remunk, 1, &rif);
            if (hr == S_OK)
                This->refs = 0;
            else if (hr == RPC_E_DISCONNECTED)
                WARN("couldn't release references because object was "
                     "disconnected: oxid = %s, oid = %s\n",
                     wine_dbgstr_longlong(This->parent->oxid),
                     wine_dbgstr_longlong(This->parent->oid));
            else
                ERR("IRemUnknown_RemRelease failed with error 0x%08lx\n", hr);
        }
    }
    ReleaseMutex(This->parent->remoting_mutex);

    return hr;
}

/* should be called inside This->parent->cs critical section */
static void ifproxy_disconnect(struct ifproxy * This)
{
    ifproxy_release_public_refs(This);
    if (This->proxy) IRpcProxyBuffer_Disconnect(This->proxy);

    IRpcChannelBuffer_Release(This->chan);
    This->chan = NULL;
}

/* should be called in This->parent->cs critical section if it is an entry in parent's list */
static void ifproxy_destroy(struct ifproxy * This)
{
    TRACE("%p\n", This);

    /* release public references to this object so that the stub can know
     * when to destroy itself */
    ifproxy_release_public_refs(This);

    list_remove(&This->entry);

    if (This->chan)
    {
        IRpcChannelBuffer_Release(This->chan);
        This->chan = NULL;
    }

    /* note: we don't call Release for This->proxy because its lifetime is
     * controlled by the return value from ClientIdentity_Release, which this
     * function is always called from */

    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT proxy_manager_construct(
    APARTMENT * apt, ULONG sorflags, OXID oxid, OID oid,
    struct proxy_manager ** proxy_manager)
{
    struct proxy_manager * This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->remoting_mutex = CreateMutexW(NULL, FALSE, NULL);
    if (!This->remoting_mutex)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    This->lpVtbl = &ClientIdentity_Vtbl;

    list_init(&This->entry);
    list_init(&This->interfaces);

    InitializeCriticalSection(&This->cs);
    DEBUG_SET_CRITSEC_NAME(&This->cs, "proxy_manager");

    /* the apartment the object was unmarshaled into */
    This->parent = apt;

    /* the source apartment and id of the object */
    This->oxid = oxid;
    This->oid = oid;

    This->refs = 1;

    /* the DCOM draft specification states that the SORF_NOPING flag is
     * proxy manager specific, not ifproxy specific, so this implies that we
     * should store the STDOBJREF flags here in the proxy manager. */
    This->sorflags = sorflags;

    /* we create the IRemUnknown proxy on demand */
    This->remunk = NULL;

    EnterCriticalSection(&apt->cs);
    /* FIXME: we are dependent on the ordering in here to make sure a proxy's
     * IRemUnknown proxy doesn't get destroyed before the regual proxy does
     * because we need the IRemUnknown proxy during the destruction of the
     * regular proxy. Ideally, we should maintain a separate list for the
     * IRemUnknown proxies that need late destruction */
    list_add_tail(&apt->proxies, &This->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("%p created for OXID %s, OID %s\n", This,
        wine_dbgstr_longlong(oxid), wine_dbgstr_longlong(oid));

    *proxy_manager = This;
    return S_OK;
}

static HRESULT proxy_manager_query_local_interface(struct proxy_manager * This, REFIID riid, void ** ppv)
{
    HRESULT hr;
    struct ifproxy * ifproxy;

    TRACE("%s\n", debugstr_guid(riid));

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMultiQI))
    {
        *ppv = (void *)&This->lpVtbl;
        IMultiQI_AddRef((IMultiQI *)&This->lpVtbl);
        return S_OK;
    }

    hr = proxy_manager_find_ifproxy(This, riid, &ifproxy);
    if (hr == S_OK)
    {
        *ppv = ifproxy->iface;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT proxy_manager_create_ifproxy(
    struct proxy_manager * This, const IPID *ipid, REFIID riid, ULONG cPublicRefs,
    IRpcChannelBuffer * channel, struct ifproxy ** iif_out)
{
    HRESULT hr;
    IPSFactoryBuffer * psfb;
    struct ifproxy * ifproxy = HeapAlloc(GetProcessHeap(), 0, sizeof(*ifproxy));
    if (!ifproxy) return E_OUTOFMEMORY;

    list_init(&ifproxy->entry);

    ifproxy->parent = This;
    ifproxy->ipid = *ipid;
    ifproxy->iid = *riid;
    ifproxy->refs = cPublicRefs;
    ifproxy->proxy = NULL;

    assert(channel);
    ifproxy->chan = channel; /* FIXME: we should take the binding strings and construct the channel in this function */

    /* the IUnknown interface is special because it does not have a
     * proxy associated with the ifproxy as we handle IUnknown ourselves */
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        ifproxy->iface = (void *)&This->lpVtbl;
        hr = S_OK;
    }
    else
    {
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
            if (hr != S_OK)
                ERR("Could not create proxy for interface %s, error 0x%08lx\n",
                    debugstr_guid(riid), hr);
        }
        else
            ERR("Could not get IPSFactoryBuffer for interface %s, error 0x%08lx\n",
                debugstr_guid(riid), hr);

        if (hr == S_OK)
            hr = IRpcProxyBuffer_Connect(ifproxy->proxy, ifproxy->chan);
    }

    /* get at least one external reference to the object to keep it alive */
    if (hr == S_OK)
        hr = ifproxy_get_public_ref(ifproxy);

    if (hr == S_OK)
    {
        EnterCriticalSection(&This->cs);
        list_add_tail(&This->interfaces, &ifproxy->entry);
        LeaveCriticalSection(&This->cs);

        *iif_out = ifproxy;
        TRACE("ifproxy %p created for IPID %s, interface %s with %lu public refs\n",
              ifproxy, debugstr_guid(ipid), debugstr_guid(riid), cPublicRefs);
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

    TRACE("oxid = %s, oid = %s\n", wine_dbgstr_longlong(This->oxid),
        wine_dbgstr_longlong(This->oid));

    /* SORFP_NOLIFTIMEMGMT proxies (for IRemUnknown) shouldn't be
     * disconnected - it won't do anything anyway, except cause
     * problems for other objects that depend on this proxy always
     * working */
    if (This->sorflags & SORFP_NOLIFETIMEMGMT) return;

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

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk)
{
    HRESULT hr = S_OK;

    /* we don't want to try and unmarshal or use IRemUnknown if we don't want
     * lifetime management */
    if (This->sorflags & SORFP_NOLIFETIMEMGMT)
        return S_FALSE;

    EnterCriticalSection(&This->cs);
    if (This->remunk)
        /* already created - return existing object */
        *remunk = This->remunk;
    else if (!This->parent)
        /* disconnected - we can't create IRemUnknown */
        hr = S_FALSE;
    else
    {
        STDOBJREF stdobjref;
        /* Don't want IRemUnknown lifetime management as this is IRemUnknown!
         * We also don't care about whether or not the stub is still alive */
        stdobjref.flags = SORFP_NOLIFETIMEMGMT | SORF_NOPING;
        stdobjref.cPublicRefs = 1;
        /* oxid of destination object */
        stdobjref.oxid = This->oxid;
        /* FIXME: what should be used for the oid? The DCOM draft doesn't say */
        stdobjref.oid = (OID)-1;
        /* FIXME: this is a hack around not having an OXID resolver yet -
         * the OXID resolver should give us the IPID of the IRemUnknown
         * interface */
        stdobjref.ipid.Data1 = 0xffffffff;
        stdobjref.ipid.Data2 = 0xffff;
        stdobjref.ipid.Data3 = 0xffff;
        assert(sizeof(stdobjref.ipid.Data4) == sizeof(stdobjref.oxid));
        memcpy(&stdobjref.ipid.Data4, &stdobjref.oxid, sizeof(OXID));
        
        /* do the unmarshal */
        hr = unmarshal_object(&stdobjref, This->parent, &IID_IRemUnknown, (void**)&This->remunk);
        if (hr == S_OK)
            *remunk = This->remunk;
    }
    LeaveCriticalSection(&This->cs);

    TRACE("got IRemUnknown* pointer %p, hr = 0x%08lx\n", *remunk, hr);

    return hr;
}

/* destroys a proxy manager, freeing the memory it used.
 * Note: this function should not be called from a list iteration in the
 * apartment, due to the fact that it removes itself from the apartment and
 * it could add a proxy to IRemUnknown into the apartment. */
static void proxy_manager_destroy(struct proxy_manager * This)
{
    struct list * cursor;

    TRACE("oxid = %s, oid = %s\n", wine_dbgstr_longlong(This->oxid),
        wine_dbgstr_longlong(This->oid));

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
    while ((cursor = list_head(&This->interfaces)))
    {
        struct ifproxy * ifproxy = LIST_ENTRY(cursor, struct ifproxy, entry);
        ifproxy_destroy(ifproxy);
    }

    if (This->remunk) IRemUnknown_Release(This->remunk);

    DEBUG_CLEAR_CRITSEC_NAME(&This->cs);
    DeleteCriticalSection(&This->cs);

    CloseHandle(This->remoting_mutex);

    HeapFree(GetProcessHeap(), 0, This);
}

/* finds the proxy manager corresponding to a given OXID and OID that has
 * been unmarshaled in the specified apartment. The caller must release the
 * reference to the proxy_manager when the object is no longer used. */
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
            ClientIdentity_AddRef((IMultiQI *)&proxy->lpVtbl);
            found = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);
    return found;
}

HRESULT apartment_disconnectproxies(struct apartment *apt)
{
    struct list * cursor;

    LIST_FOR_EACH(cursor, &apt->proxies)
    {
        struct proxy_manager * proxy = LIST_ENTRY(cursor, struct proxy_manager, entry);
        proxy_manager_disconnect(proxy);
    }

    return S_OK;
}

/********************** StdMarshal implementation ****************************/
typedef struct _StdMarshalImpl
{
    const IMarshalVtbl	*lpvtbl;
    LONG		ref;

    IID			iid;
    DWORD		dwDestContext;
    LPVOID		pvDestContext;
    DWORD		mshlflags;
} StdMarshalImpl;

static HRESULT WINAPI 
StdMarshalImpl_QueryInterface(LPMARSHAL iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IMarshal, riid))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    FIXME("No interface for %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI
StdMarshalImpl_AddRef(LPMARSHAL iface)
{
    StdMarshalImpl *This = (StdMarshalImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI
StdMarshalImpl_Release(LPMARSHAL iface)
{
    StdMarshalImpl *This = (StdMarshalImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref) HeapFree(GetProcessHeap(),0,This);
    return ref;
}

static HRESULT WINAPI
StdMarshalImpl_GetUnmarshalClass(
    LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    *pCid = CLSID_DfMarshal;
    return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_GetMarshalSizeMax(
    LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    *pSize = sizeof(STDOBJREF);
    return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_MarshalInterface(
    LPMARSHAL iface, IStream *pStm,REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags)
{
    STDOBJREF             stdobjref;
    ULONG                 res;
    HRESULT               hres;
    APARTMENT            *apt = COM_CurrentApt();

    TRACE("(...,%s,...)\n", debugstr_guid(riid));

    if (!apt)
    {
        ERR("Apartment not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    /* make sure this apartment can be reached from other threads / processes */
    RPC_StartRemoting(apt);

    hres = marshal_object(apt, &stdobjref, riid, (IUnknown *)pv, mshlflags);
    if (hres)
    {
        ERR("Failed to create ifstub, hres=0x%lx\n", hres);
        return hres;
    }

    hres = IStream_Write(pStm, &stdobjref, sizeof(stdobjref), &res);
    if (hres) return hres;

    return S_OK;
}

/* helper for StdMarshalImpl_UnmarshalInterface - does the unmarshaling with
 * no questions asked about the rules surrounding same-apartment unmarshals
 * and table marshaling */
static HRESULT unmarshal_object(const STDOBJREF *stdobjref, APARTMENT *apt, REFIID riid, void **object)
{
    struct proxy_manager *proxy_manager = NULL;
    HRESULT hr = S_OK;

    assert(apt);

    TRACE("stdobjref:\n\tflags = %04lx\n\tcPublicRefs = %ld\n\toxid = %s\n\toid = %s\n\tipid = %s\n",
        stdobjref->flags, stdobjref->cPublicRefs,
        wine_dbgstr_longlong(stdobjref->oxid),
        wine_dbgstr_longlong(stdobjref->oid),
        debugstr_guid(&stdobjref->ipid));

    /* create an a new proxy manager if one doesn't already exist for the
     * object */
    if (!find_proxy_manager(apt, stdobjref->oxid, stdobjref->oid, &proxy_manager))
    {
        hr = proxy_manager_construct(apt, stdobjref->flags,
                                     stdobjref->oxid, stdobjref->oid,
                                     &proxy_manager);
    }
    else
        TRACE("proxy manager already created, using\n");

    if (hr == S_OK)
    {
        struct ifproxy * ifproxy;
        hr = proxy_manager_find_ifproxy(proxy_manager, riid, &ifproxy);
        if (hr == E_NOINTERFACE)
        {
            IRpcChannelBuffer *chanbuf;
            hr = RPC_CreateClientChannel(&stdobjref->oxid, &stdobjref->ipid, &chanbuf);
            if (hr == S_OK)
                hr = proxy_manager_create_ifproxy(proxy_manager, &stdobjref->ipid,
                                                  riid, stdobjref->cPublicRefs,
                                                  chanbuf, &ifproxy);
        }

        if (hr == S_OK)
        {
            /* FIXME: push this AddRef inside proxy_manager_find_ifproxy/create_ifproxy? */
            ClientIdentity_AddRef((IMultiQI*)&proxy_manager->lpVtbl);
            *object = ifproxy->iface;
        }
    }

    /* release our reference to the proxy manager - the client/apartment
     * will hold on to the remaining reference for us */
    if (proxy_manager) ClientIdentity_Release((IMultiQI*)&proxy_manager->lpVtbl);

    return hr;
}

static HRESULT WINAPI
StdMarshalImpl_UnmarshalInterface(LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv)
{
    struct stub_manager  *stubmgr;
    STDOBJREF stdobjref;
    ULONG res;
    HRESULT hres;
    APARTMENT *apt = COM_CurrentApt();
    APARTMENT *stub_apt;
    OXID oxid;

    TRACE("(...,%s,....)\n", debugstr_guid(riid));

    /* we need an apartment to unmarshal into */
    if (!apt)
    {
        ERR("Apartment not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    /* read STDOBJREF from wire */
    hres = IStream_Read(pStm, &stdobjref, sizeof(stdobjref), &res);
    if (hres) return hres;

    hres = apartment_getoxid(apt, &oxid);
    if (hres) return hres;

    /* check if we're marshalling back to ourselves */
    if ((oxid == stdobjref.oxid) && (stubmgr = get_stub_manager(apt, stdobjref.oid)))
    {
        TRACE("Unmarshalling object marshalled in same apartment for iid %s, "
              "returning original object %p\n", debugstr_guid(riid), stubmgr->object);
    
        hres = IUnknown_QueryInterface(stubmgr->object, riid, ppv);
      
        /* unref the ifstub. FIXME: only do this on success? */
        if (!stub_manager_is_table_marshaled(stubmgr))
            stub_manager_ext_release(stubmgr, 1);

        stub_manager_int_release(stubmgr);
        return hres;
    }

    /* notify stub manager about unmarshal if process-local object.
     * note: if the oxid is not found then we and native will quite happily
     * ignore table marshaling and normal marshaling rules regarding number of
     * unmarshals, etc, but if you abuse these rules then your proxy could end
     * up returning RPC_E_DISCONNECTED. */
    if ((stub_apt = apartment_findfromoxid(stdobjref.oxid, TRUE)))
    {
        if ((stubmgr = get_stub_manager(stub_apt, stdobjref.oid)))
        {
            if (!stub_manager_notify_unmarshal(stubmgr))
                hres = CO_E_OBJNOTCONNECTED;

            stub_manager_int_release(stubmgr);
        }
        else
        {
            WARN("Couldn't find object for OXID %s, OID %s, assuming disconnected\n",
                wine_dbgstr_longlong(stdobjref.oxid),
                wine_dbgstr_longlong(stdobjref.oid));
            hres = CO_E_OBJNOTCONNECTED;
        }

        apartment_release(stub_apt);
    }
    else
        TRACE("Treating unmarshal from OXID %s as inter-process\n",
            wine_dbgstr_longlong(stdobjref.oxid));

    if (hres == S_OK)
        hres = unmarshal_object(&stdobjref, apt, riid, ppv);

    if (hres) WARN("Failed with error 0x%08lx\n", hres);
    else TRACE("Successfully created proxy %p\n", *ppv);

    return hres;
}

static HRESULT WINAPI
StdMarshalImpl_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm)
{
    STDOBJREF            stdobjref;
    ULONG                res;
    HRESULT              hres;
    struct stub_manager *stubmgr;
    APARTMENT           *apt;

    TRACE("iface=%p, pStm=%p\n", iface, pStm);
    
    hres = IStream_Read(pStm, &stdobjref, sizeof(stdobjref), &res);
    if (hres) return hres;

    TRACE("oxid = %s, oid = %s, ipid = %s\n",
        wine_dbgstr_longlong(stdobjref.oxid),
        wine_dbgstr_longlong(stdobjref.oid),
        wine_dbgstr_guid(&stdobjref.ipid));

    if (!(apt = apartment_findfromoxid(stdobjref.oxid, TRUE)))
    {
        WARN("Could not map OXID %s to apartment object\n",
            wine_dbgstr_longlong(stdobjref.oxid));
        return RPC_E_INVALID_OBJREF;
    }

    if (!(stubmgr = get_stub_manager(apt, stdobjref.oid)))
    {
        ERR("could not map MID to stub manager, oxid=%s, oid=%s\n",
            wine_dbgstr_longlong(stdobjref.oxid), wine_dbgstr_longlong(stdobjref.oid));
        return RPC_E_INVALID_OBJREF;
    }

    stub_manager_release_marshal_data(stubmgr, stdobjref.cPublicRefs);

    stub_manager_int_release(stubmgr);
    apartment_release(apt);

    return S_OK;
}

static HRESULT WINAPI
StdMarshalImpl_DisconnectObject(LPMARSHAL iface, DWORD dwReserved)
{
    FIXME("(), stub!\n");
    return S_OK;
}

static const IMarshalVtbl VT_StdMarshal =
{
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
    pStdMarshal->lpvtbl = &VT_StdMarshal;
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

    if (pUnk == NULL)
    {
        FIXME("(%s,NULL,%lx,%p,%lx,%p), unimplemented yet.\n",
            debugstr_guid(riid),dwDestContext,pvDestContext,mshlflags,ppMarshal);
        return E_NOTIMPL;
    }
    TRACE("(%s,%p,%lx,%p,%lx,%p)\n",
        debugstr_guid(riid),pUnk,dwDestContext,pvDestContext,mshlflags,ppMarshal);
    *ppMarshal = HeapAlloc(GetProcessHeap(),0,sizeof(StdMarshalImpl));
    dm = (StdMarshalImpl*) *ppMarshal;
    if (!dm) return E_FAIL;
    dm->lpvtbl		= &VT_StdMarshal;
    dm->ref		= 1;

    dm->iid		= *riid;
    dm->dwDestContext	= dwDestContext;
    dm->pvDestContext	= pvDestContext;
    dm->mshlflags	= mshlflags;
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
static HRESULT get_unmarshaler_from_stream(IStream *stream, IMarshal **marshal, IID *iid)
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

    if (iid) *iid = objref.iid;

    /* FIXME: handler marshaling */
    if (objref.flags & OBJREF_STANDARD)
    {
        TRACE("Using standard unmarshaling\n");
        hr = StdMarshalImpl_Construct(&IID_IMarshal, (LPVOID*)marshal);
    }
    else if (objref.flags & OBJREF_CUSTOM)
    {
        ULONG custom_header_size = FIELD_OFFSET(OBJREF, u_objref.u_custom.pData) - 
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
        *pulSize += FIELD_OFFSET(OBJREF, u_objref.u_custom.pData) - 
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

        /* write the common OBJREF header to the stream */
        hr = IStream_Write(pStream, &objref, FIELD_OFFSET(OBJREF, u_objref), NULL);
        if (hr)
        {
            ERR("Failed to write OBJREF header to stream, 0x%08lx\n", hr);
            goto cleanup;
        }
    }
    else
    {
        TRACE("Using custom marshaling\n");
        objref.flags = OBJREF_CUSTOM;
        objref.u_objref.u_custom.clsid = marshaler_clsid;
        objref.u_objref.u_custom.cbExtension = 0;
        objref.u_objref.u_custom.size = 0;
        hr = IMarshal_GetMarshalSizeMax(pMarshal, riid, pUnk, dwDestContext,
                                        pvDestContext, mshlFlags,
                                        &objref.u_objref.u_custom.size);
        if (hr)
        {
            ERR("Failed to get max size of marshal data, error 0x%08lx\n", hr);
            goto cleanup;
        }
        /* write constant sized common header and OR_CUSTOM data into stream */
        hr = IStream_Write(pStream, &objref,
                          FIELD_OFFSET(OBJREF, u_objref.u_custom.pData), NULL);
        if (hr)
        {
            ERR("Failed to write OR_CUSTOM header to stream with 0x%08lx\n", hr);
            goto cleanup;
        }
    }

    TRACE("Calling IMarshal::MarshalInterace\n");
    /* call helper object to do the actual marshaling */
    hr = IMarshal_MarshalInterface(pMarshal, pStream, riid, pUnk, dwDestContext,
                                   pvDestContext, mshlFlags);

    if (hr)
    {
        ERR("Failed to marshal the interface %s, %lx\n", debugstr_guid(riid), hr);
        goto cleanup;
    }

cleanup:
    IMarshal_Release(pMarshal);

    TRACE("completed with hr 0x%08lx\n", hr);
    
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
    HRESULT hr;
    LPMARSHAL pMarshal;
    IID iid;
    IUnknown *object;

    TRACE("(%p, %s, %p)\n", pStream, debugstr_guid(riid), ppv);

    hr = get_unmarshaler_from_stream(pStream, &pMarshal, &iid);
    if (hr != S_OK)
        return hr;

    /* call the helper object to do the actual unmarshaling */
    hr = IMarshal_UnmarshalInterface(pMarshal, pStream, &iid, (LPVOID*)&object);
    if (hr)
        ERR("IMarshal::UnmarshalInterface failed, 0x%08lx\n", hr);

    /* IID_NULL means use the interface ID of the marshaled object */
    if (!IsEqualIID(riid, &IID_NULL))
        iid = *riid;

    if (hr == S_OK)
    {
        if (!IsEqualIID(riid, &iid))
        {
            TRACE("requested interface != marshalled interface, additional QI needed\n");
            hr = IUnknown_QueryInterface(object, &iid, ppv);
            if (hr)
                ERR("Couldn't query for interface %s, hr = 0x%08lx\n",
                    debugstr_guid(riid), hr);
            IUnknown_Release(object);
        }
        else
        {
            *ppv = object;
        }
    }

    IMarshal_Release(pMarshal);

    TRACE("completed with hr 0x%lx\n", hr);
    
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

    hr = get_unmarshaler_from_stream(pStream, &pMarshal, NULL);
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
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMarshal))
        return StdMarshalImpl_Construct(riid, ppv);

    FIXME("(%s), not supported.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI StdMarshalCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl StdMarshalCFVtbl =
{
    StdMarshalCF_QueryInterface,
    StdMarshalCF_AddRef,
    StdMarshalCF_Release,
    StdMarshalCF_CreateInstance,
    StdMarshalCF_LockServer
};
static const IClassFactoryVtbl *StdMarshalCF = &StdMarshalCFVtbl;

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
