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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
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
    IUnknown_AddRef(object);
    sm->object = object;
    sm->apt    = apt;

    /* start off with 2 references because the stub is in the apartment
     * and the caller will also hold a reference */
    sm->refs   = 2;

    /* yes, that's right, this starts at zero. that's zero EXTERNAL
     * refs, ie nobody has unmarshalled anything yet. we can't have
     * negative refs because the stub manager cannot be explicitly
     * killed, it has to die by somebody unmarshalling then releasing
     * the marshalled ifptr.
     */
    sm->extrefs = 0;
    
    EnterCriticalSection(&apt->cs);
    sm->oid    = apt->oidc++;
    list_add_head(&apt->stubmgrs, &sm->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("Created new stub manager (oid=%s) at %p for object with IUnknown %p\n", wine_dbgstr_longlong(sm->oid), sm, object);
    
    return sm;
}

/* m->apt->cs must be held on entry to this function */
static void stub_manager_delete(struct stub_manager *m)
{
    struct list *cursor;

    TRACE("destroying %p (oid=%s)\n", m, wine_dbgstr_longlong(m->oid));

    list_remove(&m->entry);

    /* release every ifstub */
    while ((cursor = list_head(&m->ifstubs)))
    {
        struct ifstub *ifstub = LIST_ENTRY(cursor, struct ifstub, entry);
        stub_manager_delete_ifstub(m, ifstub);
    }

    IUnknown_Release(m->object);

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
void apartment_disconnect_object(APARTMENT *apt, void *object)
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

    TRACE("before %ld\n", refs - 1);

    return refs;
}

/* decrements the internal refcount */
ULONG stub_manager_int_release(struct stub_manager *This)
{
    ULONG refs;
    APARTMENT *apt = This->apt;

    EnterCriticalSection(&apt->cs);
    refs = --This->refs;

    TRACE("after %ld\n", refs);

    if (!refs)
        stub_manager_delete(This);
    LeaveCriticalSection(&apt->cs);

    return refs;
}

/* add some external references (ie from a client that unmarshaled an ifptr) */
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs)
{
    ULONG rc = InterlockedExchangeAdd(&m->extrefs, refs) + refs;

    TRACE("added %lu refs to %p (oid %s), rc is now %lu\n", refs, m, wine_dbgstr_longlong(m->oid), rc);

    return rc;
}

/* remove some external references */
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs)
{
    ULONG rc = InterlockedExchangeAdd(&m->extrefs, -refs) - refs;
    
    TRACE("removed %lu refs from %p (oid %s), rc is now %lu\n", refs, m, wine_dbgstr_longlong(m->oid), rc);

    if (rc == 0)
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

HRESULT ipid_to_stub_manager(const IPID *ipid, APARTMENT **stub_apt, struct stub_manager **stubmgr_ret)
{
    /* FIXME: hack for IRemUnknown */
    if (ipid->Data2 == 0xffff)
        *stub_apt = COM_ApartmentFromOXID(*(OXID *)ipid->Data4, TRUE);
    else
        *stub_apt = COM_ApartmentFromTID(ipid->Data2);
    if (!*stub_apt)
    {
        ERR("Couldn't find apartment corresponding to TID 0x%04x\n", ipid->Data2);
        return RPC_E_INVALID_OBJECT;
    }
    *stubmgr_ret = get_stub_manager_from_ipid(*stub_apt, ipid);
    if (!*stubmgr_ret)
    {
        COM_ApartmentRelease(*stub_apt);
        *stub_apt = NULL;
        return RPC_E_INVALID_OBJECT;
    }
    return S_OK;
}

IRpcStubBuffer *ipid_to_stubbuffer(const IPID *ipid)
{
    IRpcStubBuffer *ret = NULL;
    APARTMENT *apt;
    struct stub_manager *stubmgr;
    struct ifstub *ifstub;
    HRESULT hr;

    hr = ipid_to_stub_manager(ipid, &apt, &stubmgr);
    if (hr != S_OK) return NULL;

    ifstub = stub_manager_ipid_to_ifstub(stubmgr, ipid);
    if (ifstub)
        ret = ifstub->stubbuffer;

    stub_manager_int_release(stubmgr);

    COM_ApartmentRelease(apt);

    return ret;
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
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid, BOOL tablemarshal)
{
    struct ifstub *stub;

    TRACE("oid=%s, stubbuffer=%p, iptr=%p, iid=%s, tablemarshal=%s\n",
          wine_dbgstr_longlong(m->oid), sb, iptr, debugstr_guid(iid), tablemarshal ? "TRUE" : "FALSE");

    stub = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ifstub));
    if (!stub) return NULL;

    stub->stubbuffer = sb;
    IUnknown_AddRef(sb);

    /* no need to ref this, same object as sb */
    stub->iface = iptr;

    if (tablemarshal)
        stub->state = IFSTUB_STATE_TABLE_MARSHALED;
    else
        stub->state = IFSTUB_STATE_NORMAL_MARSHALED;

    stub->iid = *iid;

    /* FIXME: hack for IRemUnknown because we don't notify SCM of our IPID
     * yet, so we need to use a well-known one */
    if (IsEqualIID(iid, &IID_IRemUnknown))
    {
        stub->ipid.Data1 = 0xffffffff;
        stub->ipid.Data2 = 0xffff;
        stub->ipid.Data3 = 0xffff;
        assert(sizeof(stub->ipid.Data4) == sizeof(m->apt->oxid));
        memcpy(&stub->ipid.Data4, &m->apt->oxid, sizeof(OXID));
    }
    else
        generate_ipid(m, &stub->ipid);

    EnterCriticalSection(&m->lock);
    list_add_head(&m->ifstubs, &stub->entry);
    LeaveCriticalSection(&m->lock);

    TRACE("ifstub %p created with ipid %s\n", stub, debugstr_guid(&stub->ipid));

    return stub;
}

