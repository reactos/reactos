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

struct apartment;
typedef struct apartment APARTMENT;
typedef struct LocalServer LocalServer;

DEFINE_OLEGUID( CLSID_DfMarshal, 0x0000030b, 0, 0 );

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

    IExternalConnection *extern_conn;

    /* We need to keep a count of the outstanding marshals, so we can enforce the
     * marshalling rules (ie, you can only unmarshal normal marshals once). Note
     * that these counts do NOT include unmarshalled interfaces, once a stream is
     * unmarshalled and a proxy set up, this count is decremented.
     */

    ULONG             norm_refs;  /* refcount of normal marshals (CS lock) */
    BOOL              disconnected; /* CoDisconnectObject has been called (CS lock) */
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
  LocalServer *local_server; /* A marshallable object exposing local servers (CS cs) */

  /* FIXME: OIDs should be given out by RPCSS */
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
    DWORD             apt_mask;    /* apartment mask (+0Ch on x86) */
    IInitializeSpy   *spy;         /* The "SPY" from CoInitializeSpy */
    DWORD            inits;        /* number of times CoInitializeEx called */
    DWORD            ole_inits;    /* number of times OleInitialize called */
    GUID             causality_id; /* unique identifier for each COM call */
    LONG             pending_call_count_client; /* number of client calls pending */
    LONG             pending_call_count_server; /* number of server calls pending */
    DWORD            unknown;
    IObjContext     *context_token; /* (+38h on x86) */
    IUnknown        *call_state;    /* current call context (+3Ch on x86) */
    DWORD            unknown2[46];
    IUnknown        *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
};


/* Global Interface Table Functions */
extern IGlobalInterfaceTable *get_std_git(void) DECLSPEC_HIDDEN;
extern void release_std_git(void) DECLSPEC_HIDDEN;
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *key) DECLSPEC_HIDDEN;
HRESULT COM_OpenKeyForAppIdFromCLSID(REFCLSID clsid, REGSAM access, HKEY *subkey) DECLSPEC_HIDDEN;
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT FTMarshalCF_Create(REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

/* Stub Manager */

ULONG stub_manager_int_release(struct stub_manager *This) DECLSPEC_HIDDEN;
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs, BOOL tableweak) DECLSPEC_HIDDEN;
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs, BOOL tableweak, BOOL last_unlock_releases) DECLSPEC_HIDDEN;
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, REFIID iid,
     DWORD dest_context, void *dest_context_data, MSHLFLAGS flags) DECLSPEC_HIDDEN;
struct ifstub *stub_manager_find_ifstub(struct stub_manager *m, REFIID iid, MSHLFLAGS flags) DECLSPEC_HIDDEN;
struct stub_manager *get_stub_manager(APARTMENT *apt, OID oid) DECLSPEC_HIDDEN;
struct stub_manager *get_stub_manager_from_object(APARTMENT *apt, IUnknown *object, BOOL alloc) DECLSPEC_HIDDEN;
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid) DECLSPEC_HIDDEN;
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid) DECLSPEC_HIDDEN;
void stub_manager_release_marshal_data(struct stub_manager *m, ULONG refs, const IPID *ipid, BOOL tableweak) DECLSPEC_HIDDEN;
void stub_manager_disconnect(struct stub_manager *m) DECLSPEC_HIDDEN;
HRESULT ipid_get_dispatch_params(const IPID *ipid, APARTMENT **stub_apt, struct stub_manager **manager, IRpcStubBuffer **stub,
                                 IRpcChannelBuffer **chan, IID *iid, IUnknown **iface) DECLSPEC_HIDDEN;
HRESULT start_apartment_remote_unknown(void) DECLSPEC_HIDDEN;

HRESULT marshal_object(APARTMENT *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *obj, DWORD dest_context, void *dest_context_data, MSHLFLAGS mshlflags) DECLSPEC_HIDDEN;

/* RPC Backend */

struct dispatch_params;

void    RPC_StartRemoting(struct apartment *apt) DECLSPEC_HIDDEN;
HRESULT RPC_CreateClientChannel(const OXID *oxid, const IPID *ipid,
                                const OXID_INFO *oxid_info,
                                DWORD dest_context, void *dest_context_data,
                                IRpcChannelBuffer **chan) DECLSPEC_HIDDEN;
