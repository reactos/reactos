/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
 * Copyright 2003 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Mike Hearn, Rob Shearman, CodeWeavers Inc
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

#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include <stdarg.h>

#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dcom.h"
#include "winreg.h"
#include "winternl.h"

struct apartment;
typedef struct apartment APARTMENT;

/* Thread-safety Annotation Legend:
 *
 * RO   - The value is read only. It never changes after creation, so no
 *        locking is required.
 * LOCK - The value is written to only using Interlocked* functions.
 * CS   - The value is read or written to with a critical section held.
 *        The identifier following "CS" is the specific critical section that
 *        must be used.
 */

typedef enum ifstub_state
{
    IFSTUB_STATE_NORMAL_MARSHALED,
    IFSTUB_STATE_NORMAL_UNMARSHALED,
    IFSTUB_STATE_TABLE_MARSHALED
} IFSTUB_STATE;

/* an interface stub */
struct ifstub   
{
    struct list       entry;      /* entry in stub_manager->ifstubs list (CS stub_manager->lock) */
    IRpcStubBuffer   *stubbuffer; /* RO */
    IID               iid;        /* RO */
    IPID              ipid;       /* RO */
    IUnknown         *iface;      /* RO */
    IFSTUB_STATE      state;      /* CS stub_manager->lock */
};


/* stub managers hold refs on the object and each interface stub */
struct stub_manager
{
    struct list       entry;      /* entry in apartment stubmgr list (CS apt->cs) */
    struct list       ifstubs;    /* list of active ifstubs for the object (CS lock) */
    CRITICAL_SECTION  lock;
    APARTMENT        *apt;        /* owning apt (RO) */

    ULONG             extrefs;    /* number of 'external' references (LOCK) */
    ULONG             refs;       /* internal reference count (CS apt->cs) */
    OID               oid;        /* apartment-scoped unique identifier (RO) */
    IUnknown         *object;     /* the object we are managing the stub for (RO) */
    ULONG             next_ipid;  /* currently unused (LOCK) */
};

/* imported interface proxy */
struct ifproxy
{
  struct list entry;       /* entry in proxy_manager list (CS parent->cs) */
  struct proxy_manager *parent; /* owning proxy_manager (RO) */
  LPVOID iface;            /* interface pointer (RO) */
  IID iid;                 /* interface ID (RO) */
  IPID ipid;               /* imported interface ID (RO) */
  LPRPCPROXYBUFFER proxy;  /* interface proxy (RO) */
  DWORD refs;              /* imported (public) references (CS parent->cs) */
};

/* imported object / proxy manager */
struct proxy_manager
{
  const IMultiQIVtbl *lpVtbl;
  struct apartment *parent; /* owning apartment (RO) */
  struct list entry;        /* entry in apartment (CS parent->cs) */
  LPRPCCHANNELBUFFER chan;  /* channel to object (CS cs) */
  OXID oxid;                /* object exported ID (RO) */
  OID oid;                  /* object ID (RO) */
  struct list interfaces;   /* imported interfaces (CS cs) */
  DWORD refs;               /* proxy reference count (LOCK) */
  CRITICAL_SECTION cs;      /* thread safety for this object and children */
  ULONG sorflags;           /* STDOBJREF flags (RO) */
  IRemUnknown *remunk;      /* proxy to IRemUnknown used for lifecycle management (CS cs) */
};

/* this needs to become a COM object that implements IRemUnknown */
struct apartment
{
  struct list entry;       

  DWORD refs;              /* refcount of the apartment (LOCK) */
  DWORD model;             /* threading model (RO) */
  DWORD tid;               /* thread id (RO) */
  HANDLE thread;           /* thread handle (RO) */
  OXID oxid;               /* object exporter ID (RO) */
  DWORD ipidc;             /* interface pointer ID counter, starts at 1 (LOCK) */
  HWND win;                /* message window (RO) */
  CRITICAL_SECTION cs;     /* thread safety */
  LPMESSAGEFILTER filter;  /* message filter (CS cs) */
  struct list proxies;     /* imported objects (CS cs) */
  struct list stubmgrs;    /* stub managers for exported objects (CS cs) */
  BOOL remunk_exported;    /* has the IRemUnknown interface for this apartment been created yet? (CS cs) */

