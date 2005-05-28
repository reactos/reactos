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
 * RO    - The value is read only. It never changes after creation, so no
 *         locking is required.
 * LOCK  - The value is written to only using Interlocked* functions.
 * CS    - The value is read or written to inside a critical section.
 *         The identifier following "CS" is the specific critical setion that
 *         must be used.
 * MUTEX - The value is read or written to with a mutex held.
 *         The identifier following "MUTEX" is the specific mutex that
 *         must be used.
 */

typedef enum ifstub_state
{
    STUBSTATE_NORMAL_MARSHALED,
    STUBSTATE_NORMAL_UNMARSHALED,
    STUBSTATE_TABLE_WEAK_MARSHALED,
    STUBSTATE_TABLE_WEAK_UNMARSHALED,
    STUBSTATE_TABLE_STRONG,
} STUB_STATE;

/* an interface stub */
struct ifstub   
{
    struct list       entry;      /* entry in stub_manager->ifstubs list (CS stub_manager->lock) */
    IRpcStubBuffer   *stubbuffer; /* RO */
    IID               iid;        /* RO */
    IPID              ipid;       /* RO */
    IUnknown         *iface;      /* RO */
};


/* stub managers hold refs on the object and each interface stub */
struct stub_manager
{
    struct list       entry;      /* entry in apartment stubmgr list (CS apt->cs) */
    struct list       ifstubs;    /* list of active ifstubs for the object (CS lock) */
    CRITICAL_SECTION  lock;
    APARTMENT        *apt;        /* owning apt (RO) */

    ULONG             extrefs;    /* number of 'external' references (CS lock) */
    ULONG             refs;       /* internal reference count (CS apt->cs) */
    OID               oid;        /* apartment-scoped unique identifier (RO) */
    IUnknown         *object;     /* the object we are managing the stub for (RO) */
    ULONG             next_ipid;  /* currently unused (LOCK) */
    STUB_STATE        state;      /* state machine (CS lock) */
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
  DWORD refs;              /* imported (public) references (MUTEX parent->remoting_mutex) */
  IRpcChannelBuffer *chan; /* channel to object (CS parent->cs) */
};

/* imported object / proxy manager */
struct proxy_manager
{
  const IMultiQIVtbl *lpVtbl;
  struct apartment *parent; /* owning apartment (RO) */
  struct list entry;        /* entry in apartment (CS parent->cs) */
  OXID oxid;                /* object exported ID (RO) */
  OID oid;                  /* object ID (RO) */
  struct list interfaces;   /* imported interfaces (CS cs) */
  DWORD refs;               /* proxy reference count (LOCK) */
  CRITICAL_SECTION cs;      /* thread safety for this object and children */
  ULONG sorflags;           /* STDOBJREF flags (RO) */
  IRemUnknown *remunk;      /* proxy to IRemUnknown used for lifecycle management (CS cs) */
  HANDLE remoting_mutex;    /* mutex used for synchronizing access to IRemUnknown */
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
  LONG remoting_started;   /* has the RPC system been started for this apartment? (LOCK) */

  /* FIXME: OID's should be given out by RPCSS */
  OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */
};

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
    DWORD            inits;        /* number of times CoInitializeEx called */
};


/* Global Interface Table Functions */

extern void* StdGlobalInterfaceTable_Construct(void);
extern void  StdGlobalInterfaceTable_Destroy(void* self);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);
extern void* StdGlobalInterfaceTableInstance;

/* FIXME: these shouldn't be needed, except for 16-bit functions */
extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);
HRESULT WINAPI __CLSIDFromStringA(LPCSTR idstr, CLSID *id);

HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);

/* Stub Manager */

ULONG stub_manager_int_addref(struct stub_manager *This);
ULONG stub_manager_int_release(struct stub_manager *This);
struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object, MSHLFLAGS mshlflags);
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs);
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs);
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid);
struct stub_manager *get_stub_manager(APARTMENT *apt, OID oid);
struct stub_manager *get_stub_manager_from_object(APARTMENT *apt, void *object);
BOOL stub_manager_notify_unmarshal(struct stub_manager *m);
BOOL stub_manager_is_table_marshaled(struct stub_manager *m);
void stub_manager_release_marshal_data(struct stub_manager *m, ULONG refs);
HRESULT ipid_to_stub_manager(const IPID *ipid, APARTMENT **stub_apt, struct stub_manager **stubmgr_ret);
IRpcStubBuffer *ipid_to_apt_and_stubbuffer(const IPID *ipid, APARTMENT **stub_apt);
HRESULT start_apartment_remote_unknown(void);

HRESULT marshal_object(APARTMENT *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *obj, MSHLFLAGS mshlflags);

/* RPC Backend */

struct dispatch_params;

void    RPC_StartRemoting(struct apartment *apt);
HRESULT RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid, IRpcChannelBuffer **pipebuf);
HRESULT RPC_ExecuteCall(struct dispatch_params *params);
HRESULT RPC_RegisterInterface(REFIID riid);
void    RPC_UnregisterInterface(REFIID riid);
void    RPC_StartLocalServer(REFCLSID clsid, IStream *stream);
HRESULT RPC_GetLocalClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void);

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void);

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable);


/* Apartment Functions */

APARTMENT *apartment_findfromoxid(OXID oxid, BOOL ref);
APARTMENT *apartment_findfromtid(DWORD tid);
DWORD apartment_addref(struct apartment *apt);
DWORD apartment_release(struct apartment *apt);
HRESULT apartment_disconnectproxies(struct apartment *apt);
void apartment_disconnectobject(struct apartment *apt, void *object);
static inline HRESULT apartment_getoxid(struct apartment *apt, OXID *oxid)
{
    *oxid = apt->oxid;
    return S_OK;
}


/* DCOM messages used by the apartment window (not compatible with native) */
#define DM_EXECUTERPC   (WM_USER + 0) /* WPARAM = 0, LPARAM = (struct dispatch_params *) */

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

/* helpers for debugging */
#ifdef __i386__
# define DEBUG_SET_CRITSEC_NAME(cs, name) (cs)->DebugInfo->Spare[1] = (DWORD)(__FILE__ ": " name)
# define DEBUG_CLEAR_CRITSEC_NAME(cs) (cs)->DebugInfo->Spare[1] = 0
#else
# define DEBUG_SET_CRITSEC_NAME(cs, name)
# define DEBUG_CLEAR_CRITSEC_NAME(cs)
#endif

extern HINSTANCE OLE32_hInstance; /* FIXME: make static */

#endif /* __WINE_OLE_COMPOBJ_H */