HRESULT RPC_CreateServerChannel(DWORD dest_context, void *dest_context_data, IRpcChannelBuffer **chan) DECLSPEC_HIDDEN;
void    RPC_ExecuteCall(struct dispatch_params *params) DECLSPEC_HIDDEN;
HRESULT RPC_RegisterInterface(REFIID riid) DECLSPEC_HIDDEN;
void    RPC_UnregisterInterface(REFIID riid, BOOL wait) DECLSPEC_HIDDEN;
HRESULT RPC_StartLocalServer(REFCLSID clsid, IStream *stream, BOOL multi_use, void **registration) DECLSPEC_HIDDEN;
void    RPC_StopLocalServer(void *registration) DECLSPEC_HIDDEN;
HRESULT RPC_GetLocalClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT RPC_RegisterChannelHook(REFGUID rguid, IChannelHook *hook) DECLSPEC_HIDDEN;
void    RPC_UnregisterAllChannelHooks(void) DECLSPEC_HIDDEN;
HRESULT RPC_ResolveOxid(OXID oxid, OXID_INFO *oxid_info) DECLSPEC_HIDDEN;

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void) DECLSPEC_HIDDEN;

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void) DECLSPEC_HIDDEN;

/* Drag and drop */
void OLEDD_UnInitialize(void) DECLSPEC_HIDDEN;

/* Apartment Functions */

APARTMENT *apartment_findfromoxid(OXID oxid, BOOL ref) DECLSPEC_HIDDEN;
APARTMENT *apartment_findfromtid(DWORD tid) DECLSPEC_HIDDEN;
DWORD apartment_release(struct apartment *apt) DECLSPEC_HIDDEN;
HRESULT apartment_disconnectproxies(struct apartment *apt) DECLSPEC_HIDDEN;
static inline HRESULT apartment_getoxid(const struct apartment *apt, OXID *oxid)
{
    *oxid = apt->oxid;
    return S_OK;
}
HRESULT apartment_createwindowifneeded(struct apartment *apt) DECLSPEC_HIDDEN;
HWND apartment_getwindow(const struct apartment *apt) DECLSPEC_HIDDEN;
void apartment_joinmta(void) DECLSPEC_HIDDEN;


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

extern HRESULT Handler_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
extern HRESULT HandlerCF_Create(REFCLSID rclsid, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

/* Exported non-interface Data Advise Holder functions */
HRESULT DataAdviseHolder_OnConnect(IDataAdviseHolder *iface, IDataObject *pDelegate) DECLSPEC_HIDDEN;
void DataAdviseHolder_OnDisconnect(IDataAdviseHolder *iface) DECLSPEC_HIDDEN;

extern UINT ownerlink_clipboard_format DECLSPEC_HIDDEN;
extern UINT filename_clipboard_format DECLSPEC_HIDDEN;
extern UINT filenameW_clipboard_format DECLSPEC_HIDDEN;
extern UINT dataobject_clipboard_format DECLSPEC_HIDDEN;
extern UINT embedded_object_clipboard_format DECLSPEC_HIDDEN;
extern UINT embed_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT custom_link_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT link_source_clipboard_format DECLSPEC_HIDDEN;
extern UINT object_descriptor_clipboard_format DECLSPEC_HIDDEN;
extern UINT link_source_descriptor_clipboard_format DECLSPEC_HIDDEN;
extern UINT ole_private_data_clipboard_format DECLSPEC_HIDDEN;

extern LSTATUS create_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *) DECLSPEC_HIDDEN;
extern LSTATUS open_classes_key(HKEY, const WCHAR *, REGSAM, HKEY *) DECLSPEC_HIDDEN;

extern BOOL actctx_get_miscstatus(const CLSID*, DWORD, DWORD*) DECLSPEC_HIDDEN;

extern const char *debugstr_formatetc(const FORMATETC *formatetc) DECLSPEC_HIDDEN;

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#endif /* __WINE_OLE_COMPOBJ_H */