  /* FIXME: These should all be removed long term as they leak information that should be encapsulated */
  OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */
  DWORD listenertid;       /* id of apartment_listener_thread */
  HANDLE shutdown_event;   /* event used to tell the client_dispatch_thread to shut down */
};

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
    DWORD            inits;        /* number of times CoInitializeEx called */
};

extern void* StdGlobalInterfaceTable_Construct(void);
extern void  StdGlobalInterfaceTable_Destroy(void* self);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);

/* FIXME: these shouldn't be needed, except for 16-bit functions */
extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);
HRESULT WINAPI __CLSIDFromStringA(LPCSTR idstr, CLSID *id);

extern HRESULT create_marshalled_proxy(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

extern void* StdGlobalInterfaceTableInstance;

/* Standard Marshalling definitions */
typedef struct _wine_marshal_id {
    OXID    oxid;       /* id of apartment */
    OID     oid;        /* id of stub manager */
    IPID    ipid;       /* id of interface pointer */
} wine_marshal_id;

inline static BOOL
MARSHAL_Compare_Mids(wine_marshal_id *mid1,wine_marshal_id *mid2) {
    return
	(mid1->oxid == mid2->oxid)	&&
	(mid1->oid == mid2->oid)	&&
	IsEqualGUID(&(mid1->ipid),&(mid2->ipid))
    ;
}

HRESULT MARSHAL_Disconnect_Proxies(APARTMENT *apt);
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);

ULONG stub_manager_int_addref(struct stub_manager *This);
ULONG stub_manager_int_release(struct stub_manager *This);
struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object);
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs);
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs);
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid, BOOL tablemarshal);
struct stub_manager *get_stub_manager(APARTMENT *apt, OID oid);
struct stub_manager *get_stub_manager_from_object(APARTMENT *apt, void *object);
void apartment_disconnect_object(APARTMENT *apt, void *object);
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid);
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid);
HRESULT register_ifstub(APARTMENT *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *obj, MSHLFLAGS mshlflags);
HRESULT ipid_to_stub_manager(const IPID *ipid, APARTMENT **stub_apt, struct stub_manager **stubmgr_ret);
IRpcStubBuffer *ipid_to_stubbuffer(const IPID *ipid);
HRESULT start_apartment_remote_unknown(void);

IRpcStubBuffer *mid_to_stubbuffer(wine_marshal_id *mid);

void start_apartment_listener_thread(void);

extern HRESULT PIPE_GetNewPipeBuf(wine_marshal_id *mid, IRpcChannelBuffer **pipebuf);
void RPC_StartLocalServer(REFCLSID clsid, IStream *stream);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void);

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void);

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int WINAPI FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable);

/* compobj.c */
APARTMENT *COM_ApartmentFromOXID(OXID oxid, BOOL ref);
APARTMENT *COM_ApartmentFromTID(DWORD tid);
DWORD COM_ApartmentAddRef(struct apartment *apt);
DWORD COM_ApartmentRelease(struct apartment *apt);

/*
 * Per-thread values are stored in the TEB on offset 0xF80,
 * see http://www.microsoft.com/msj/1099/bugslayer/bugslayer1099.htm
 */

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    if (!NtCurrentTeb()->ReservedForOle)
        NtCurrentTeb()->ReservedForOle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct oletls));

    return NtCurrentTeb()->ReservedForOle;
}

static inline APARTMENT* COM_CurrentApt(void)
{  
    return COM_CurrentInfo()->apt;
}

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#endif /* __WINE_OLE_COMPOBJ_H */