static void stub_manager_delete_ifstub(struct stub_manager *m, struct ifstub *ifstub)
{
    TRACE("m=%p, m->oid=%s, ipid=%s\n", m, wine_dbgstr_longlong(m->oid), debugstr_guid(&ifstub->ipid));
    
    list_remove(&ifstub->entry);
        
    IUnknown_Release(ifstub->stubbuffer);
    IUnknown_Release(ifstub->iface);

    HeapFree(GetProcessHeap(), 0, ifstub);
}

/* returns TRUE if it is possible to unmarshal, FALSE otherwise. */
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid)
{
    struct ifstub *ifstub;
    BOOL ret;

    ifstub = stub_manager_ipid_to_ifstub(m, ipid);
    if (!ifstub)
    {
        WARN("Can't find ifstub for OID %s, IPID %s\n",
            wine_dbgstr_longlong(m->oid), wine_dbgstr_guid(ipid));
        return FALSE;
    }

    EnterCriticalSection(&m->lock);

    switch (ifstub->state)
    {
    case IFSTUB_STATE_TABLE_MARSHALED:
        ret = TRUE;
        break;
    case IFSTUB_STATE_NORMAL_MARSHALED:
        ifstub->state = IFSTUB_STATE_NORMAL_UNMARSHALED;
        ret = TRUE;
        break;
    default:
        WARN("object OID %s, IPID %s already unmarshaled\n",
            wine_dbgstr_longlong(m->oid), wine_dbgstr_guid(ipid));
        ret = FALSE;
        break;
    }

    LeaveCriticalSection(&m->lock);

    return ret;
}

/* is an ifstub table marshaled? */
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid)
{
    struct ifstub *ifstub;
    BOOL ret;

    ifstub = stub_manager_ipid_to_ifstub(m, ipid);
    if (!ifstub)
    {
        WARN("Can't find ifstub for OID %s, IPID %s\n",
            wine_dbgstr_longlong(m->oid), wine_dbgstr_guid(ipid));
        return FALSE;
    }

    EnterCriticalSection(&m->lock);
    ret = (ifstub->state == IFSTUB_STATE_TABLE_MARSHALED);
    LeaveCriticalSection(&m->lock);

    return ret;
}


/*****************************************************************************
 *
 * IRemUnknown implementation
 *
 *
 * Note: this object is not related to the lifetime of a stub_manager, but it
 * interacts with stub managers.
 */

const IID IID_IRemUnknown = { 0x00000131, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46} };

typedef struct rem_unknown
{
    const IRemUnknownVtbl *lpVtbl;
    ULONG refs;
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

    TRACE("%p before: %ld\n", iface, refs-1);
    return refs;
}

static ULONG WINAPI RemUnknown_Release(IRemUnknown *iface)
{
    ULONG refs;
    RemUnknown *This = (RemUnknown *)iface;

    refs = InterlockedDecrement(&This->refs);
    if (!refs)
        HeapFree(GetProcessHeap(), 0, This);

    TRACE("%p after: %ld\n", iface, refs);
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

    TRACE("(%p)->(%s, %ld, %d, %p, %p)\n", iface, debugstr_guid(ripid), cRefs, cIids, iids, ppQIResults);

    hr = ipid_to_stub_manager(ripid, &apt, &stubmgr);
    if (hr != S_OK) return hr;

    *ppQIResults = CoTaskMemAlloc(sizeof(REMQIRESULT) * cIids);

    for (i = 0; i < cIids; i++)
    {
        HRESULT hrobj = register_ifstub(apt, &(*ppQIResults)[i].std, &iids[i],
                                        stubmgr->object, MSHLFLAGS_NORMAL);
        if (hrobj == S_OK)
            successful_qis++;
        (*ppQIResults)[i].hResult = hrobj;
    }

    stub_manager_int_release(stubmgr);
    COM_ApartmentRelease(apt);

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

        stub_manager_ext_addref(stubmgr, InterfaceRefs[i].cPublicRefs);
        if (InterfaceRefs[i].cPrivateRefs)
            FIXME("Adding %ld refs securely not implemented\n", InterfaceRefs[i].cPrivateRefs);

        stub_manager_int_release(stubmgr);
        COM_ApartmentRelease(apt);
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

        stub_manager_ext_release(stubmgr, InterfaceRefs[i].cPublicRefs);
        if (InterfaceRefs[i].cPrivateRefs)
            FIXME("Releasing %ld refs securely not implemented\n", InterfaceRefs[i].cPrivateRefs);

        stub_manager_int_release(stubmgr);
        COM_ApartmentRelease(apt);
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
HRESULT start_apartment_remote_unknown()
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
            hr = register_ifstub(COM_CurrentApt(), &stdobjref, &IID_IRemUnknown, (IUnknown *)pRemUnknown, MSHLFLAGS_NORMAL);
            /* release our reference to the object as the stub manager will manage the life cycle for us */
            IRemUnknown_Release(pRemUnknown);
            if (hr == S_OK)
                apt->remunk_exported = TRUE;
        }
    }
    LeaveCriticalSection(&apt->cs);
    return hr;
}
