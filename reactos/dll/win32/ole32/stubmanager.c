/*
 * A stub manager is an object that controls interface stubs. It is
 * identified by an OID (object identifier) and acts as the network
 * identity of the object. There can be many stub managers in a
 * process or apartment.
 *
 * Copyright 2002 Marcus Meissner
 * Copyright 2004 Mike Hearn for CodeWeavers
 * Copyright 2004 Robert Shearman (for CodeWeavers)
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <assert.h>
#include <stdarg.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "rpc.h"

#include "wine/debug.h"
#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static void stub_manager_delete_ifstub(struct stub_manager *m, struct ifstub *ifstub);
static struct ifstub *stub_manager_ipid_to_ifstub(struct stub_manager *m, const IPID *ipid);

/* creates a new stub manager and adds it into the apartment. caller must
 * release stub manager when it is no longer required. the apartment and
 * external refs together take one implicit ref */
struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object)
{
    struct stub_manager *sm;

    assert( apt );
    
    sm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct stub_manager));
    if (!sm) return NULL;

    list_init(&sm->ifstubs);

    InitializeCriticalSection(&sm->lock);
    DEBUG_SET_CRITSEC_NAME(&sm->lock, "stub_manager");

    IUnknown_AddRef(object);
    sm->object = object;
    sm->apt    = apt;

    /* start off with 2 references because the stub is in the apartment
     * and the caller will also hold a reference */
    sm->refs   = 2;
    sm->weakrefs = 0;

    sm->oxid_info.dwPid = GetCurrentProcessId();
    sm->oxid_info.dwTid = GetCurrentThreadId();
    /*
     * FIXME: this is a hack for marshalling IRemUnknown. In real
     * DCOM, the IPID of the IRemUnknown interface is generated like
     * any other and passed to the OXID resolver which then returns it
     * when queried. We don't have an OXID resolver yet so instead we
     * use a magic IPID reserved for IRemUnknown.
     */
    sm->oxid_info.ipidRemUnknown.Data1 = 0xffffffff;
    sm->oxid_info.ipidRemUnknown.Data2 = 0xffff;
    sm->oxid_info.ipidRemUnknown.Data3 = 0xffff;
    assert(sizeof(sm->oxid_info.ipidRemUnknown.Data4) == sizeof(apt->oxid));
    memcpy(sm->oxid_info.ipidRemUnknown.Data4, &apt->oxid, sizeof(OXID));
    sm->oxid_info.dwAuthnHint = RPC_C_AUTHN_LEVEL_NONE;
    sm->oxid_info.psa = NULL /* FIXME */;

    /* Yes, that's right, this starts at zero. that's zero EXTERNAL
     * refs, i.e., nobody has unmarshalled anything yet. We can't have
     * negative refs because the stub manager cannot be explicitly
     * killed, it has to die by somebody unmarshalling then releasing
     * the marshalled ifptr.
     */
    sm->extrefs = 0;

    EnterCriticalSection(&apt->cs);
    sm->oid = apt->oidc++;
    list_add_head(&apt->stubmgrs, &sm->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("Created new stub manager (oid=%s) at %p for object with IUnknown %p\n", wine_dbgstr_longlong(sm->oid), sm, object);
    
    return sm;
}

/* caller must remove stub manager from apartment prior to calling this function */
static void stub_manager_delete(struct stub_manager *m)
{
    struct list *cursor;

    TRACE("destroying %p (oid=%s)\n", m, wine_dbgstr_longlong(m->oid));

    /* release every ifstub */
    while ((cursor = list_head(&m->ifstubs)))
    {
        struct ifstub *ifstub = LIST_ENTRY(cursor, struct ifstub, entry);
        stub_manager_delete_ifstub(m, ifstub);
    }

    CoTaskMemFree(m->oxid_info.psa);
    IUnknown_Release(m->object);

    DEBUG_CLEAR_CRITSEC_NAME(&m->lock);
    DeleteCriticalSection(&m->lock);

    HeapFree(GetProcessHeap(), 0, m);
}

