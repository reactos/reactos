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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

DEFINE_OLEGUID( CLSID_DfMarshal, 0x0000030b, 0, 0 );
DEFINE_OLEGUID( CLSID_PSFactoryBuffer, 0x00000320, 0, 0 );

/* signal to stub manager that this is a rem unknown object */
#define MSHLFLAGSP_REMUNKNOWN   0x80000000

/* Thread-safety Annotation Legend:
 *
 * RO    - The value is read only. It never changes after creation, so no
 *         locking is required.
 * LOCK  - The value is written to only using Interlocked* functions.
 * CS    - The value is read or written to inside a critical section.
 *         The identifier following "CS" is the specific critical section that
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
    MSHLFLAGS         flags;      /* so we can enforce process-local marshalling rules (RO) */
    IRpcChannelBuffer*chan;       /* channel passed to IRpcStubBuffer::Invoke (RO) */
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
    ULONG             weakrefs;   /* number of weak references (CS lock) */
    OID               oid;        /* apartment-scoped unique identifier (RO) */
    IUnknown         *object;     /* the object we are managing the stub for (RO) */
    ULONG             next_ipid;  /* currently unused (LOCK) */
    OXID_INFO         oxid_info;  /* string binding, ipid of rem unknown and other information (RO) */

    /* We need to keep a count of the outstanding marshals, so we can enforce the
     * marshalling rules (ie, you can only unmarshal normal marshals once). Note
     * that these counts do NOT include unmarshalled interfaces, once a stream is
     * unmarshalled and a proxy set up, this count is decremented.
     */

    ULONG             norm_refs;  /* refcount of normal marshals (CS lock) */
};

/* imported interface proxy */
struct ifproxy
{
  struct list entry;       /* entry in proxy_manager list (CS parent->cs) */
  struct proxy_manager *parent; /* owning proxy_manager (RO) */
  LPVOID iface;            /* interface pointer (RO) */
  STDOBJREF stdobjref;     /* marshal data that represents this object (RO) */
  IID iid;                 /* interface ID (RO) */
  LPRPCPROXYBUFFER proxy;  /* interface proxy (RO) */
  ULONG refs;              /* imported (public) references (LOCK) */
  IRpcChannelBuffer *chan; /* channel to object (CS parent->cs) */
};

/* imported object / proxy manager */
struct proxy_manager
{
  const IMultiQIVtbl *lpVtbl;
  const IMarshalVtbl *lpVtblMarshal;
  const IClientSecurityVtbl *lpVtblCliSec;
  struct apartment *parent; /* owning apartment (RO) */
  struct list entry;        /* entry in apartment (CS parent->cs) */
  OXID oxid;                /* object exported ID (RO) */
  OXID_INFO oxid_info;      /* string binding, ipid of rem unknown and other information (RO) */
  OID oid;                  /* object ID (RO) */
  struct list interfaces;   /* imported interfaces (CS cs) */
  LONG refs;                /* proxy reference count (LOCK) */
  CRITICAL_SECTION cs;      /* thread safety for this object and children */
  ULONG sorflags;           /* STDOBJREF flags (RO) */
  IRemUnknown *remunk;      /* proxy to IRemUnknown used for lifecycle management (CS cs) */
  HANDLE remoting_mutex;    /* mutex used for synchronizing access to IRemUnknown */
  MSHCTX dest_context;      /* context used for activating optimisations (LOCK) */
  void *dest_context_data;  /* reserved context value (LOCK) */
};

struct apartment
{
  struct list entry;

  LONG  refs;              /* refcount of the apartment (LOCK) */
  BOOL multi_threaded;     /* multi-threaded or single-threaded apartment? (RO) */
  DWORD tid;               /* thread id (RO) */
  OXID oxid;               /* object exporter ID (RO) */
  LONG ipidc;              /* interface pointer ID counter, starts at 1 (LOCK) */
  CRITICAL_SECTION cs;     /* thread safety */
  struct list proxies;     /* imported objects (CS cs) */
  struct list stubmgrs;    /* stub managers for exported objects (CS cs) */
  BOOL remunk_exported;    /* has the IRemUnknown interface for this apartment been created yet? (CS cs) */
  LONG remoting_started;   /* has the RPC system been started for this apartment? (LOCK) */
  struct list psclsids;    /* list of registered PS CLSIDs (CS cs) */
  struct list loaded_dlls; /* list of dlls loaded by this apartment (CS cs) */
  DWORD host_apt_tid;      /* thread ID of apartment hosting objects of differing threading model (CS cs) */
  HWND host_apt_hwnd;      /* handle to apartment window of host apartment (CS cs) */

  /* FIXME: OID's should be given out by RPCSS */
  OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */

  /* STA-only fields */
  HWND win;                /* message window (LOCK) */
  LPMESSAGEFILTER filter;  /* message filter (CS cs) */
  BOOL main;               /* is this a main-threaded-apartment? (RO) */
};

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
    IInitializeSpy   *spy;         /* The "SPY" from CoInitializeSpy */
    DWORD            inits;        /* number of times CoInitializeEx called */
    DWORD            ole_inits;    /* number of times OleInitialize called */
    GUID             causality_id; /* unique identifier for each COM call */
    LONG             pending_call_count_client; /* number of client calls pending */
    LONG             pending_call_count_server; /* number of server calls pending */
};


