/*
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

#include "winternl.h"
#include "wine/orpc.h"

#include "wine/list.h"

extern HINSTANCE hProxyDll;

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
    struct list loaded_dlls; /* list of dlls loaded by this apartment (CS cs) */
    DWORD host_apt_tid;      /* thread ID of apartment hosting objects of differing threading model (CS cs) */
    HWND host_apt_hwnd;      /* handle to apartment window of host apartment (CS cs) */
    struct local_server *local_server; /* A marshallable object exposing local servers (CS cs) */
    BOOL being_destroyed;    /* is currently being destroyed */

    /* FIXME: OIDs should be given out by RPCSS */
    OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */

    /* STA-only fields */
    HWND win;                /* message window (LOCK) */
    IMessageFilter *filter;  /* message filter (CS cs) */
    BOOL main;               /* is this a main-threaded-apartment? (RO) */

    /* MTA-only */
    struct list usage_cookies; /* Used for refcount control with CoIncrementMTAUsage()/CoDecrementMTAUsage(). */
};

HRESULT open_key_for_clsid(REFCLSID clsid, const WCHAR *keyname, REGSAM access, HKEY *subkey);
HRESULT open_appidkey_from_clsid(REFCLSID clsid, REGSAM access, HKEY *subkey);

/* DCOM messages used by the apartment window (not compatible with native) */
#define DM_EXECUTERPC   (WM_USER + 0) /* WPARAM = 0, LPARAM = (struct dispatch_params *) */
#define DM_HOSTOBJECT   (WM_USER + 1) /* WPARAM = 0, LPARAM = (struct host_object_params *) */

#define CHARS_IN_GUID 39

enum tlsdata_flags
{
    OLETLS_UUIDINITIALIZED = 0x2,
    OLETLS_DISABLE_OLE1DDE = 0x40,
    OLETLS_APARTMENTTHREADED = 0x80,
    OLETLS_MULTITHREADED = 0x100,
};

/* this is what is stored in TEB->ReservedForOle */
struct tlsdata
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;
    DWORD             thread_seqid;  /* returned with CoGetCurrentProcess */
    DWORD             flags;         /* tlsdata_flags (+0Ch on x86) */
    void             *unknown0;
    DWORD             inits;         /* number of times CoInitializeEx called */
    DWORD             ole_inits;     /* number of times OleInitialize called */
    GUID              causality_id;  /* unique identifier for each COM call */
    LONG              pending_call_count_client; /* number of client calls pending */
    LONG              pending_call_count_server; /* number of server calls pending */
    DWORD             unknown;
    IObjContext      *context_token; /* (+38h on x86) */
    IUnknown         *call_state;    /* current call context (+3Ch on x86) */
    DWORD             unknown2[46];
    IUnknown         *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
    IUnknown         *state;         /* see CoSetState */
    struct list       spies;         /* Spies installed with CoRegisterInitializeSpy */
    DWORD             spies_lock;
    DWORD             cancelcount;
    CO_MTA_USAGE_COOKIE implicit_mta_cookie; /* mta referenced by roapi from sta thread */
};

extern HRESULT WINAPI InternalTlsAllocData(struct tlsdata **data);
extern BOOL WINAPI InternalIsProcessInitialized(void);

static inline HRESULT com_get_tlsdata(struct tlsdata **data)
{
    *data = NtCurrentTeb()->ReservedForOle;
    return *data ? S_OK : InternalTlsAllocData(data);
}

static inline struct apartment* com_get_current_apt(void)
{
    struct tlsdata *tlsdata = NULL;
    com_get_tlsdata(&tlsdata);
    return tlsdata->apt;
}

HWND apartment_getwindow(const struct apartment *apt);
HRESULT apartment_createwindowifneeded(struct apartment *apt);
void apartment_freeunusedlibraries(struct apartment *apt, DWORD unload_delay);
void apartment_global_cleanup(void);
OXID apartment_getoxid(const struct apartment *apt);
HRESULT apartment_disconnectproxies(struct apartment *apt);