/* gets the stub manager associated with an object - caller must have
 * a reference to the apartment while a reference to the stub manager is held.
 * it must also call release on the stub manager when it is no longer needed */
struct stub_manager *get_stub_manager_from_object(APARTMENT *apt, void *object)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH( cursor, &apt->stubmgrs )
    {
        struct stub_manager *m = LIST_ENTRY( cursor, struct stub_manager, entry );

        if (m->object == object)
        {
            result = m;
            stub_manager_int_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    if (result)
        TRACE("found %p for object %p\n", result, object);
    else
        TRACE("not found for object %p\n", object);

    return result;    
}

/* removes the apartment reference to an object, destroying it when no other
 * threads have a reference to it */
void apartment_disconnectobject(struct apartment *apt, void *object)
{
    int found = FALSE;
    struct stub_manager *stubmgr;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH_ENTRY( stubmgr, &apt->stubmgrs, struct stub_manager, entry )
    {
        if (stubmgr->object == object)
        {
            found = TRUE;
            stub_manager_int_release(stubmgr);
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    if (found)
        TRACE("disconnect object %p\n", object);
    else
        WARN("couldn't find object %p\n", object);
}

/* gets the stub manager associated with an object id - caller must have
 * a reference to the apartment while a reference to the stub manager is held.
 * it must also call release on the stub manager when it is no longer needed */
struct stub_manager *get_stub_manager(APARTMENT *apt, OID oid)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH( cursor, &apt->stubmgrs )
    {
        struct stub_manager *m = LIST_ENTRY( cursor, struct stub_manager, entry );

        if (m->oid == oid)
        {
            result = m;
            stub_manager_int_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    if (result)
        TRACE("found %p for oid %s\n", result, wine_dbgstr_longlong(oid));
    else
        TRACE("not found for oid %s\n", wine_dbgstr_longlong(oid));

    return result;
}

/* increments the internal refcount */
ULONG stub_manager_int_addref(struct stub_manager *This)
{
    ULONG refs;

    EnterCriticalSection(&This->apt->cs);
    refs = ++This->refs;
    LeaveCriticalSection(&This->apt->cs);

    TRACE("before %d\n", refs - 1);

    return refs;
}

/* decrements the internal refcount */
ULONG stub_manager_int_release(struct stub_manager *This)
{
    ULONG refs;
    APARTMENT *apt = This->apt;

    EnterCriticalSection(&apt->cs);
    refs = --This->refs;

    TRACE("after %d\n", refs);

    /* remove from apartment so no other thread can access it... */
    if (!refs)
        list_remove(&This->entry);

    LeaveCriticalSection(&apt->cs);

    /* ... so now we can delete it without being inside the apartment critsec */
    if (!refs)
        stub_manager_delete(This);

    return refs;
}

/* add some external references (ie from a client that unmarshaled an ifptr) */
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs, BOOL tableweak)
{
    ULONG rc;

    EnterCriticalSection(&m->lock);

    /* make sure we don't overflow extrefs */
    refs = min(refs, (ULONG_MAX-1 - m->extrefs));
    rc = (m->extrefs += refs);

    if (tableweak)
        rc += ++m->weakrefs;

    LeaveCriticalSection(&m->lock);
    
    TRACE("added %u refs to %p (oid %s), rc is now %u\n", refs, m, wine_dbgstr_longlong(m->oid), rc);

    return rc;
}

/* remove some external references */
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs, BOOL tableweak, BOOL last_unlock_releases)
{
    ULONG rc;

    EnterCriticalSection(&m->lock);

    /* make sure we don't underflow extrefs */
    refs = min(refs, m->extrefs);
    rc = (m->extrefs -= refs);

    if (tableweak)
        rc += --m->weakrefs;

    LeaveCriticalSection(&m->lock);
    
    TRACE("removed %u refs from %p (oid %s), rc is now %u\n", refs, m, wine_dbgstr_longlong(m->oid), rc);

    if (rc == 0 && last_unlock_releases)
        stub_manager_int_release(m);

    return rc;
}

static struct ifstub *stub_manager_ipid_to_ifstub(struct stub_manager *m, const IPID *ipid)
{
    struct list    *cursor;
    struct ifstub  *result = NULL;
    
    EnterCriticalSection(&m->lock);
    LIST_FOR_EACH( cursor, &m->ifstubs )
    {
        struct ifstub *ifstub = LIST_ENTRY( cursor, struct ifstub, entry );

        if (IsEqualGUID(ipid, &ifstub->ipid))
        {
            result = ifstub;
            break;
        }
    }
    LeaveCriticalSection(&m->lock);

    return result;
}

struct ifstub *stub_manager_find_ifstub(struct stub_manager *m, REFIID iid, MSHLFLAGS flags)
{
    struct ifstub  *result = NULL;
    struct ifstub  *ifstub;

    EnterCriticalSection(&m->lock);
    LIST_FOR_EACH_ENTRY( ifstub, &m->ifstubs, struct ifstub, entry )
    {
        if (IsEqualIID(iid, &ifstub->iid) && (ifstub->flags == flags))
        {
            result = ifstub;
            break;
        }
    }
    LeaveCriticalSection(&m->lock);

    return result;
}

/* gets the stub manager associated with an ipid - caller must have
 * a reference to the apartment while a reference to the stub manager is held.
 * it must also call release on the stub manager when it is no longer needed */
static struct stub_manager *get_stub_manager_from_ipid(APARTMENT *apt, const IPID *ipid)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH( cursor, &apt->stubmgrs )
    {
        struct stub_manager *m = LIST_ENTRY( cursor, struct stub_manager, entry );

        if (stub_manager_ipid_to_ifstub(m, ipid))
        {
            result = m;
            stub_manager_int_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&apt->cs);

    if (result)
        TRACE("found %p for ipid %s\n", result, debugstr_guid(ipid));
    else
        ERR("not found for ipid %s\n", debugstr_guid(ipid));

    return result;
}

static HRESULT ipid_to_stub_manager(const IPID *ipid, APARTMENT **stub_apt, struct stub_manager **stubmgr_ret)
{
    /* FIXME: hack for IRemUnknown */
    if (ipid->Data2 == 0xffff)
        *stub_apt = apartment_findfromoxid(*(const OXID *)ipid->Data4, TRUE);
    else
        *stub_apt = apartment_findfromtid(ipid->Data2);
    if (!*stub_apt)
    {
        TRACE("Couldn't find apartment corresponding to TID 0x%04x\n", ipid->Data2);
        return RPC_E_INVALID_OBJECT;
    }
    *stubmgr_ret = get_stub_manager_from_ipid(*stub_apt, ipid);
    if (!*stubmgr_ret)
    {
        apartment_release(*stub_apt);
        *stub_apt = NULL;
        return RPC_E_INVALID_OBJECT;
    }
    return S_OK;
}

/* gets the apartment, stub and channel of an object. the caller must
 * release the references to all objects (except iface) if the function
 * returned success, otherwise no references are returned. */
HRESULT ipid_get_dispatch_params(const IPID *ipid, APARTMENT **stub_apt,
                                 IRpcStubBuffer **stub, IRpcChannelBuffer **chan,
                                 IID *iid, IUnknown **iface)
{
    struct stub_manager *stubmgr;
    struct ifstub *ifstub;
    APARTMENT *apt;
    HRESULT hr;

    hr = ipid_to_stub_manager(ipid, &apt, &stubmgr);
    if (hr != S_OK) return RPC_E_DISCONNECTED;

    ifstub = stub_manager_ipid_to_ifstub(stubmgr, ipid);
    if (ifstub)
    {
        *stub = ifstub->stubbuffer;
        IRpcStubBuffer_AddRef(*stub);
        *chan = ifstub->chan;
        IRpcChannelBuffer_AddRef(*chan);
        *stub_apt = apt;
        *iid = ifstub->iid;
        *iface = ifstub->iface;

        stub_manager_int_release(stubmgr);
        return S_OK;
    }
    else
    {
        stub_manager_int_release(stubmgr);
        apartment_release(apt);
        return RPC_E_DISCONNECTED;
    }
}

/* generates an ipid in the following format (similar to native version):
 * Data1 = apartment-local ipid counter
 * Data2 = apartment creator thread ID
 * Data3 = process ID
 * Data4 = random value
 */
static inline HRESULT generate_ipid(struct stub_manager *m, IPID *ipid)
{
    HRESULT hr;
    hr = UuidCreate(ipid);
    if (FAILED(hr))
    {
        ERR("couldn't create IPID for stub manager %p\n", m);
        UuidCreateNil(ipid);
        return hr;
    }

    ipid->Data1 = InterlockedIncrement(&m->apt->ipidc);
    ipid->Data2 = (USHORT)m->apt->tid;
    ipid->Data3 = (USHORT)GetCurrentProcessId();
    return S_OK;
}

/* registers a new interface stub COM object with the stub manager and returns registration record */
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid, MSHLFLAGS flags)
{
    struct ifstub *stub;
    HRESULT hr;

    TRACE("oid=%s, stubbuffer=%p, iptr=%p, iid=%s\n",
          wine_dbgstr_longlong(m->oid), sb, iptr, debugstr_guid(iid));

    stub = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ifstub));
    if (!stub) return NULL;

    hr = RPC_CreateServerChannel(&stub->chan);
    if (hr != S_OK)
    {
        HeapFree(GetProcessHeap(), 0, stub);
        return NULL;
    }

    stub->stubbuffer = sb;
    if (sb) IRpcStubBuffer_AddRef(sb);

    IUnknown_AddRef(iptr);
    stub->iface = iptr;
    stub->flags = flags;
    stub->iid = *iid;

    /* FIXME: find a cleaner way of identifying that we are creating an ifstub
     * for the remunknown interface */
    if (flags & MSHLFLAGSP_REMUNKNOWN)
        stub->ipid = m->oxid_info.ipidRemUnknown;
    else
        generate_ipid(m, &stub->ipid);

    EnterCriticalSection(&m->lock);
    list_add_head(&m->ifstubs, &stub->entry);
    /* every normal marshal is counted so we don't allow more than we should */
    if (flags & MSHLFLAGS_NORMAL) m->norm_refs++;
    LeaveCriticalSection(&m->lock);

    TRACE("ifstub %p created with ipid %s\n", stub, debugstr_guid(&stub->ipid));

    return stub;
}

