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

    while ((cursor = list_head(&m->ifstubs)))
    {
        struct ifstub *ifstub = LIST_ENTRY(cursor, struct ifstub, entry);
        stub_manager_delete_ifstub(m, ifstub);
    }

    IUnknown_Release(m->object);

    DeleteCriticalSection(&m->lock);

    HeapFree(GetProcessHeap(), 0, m);
}

/* gets the stub manager associated with an object - caller must call
 * release on the stub manager when it is no longer needed */
struct stub_manager *get_stub_manager_from_object(OXID oxid, void *object)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;
    APARTMENT           *apt;

    if (!(apt = COM_ApartmentFromOXID(oxid, TRUE)))
    {
        WARN("Could not map OXID %s to apartment object\n", wine_dbgstr_longlong(oxid));
        return NULL;
    }

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

    COM_ApartmentRelease(apt);
    
    TRACE("found %p from object %p\n", result, object);

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

/* gets the stub manager associated with an object id - caller must call
 * release on the stub manager when it is no longer needed */
struct stub_manager *get_stub_manager(OXID oxid, OID oid)
{
    struct stub_manager *result = NULL;
    struct list         *cursor;
    APARTMENT           *apt;

    if (!(apt = COM_ApartmentFromOXID(oxid, TRUE)))
    {
        WARN("Could not map OXID %s to apartment object\n", wine_dbgstr_longlong(oxid));
        return NULL;
    }
    
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

    COM_ApartmentRelease(apt);

    TRACE("found %p from oid %s\n", result, wine_dbgstr_longlong(oid));

    return result;
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

IRpcStubBuffer *stub_manager_ipid_to_stubbuffer(struct stub_manager *m, const IPID *ipid)
{
    struct ifstub *ifstub = stub_manager_ipid_to_ifstub(m, ipid);
    
    return ifstub ? ifstub->stubbuffer : NULL;
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
    stub->table = tablemarshal;
    stub->iid = *iid;
    stub->ipid = *iid; /* FIXME: should be globally unique */

    EnterCriticalSection(&m->lock);
    list_add_head(&m->ifstubs, &stub->entry);
    LeaveCriticalSection(&m->lock);

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