/* RpcSs interface */
HRESULT rpcss_get_next_seqid(DWORD *id);
HRESULT rpc_get_local_class_object(REFCLSID rclsid, REFIID riid, void **obj);
HRESULT rpc_register_local_server(REFCLSID clsid, IStream *stream, DWORD flags, unsigned int *cookie);
HRESULT rpc_revoke_local_server(unsigned int cookie);
HRESULT rpc_create_clientchannel(const OXID *oxid, const IPID *ipid, const OXID_INFO *oxid_info, const IID *iid,
        DWORD dest_context, void *dest_context_data, IRpcChannelBuffer **chan, struct apartment *apt);
HRESULT rpc_create_serverchannel(DWORD dest_context, void *dest_context_data, IRpcChannelBuffer **chan);
HRESULT rpc_register_interface(REFIID riid);
void rpc_unregister_interface(REFIID riid, BOOL wait);
HRESULT rpc_resolve_oxid(OXID oxid, OXID_INFO *oxid_info);
void rpc_start_remoting(struct apartment *apt);
HRESULT rpc_register_channel_hook(REFGUID rguid, IChannelHook *hook);
void rpc_unregister_channel_hooks(void);

struct dispatch_params;
void rpc_execute_call(struct dispatch_params *params);

enum class_reg_data_origin
{
    CLASS_REG_ACTCTX,
    CLASS_REG_REGISTRY,
};

struct class_reg_data
{
    enum class_reg_data_origin origin;
    union
    {
        struct
        {
            const WCHAR *module_name;
            DWORD threading_model;
            HANDLE hactctx;
        } actctx;
        HKEY hkey;
    } u;
};

HRESULT enter_apartment(struct tlsdata *data, DWORD model);
void leave_apartment(struct tlsdata *data);
void apartment_release(struct apartment *apt);
struct apartment * apartment_get_current_or_mta(void);
HRESULT apartment_increment_mta_usage(CO_MTA_USAGE_COOKIE *cookie);
void apartment_decrement_mta_usage(CO_MTA_USAGE_COOKIE cookie);
HRESULT ensure_mta(void);
struct apartment * apartment_get_mta(void);
HRESULT apartment_get_inproc_class_object(struct apartment *apt, const struct class_reg_data *regdata,
        REFCLSID rclsid, REFIID riid, DWORD class_context, void **ppv);
HRESULT apartment_get_local_server_stream(struct apartment *apt, IStream **ret);
IUnknown *com_get_registered_class_object(const struct apartment *apartment, REFCLSID rclsid,
        DWORD clscontext);
void apartment_revoke_all_classes(const struct apartment *apt);
struct apartment * apartment_findfromoxid(OXID oxid);
struct apartment * apartment_findfromtid(DWORD tid);

HRESULT marshal_object(struct apartment *apt, STDOBJREF *stdobjref, REFIID riid, IUnknown *object,
        DWORD dest_context, void *dest_context_data, MSHLFLAGS mshlflags);

/* Stub Manager */

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
    struct apartment *apt;        /* owning apt (RO) */

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

ULONG stub_manager_int_release(struct stub_manager *stub_manager);
struct stub_manager * get_stub_manager_from_object(struct apartment *apt, IUnknown *object, BOOL alloc);
void stub_manager_disconnect(struct stub_manager *m);
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs, BOOL tableweak);
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs, BOOL tableweak, BOOL last_unlock_releases);
struct stub_manager * get_stub_manager(struct apartment *apt, OID oid);
void stub_manager_release_marshal_data(struct stub_manager *m, ULONG refs, const IPID *ipid, BOOL tableweak);
BOOL stub_manager_is_table_marshaled(struct stub_manager *m, const IPID *ipid);
BOOL stub_manager_notify_unmarshal(struct stub_manager *m, const IPID *ipid);
struct ifstub * stub_manager_find_ifstub(struct stub_manager *m, REFIID iid, MSHLFLAGS flags);
struct ifstub * stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, REFIID iid, DWORD dest_context,
    void *dest_context_data, MSHLFLAGS flags);
HRESULT ipid_get_dispatch_params(const IPID *ipid, struct apartment **stub_apt,
        struct stub_manager **manager, IRpcStubBuffer **stub, IRpcChannelBuffer **chan,
        IID *iid, IUnknown **iface);
HRESULT ipid_get_dest_context(const IPID *ipid, MSHCTX *dest_context, void **dest_context_data);
HRESULT start_apartment_remote_unknown(struct apartment *apt);