static void stub_manager_delete_ifstub(struct stub_manager *m, struct ifstub *ifstub)
{
    TRACE("m=%p, m->oid=%s, ipid=%s\n", m, wine_dbgstr_longlong(m->oid), debugstr_guid(&ifstub->ipid));

    list_remove(&ifstub->entry);

    RPC_UnregisterInterface(&ifstub->iid);

    if (ifstub->stubbuffer) IUnknown_Release(ifstub->stubbuffer);
    IUnknown_Release(ifstub->iface);
    IRpcChannelBuffer_Release(ifstub->chan);

    HeapFree(GetProcessHeap(), 0, ifstub);
}

/* returns TRUE if it is possible to unmarshal, FALSE otherwise. */
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid)
{
    BOOL ret = TRUE;
    struct ifstub *ifstub;

    if (!(ifstub = stub_manager_ipid_to_ifstub(m, ipid)))
    {
        ERR("attempted unmarshal of unknown IPID %s\n", debugstr_guid(ipid));
        return FALSE;
    }

    EnterCriticalSection(&m->lock);

    /* track normal marshals so we can enforce rules whilst in-process */
    if (ifstub->flags & MSHLFLAGS_NORMAL)
    {
        if (m->norm_refs)
            m->norm_refs--;
        else
        {
            ERR("attempted invalid normal unmarshal, norm_refs is zero\n");
            ret = FALSE;
        }
    }

    LeaveCriticalSection(&m->lock);

    return ret;
}