/* Global Interface Table Functions */

extern void* StdGlobalInterfaceTable_Construct(void);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);
extern void* StdGlobalInterfaceTableInstance;

/* FIXME: these shouldn't be needed, except for 16-bit functions */
extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);

HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *key);
HRESULT COM_OpenKeyForAppIdFromCLSID(REFCLSID clsid, REGSAM access, HKEY *subkey);
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);
HRESULT FTMarshalCF_Create(REFIID riid, LPVOID *ppv);

/* Stub Manager */

ULONG stub_manager_int_addref(struct stub_manager *This);
ULONG stub_manager_int_release(struct stub_manager *This);
struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object);
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs, BOOL tableweak);
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs, BOOL tableweak, BOOL last_unlock_releases);
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid, MSHLFLAGS flags);
struct ifstub *stub_manager_find_ifstub(struct stub_manager *m, REFIID iid, MSHLFLAGS flags);
struct stub_manager *get_stub_manager(APARTMENT *apt, OID oid);
struct stub_manager *get_stub_manager_from_object(APARTMENT *apt, void *object);
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid);
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid);
void stub_manager_release_marshal_data(struct stub_manager *m, ULONG refs, const IPID *ipid, BOOL tableweak);
HRESULT ipid_get_dispatch_params(const IPID *ipid, APARTMENT **stub_apt, IRpcStubBuffer **stub, IRpcChannelBuffer **chan, IID *iid, IUnknown **iface);
HRESULT start_apartment_remote_unknown(void);

HRESULT marshal_object(APARTMENT *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *obj, MSHLFLAGS mshlflags);

/* RPC Backend */

struct dispatch_params;

void    RPC_StartRemoting(struct apartment *apt);
HRESULT RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid,
                                const OXID_INFO *oxid_info,
                                DWORD dest_context, void *dest_context_data,
                                IRpcChannelBuffer **chan);
HRESULT RPC_CreateServerChannel(IRpcChannelBuffer **chan);
void    RPC_ExecuteCall(struct dispatch_params *params);
HRESULT RPC_RegisterInterface(REFIID riid);
void    RPC_UnregisterInterface(REFIID riid);
HRESULT RPC_StartLocalServer(REFCLSID clsid, IStream *stream, BOOL multi_use, void **registration);
void    RPC_StopLocalServer(void *registration);
HRESULT RPC_GetLocalClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv);
HRESULT RPC_RegisterChannelHook(REFGUID rguid, IChannelHook *hook);
void    RPC_UnregisterAllChannelHooks(void);
HRESULT RPC_ResolveOxid(OXID oxid, OXID_INFO *oxid_info);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void);

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void);

/* Drag and drop */
void OLEDD_UnInitialize(void);

/* Apartment Functions */

APARTMENT *apartment_findfromoxid(OXID oxid, BOOL ref);
APARTMENT *apartment_findfromtid(DWORD tid);
DWORD apartment_addref(struct apartment *apt);
DWORD apartment_release(struct apartment *apt);
HRESULT apartment_disconnectproxies(struct apartment *apt);
void apartment_disconnectobject(struct apartment *apt, void *object);
static inline HRESULT apartment_getoxid(const struct apartment *apt, OXID *oxid)
{
    *oxid = apt->oxid;
    return S_OK;
}
HRESULT apartment_createwindowifneeded(struct apartment *apt);
HWND apartment_getwindow(const struct apartment *apt);
void apartment_joinmta(void);


/* DCOM messages used by the apartment window (not compatible with native) */
#define DM_EXECUTERPC   (WM_USER + 0) /* WPARAM = 0, LPARAM = (struct dispatch_params *) */
#define DM_HOSTOBJECT   (WM_USER + 1) /* WPARAM = 0, LPARAM = (struct host_object_params *) */

/*
 * Per-thread values are stored in the TEB on offset 0xF80
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

static inline GUID COM_CurrentCausalityId(void)
{
    struct oletls *info = COM_CurrentInfo();
    if (!info)
        return GUID_NULL;
    if (IsEqualGUID(&info->causality_id, &GUID_NULL))
        CoCreateGuid(&info->causality_id);
    return info->causality_id;
}

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/* helpers for debugging */
# define DEBUG_SET_CRITSEC_NAME(cs, name) (cs)->DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": " name)
# define DEBUG_CLEAR_CRITSEC_NAME(cs) (cs)->DebugInfo->Spare[0] = 0

#define CHARS_IN_GUID 39 /* including NULL */

#define WINE_CLSCTX_DONT_HOST   0x80000000

/* from dlldata.c */
extern HINSTANCE hProxyDll DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLE32_DllUnregisterServer(void) DECLSPEC_HIDDEN;

/* Exported non-interface Data Advise Holder functions */
HRESULT DataAdviseHolder_OnConnect(IDataAdviseHolder *iface, IDataObject *pDelegate);
void DataAdviseHolder_OnDisconnect(IDataAdviseHolder *iface);

#endif /* __WINE_OLE_COMPOBJ_H */