/* handles refcounting for CoReleaseMarshalData */
void stub_manager_release_marshal_data(struct stub_manager *m, ULONG refs, const IPID *ipid, BOOL tableweak)
{
    struct ifstub *ifstub;
 
    if (!(ifstub = stub_manager_ipid_to_ifstub(m, ipid)))
        return;
 
    if (ifstub->flags & MSHLFLAGS_TABLEWEAK)
        refs = 0;
    else if (ifstub->flags & MSHLFLAGS_TABLESTRONG)
        refs = 1;

    stub_manager_ext_release(m, refs, tableweak, TRUE);
}

/* is an ifstub table marshaled? */
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid)
{
    struct ifstub *ifstub = stub_manager_ipid_to_ifstub(m, ipid);
 
    assert( ifstub );
    
    return ifstub->flags & (MSHLFLAGS_TABLESTRONG | MSHLFLAGS_TABLEWEAK);
}


/*****************************************************************************
 *
 * IRemUnknown implementation
 *
 *
 * Note: this object is not related to the lifetime of a stub_manager, but it
 * interacts with stub managers.
 */

typedef struct rem_unknown
{
    const IRemUnknownVtbl *lpVtbl;
    LONG refs;
} RemUnknown;

static const IRemUnknownVtbl RemUnknown_Vtbl;


/* construct an IRemUnknown object with one outstanding reference */
static HRESULT RemUnknown_Construct(IRemUnknown **ppRemUnknown)
{
    RemUnknown *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &RemUnknown_Vtbl;
    This->refs = 1;

    *ppRemUnknown = (IRemUnknown *)This;
    return S_OK;
}

static HRESULT WINAPI RemUnknown_QueryInterface(IRemUnknown *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRemUnknown))
    {
        *ppv = (LPVOID)iface;
        IRemUnknown_AddRef(iface);
        return S_OK;
    }

    FIXME("No interface for iid %s\n", debugstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI RemUnknown_AddRef(IRemUnknown *iface)
{
    ULONG refs;
    RemUnknown *This = (RemUnknown *)iface;

    refs = InterlockedIncrement(&This->refs);

    TRACE("%p before: %d\n", iface, refs-1);
    return refs;
}

static ULONG WINAPI RemUnknown_Release(IRemUnknown *iface)
{
    ULONG refs;
    RemUnknown *This = (RemUnknown *)iface;

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
        HeapFree(GetProcessHeap(), 0, This);

    TRACE("%p after: %d\n", iface, refs);
    return refs;
}

static HRESULT WINAPI RemUnknown_RemQueryInterface(IRemUnknown *iface,
    REFIPID ripid, ULONG cRefs, USHORT cIids, IID *iids /* [size_is(cIids)] */,
    REMQIRESULT **ppQIResults /* [size_is(,cIids)] */)
{
    HRESULT hr;
    USHORT i;
    USHORT successful_qis = 0;
    APARTMENT *apt;
    struct stub_manager *stubmgr;

    TRACE("(%p)->(%s, %d, %d, %p, %p)\n", iface, debugstr_guid(ripid), cRefs, cIids, iids, ppQIResults);

    hr = ipid_to_stub_manager(ripid, &apt, &stubmgr);
    if (hr != S_OK) return hr;

    *ppQIResults = CoTaskMemAlloc(sizeof(REMQIRESULT) * cIids);

    for (i = 0; i < cIids; i++)
    {
        HRESULT hrobj = marshal_object(apt, &(*ppQIResults)[i].std, &iids[i],
                                       stubmgr->object, MSHLFLAGS_NORMAL);
        if (hrobj == S_OK)
            successful_qis++;
        (*ppQIResults)[i].hResult = hrobj;
    }

    stub_manager_int_release(stubmgr);
    apartment_release(apt);

    if (successful_qis == cIids)
        return S_OK; /* we got all requested interfaces */
    else if (successful_qis == 0)
        return E_NOINTERFACE; /* we didn't get any interfaces */
    else
        return S_FALSE; /* we got some interfaces */
}

static HRESULT WINAPI RemUnknown_RemAddRef(IRemUnknown *iface,
    USHORT cInterfaceRefs,
    REMINTERFACEREF* InterfaceRefs /* [size_is(cInterfaceRefs)] */,
    HRESULT *pResults /* [size_is(cInterfaceRefs)] */)
{
    HRESULT hr = S_OK;
    USHORT i;

    TRACE("(%p)->(%d, %p, %p)\n", iface, cInterfaceRefs, InterfaceRefs, pResults);

    for (i = 0; i < cInterfaceRefs; i++)
    {
        APARTMENT *apt;
        struct stub_manager *stubmgr;

        pResults[i] = ipid_to_stub_manager(&InterfaceRefs[i].ipid, &apt, &stubmgr);
        if (pResults[i] != S_OK)
        {
            hr = S_FALSE;
            continue;
        }

        stub_manager_ext_addref(stubmgr, InterfaceRefs[i].cPublicRefs, FALSE);
        if (InterfaceRefs[i].cPrivateRefs)
            FIXME("Adding %ld refs securely not implemented\n", InterfaceRefs[i].cPrivateRefs);

        stub_manager_int_release(stubmgr);
        apartment_release(apt);
    }

    return hr;
}

static HRESULT WINAPI RemUnknown_RemRelease(IRemUnknown *iface,
    USHORT cInterfaceRefs,
    REMINTERFACEREF* InterfaceRefs /* [size_is(cInterfaceRefs)] */)
{
    HRESULT hr = S_OK;
    USHORT i;

    TRACE("(%p)->(%d, %p)\n", iface, cInterfaceRefs, InterfaceRefs);

    for (i = 0; i < cInterfaceRefs; i++)
    {
        APARTMENT *apt;
        struct stub_manager *stubmgr;

        hr = ipid_to_stub_manager(&InterfaceRefs[i].ipid, &apt, &stubmgr);
        if (hr != S_OK)
        {
            hr = E_INVALIDARG;
            /* FIXME: we should undo any changes already made in this function */
            break;
        }

        stub_manager_ext_release(stubmgr, InterfaceRefs[i].cPublicRefs, FALSE, TRUE);
        if (InterfaceRefs[i].cPrivateRefs)
            FIXME("Releasing %ld refs securely not implemented\n", InterfaceRefs[i].cPrivateRefs);

        stub_manager_int_release(stubmgr);
        apartment_release(apt);
    }

    return hr;
}

static const IRemUnknownVtbl RemUnknown_Vtbl =
{
    RemUnknown_QueryInterface,
    RemUnknown_AddRef,
    RemUnknown_Release,
    RemUnknown_RemQueryInterface,
    RemUnknown_RemAddRef,
    RemUnknown_RemRelease
};

/* starts the IRemUnknown listener for the current apartment */
HRESULT start_apartment_remote_unknown(void)
{
    IRemUnknown *pRemUnknown;
    HRESULT hr = S_OK;
    APARTMENT *apt = COM_CurrentApt();

    EnterCriticalSection(&apt->cs);
    if (!apt->remunk_exported)
    {
        /* create the IRemUnknown object */
        hr = RemUnknown_Construct(&pRemUnknown);
        if (hr == S_OK)
        {
            STDOBJREF stdobjref; /* dummy - not used */
            /* register it with the stub manager */
            hr = marshal_object(apt, &stdobjref, &IID_IRemUnknown, (IUnknown *)pRemUnknown, MSHLFLAGS_NORMAL|MSHLFLAGSP_REMUNKNOWN);
            /* release our reference to the object as the stub manager will manage the life cycle for us */
            IRemUnknown_Release(pRemUnknown);
            if (hr == S_OK)
                apt->remunk_exported = TRUE;
        }
    }
    LeaveCriticalSection(&apt->cs);
    return hr;
}
