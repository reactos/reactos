/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2.2 Library
 * FILE:        lib/ws2_32.h
 * PURPOSE:     WinSock 2.2 Main Header
 */

#ifndef __WS2_32P_H
#define __WS2_32P_H

#define WINSOCK_ROOT "System\\CurrentControlSet\\Services\\WinSock2\\Parameters"
#define MAXALIASES 35

typedef enum _WSASYNCOPS
{
    WsAsyncGetHostByAddr,
    WsAsyncGetHostByName,
    WsAsyncGetProtoByName,
    WsAsyncGetProtoByNumber,
    WsAsyncGetServByName,
    WsAsyncGetServByPort,
    WsAsyncTerminate,
} WSASYNCOPS;

typedef struct _WSASYNCBLOCK
{
    LIST_ENTRY AsyncQueue;
    HANDLE TaskHandle;
    WSASYNCOPS Operation;
    union
    {
        struct
        {
            HWND hWnd;
            UINT wMsg;
            PCHAR ByWhat;
            DWORD Length;
            DWORD Type;
            PVOID Buffer;
            DWORD BufferLength;
        } GetHost;
        struct
        {
            HWND hWnd;
            UINT wMsg;
            PCHAR ByWhat;
            DWORD Length;
            PVOID Buffer;
            DWORD BufferLength;
        } GetProto;
        struct
        {
            HWND hWnd;
            UINT wMsg;
            PCHAR ByWhat;
            DWORD Length;
            PCHAR Protocol;
            PVOID Buffer;
            DWORD BufferLength;
        } GetServ;
    };
} WSASYNCBLOCK, *PWSASYNCBLOCK;

typedef struct _WSASYNCCONTEXT
{
    LIST_ENTRY AsyncQueue;
    HANDLE AsyncEvent;
    LIST_ENTRY SocketList;
} WSASYNCCONTEXT, *PWSASYNCCONTEXT;

typedef struct _WSPROTO_BUFFER
{
    PROTOENT Protoent;
    PCHAR Aliases[MAXALIASES];
    CHAR LineBuffer[512];
} WSPROTO_BUFFER, *PWSPROTO_BUFFER;

typedef struct _TPROVIDER
{
    LONG RefCount;
    WSPPROC_TABLE Service;
    HINSTANCE DllHandle;
} TPROVIDER, *PTPROVIDER;

typedef struct _TCATALOG_ENTRY
{
    LIST_ENTRY CatalogLink;
    LONG RefCount;
    PTPROVIDER Provider;
    CHAR DllPath[MAX_PATH];
    WSAPROTOCOL_INFOW ProtocolInfo;
} TCATALOG_ENTRY, *PTCATALOG_ENTRY;

typedef struct _TCATALOG
{
    LIST_ENTRY ProtocolList;
    DWORD ItemCount;
    DWORD UniqueId;
    DWORD NextId;
    HKEY CatalogKey;
    RTL_CRITICAL_SECTION Lock;
    BOOLEAN Initialized;
} TCATALOG, *PTCATALOG;

typedef struct _NSPROVIDER
{
    LONG RefCount;
    DWORD NamespaceId;
    HINSTANCE DllHandle;
    GUID ProviderId;
    NSP_ROUTINE Service;
} NSPROVIDER, *PNS_PROVIDER;

typedef struct _NSQUERY_PROVIDER
{
    LIST_ENTRY QueryLink;
    PNS_PROVIDER Provider;
    HANDLE LookupHandle;
} NSQUERY_PROVIDER, *PNSQUERY_PROVIDER;

typedef struct _NSCATALOG_ENTRY
{
    LIST_ENTRY CatalogLink;
    LONG RefCount;
    PNS_PROVIDER Provider;
    LONG AddressFamily;
    DWORD NamespaceId;
    DWORD Version;
    LPWSTR ProviderName;
    BOOLEAN Enabled;
    BOOLEAN StoresServiceClassInfo;
    GUID ProviderId;
    WCHAR DllPath[MAX_PATH];
} NSCATALOG_ENTRY, *PNSCATALOG_ENTRY;

typedef struct _NSCATALOG
{
    LIST_ENTRY CatalogList;
    DWORD ItemCount;
    DWORD UniqueId;
    HKEY CatalogKey;
    RTL_CRITICAL_SECTION Lock;
} NSCATALOG, *PNSCATALOG;

typedef struct _NSQUERY
{
    DWORD Signature;
    LONG RefCount;
    BOOLEAN ShuttingDown;
    LIST_ENTRY ProviderList;
    PNSQUERY_PROVIDER ActiveProvider;
    RTL_CRITICAL_SECTION Lock;
    PNSQUERY_PROVIDER CurrentProvider;
    LPWSAQUERYSETW QuerySet;
    DWORD ControlFlags;
    PNSCATALOG Catalog;
    DWORD TryAgain;
} NSQUERY, *PNSQUERY;

typedef struct _WSPROCESS
{
    LONG RefCount;
    HANDLE ApcHelper;
    HANDLE HandleHelper;
    HANDLE NotificationHelper;
    PTCATALOG ProtocolCatalog;
    PNSCATALOG NamespaceCatalog;
    HANDLE ProtocolCatalogEvent;
    HANDLE NamespaceCatalogEvent;
    DWORD Version;
    BOOLEAN LockReady;
    RTL_CRITICAL_SECTION ThreadLock;
} WSPROCESS, *PWSPROCESS;

typedef struct _WSTHREAD
{
    PWSPROCESS Process;
    WSATHREADID WahThreadId;
    HANDLE AsyncHelper;
    LPWSPCANCELBLOCKINGCALL CancelBlockingCall;
    LPBLOCKINGCALLBACK BlockingCallback;
    FARPROC BlockingHook;
    BOOLEAN Blocking;
    BOOLEAN Cancelled;
    CHAR Buffer[32];
    PCHAR Hostent;
    PCHAR Servent;
    DWORD HostentSize;
    DWORD ServentSize;
    DWORD OpenType;
    PVOID ProtocolInfo;
} WSTHREAD, *PWSTHREAD;

typedef struct _WSSOCKET
{
   LONG RefCount;
   HANDLE Handle;
   PWSPROCESS Process;
   PTPROVIDER Provider;
   PTCATALOG_ENTRY CatalogEntry;
   BOOLEAN Overlapped;
   BOOLEAN ApiSocket;
   BOOLEAN IsProvider;
} WSSOCKET, *PWSSOCKET;

typedef struct _ENUM_CONTEXT
{
   LPWSAQUERYSETW lpqsRestrictions;
   INT ErrorCode;
   PNSQUERY NsQuery;
   PNSCATALOG Catalog;
} ENUM_CONTEXT, *PENUM_CONTEXT;

typedef struct _PROTOCOL_ENUM_CONTEXT
{
    LPINT Protocols;
    LPWSAPROTOCOL_INFOW ProtocolBuffer;
    DWORD BufferLength;
    DWORD BufferUsed;
    DWORD Count;
    INT ErrorCode;
} PROTOCOL_ENUM_CONTEXT, *PPROTOCOL_ENUM_CONTEXT;

typedef struct _WS_BUFFER
{
    ULONG_PTR Position;
    SIZE_T MaxSize;
    SIZE_T BytesUsed;
} WS_BUFFER, *PWS_BUFFER;

typedef BOOL
(WINAPI *PNSCATALOG_ENUMERATE_PROC)(
    IN PVOID Context,
    IN PNSCATALOG_ENTRY Entry
);

typedef BOOL
(WINAPI *PTCATALOG_ENUMERATE_PROC)(
    IN PVOID Context,
    IN PTCATALOG_ENTRY Entry
);

typedef BOOL
(WINAPI *PWS_SOCK_POST_ROUTINE)(
    IN HWND hWnd,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

extern HINSTANCE WsDllHandle;
extern HANDLE WsSockHeap;
extern PWAH_HANDLE_TABLE WsSockHandleTable;
extern DWORD TlsIndex;
extern PWSPROCESS CurrentWsProcess;
extern DWORD GlobalTlsIndex;
extern BOOLEAN WsAsyncThreadInitialized;
extern PWS_SOCK_POST_ROUTINE WsSockPostRoutine;

LPSTR
WSAAPI
AnsiDupFromUnicode(IN LPWSTR UnicodeString);

LPWSTR
WSAAPI
UnicodeDupFromAnsi(IN LPSTR AnsiString);

VOID
WSAAPI
WsRasInitializeAutodial(VOID);

VOID
WSAAPI
WsRasUninitializeAutodial(VOID);

BOOL
WSAAPI
WSAttemptAutodialName(IN CONST LPWSAQUERYSETW lpqsRestrictions);

BOOL
WSAAPI
WSAttemptAutodialAddr(
    IN CONST SOCKADDR FAR *Name,
    IN INT NameLength
);

VOID
WSAAPI
WSNoteSuccessfulHostentLookup(
    IN CONST CHAR FAR *Name,
    IN CONST ULONG Address
);

INT
WSAAPI
MapUnicodeProtocolInfoToAnsi(IN LPWSAPROTOCOL_INFOW UnicodeInfo,
                             OUT LPWSAPROTOCOL_INFOA AnsiInfo);

INT
WSAAPI
MapAnsiQuerySetToUnicode(IN LPWSAQUERYSETA AnsiSet,
                         IN OUT PSIZE_T SetSize,
                         OUT LPWSAQUERYSETW UnicodeSet);

INT
WSAAPI
MapUnicodeQuerySetToAnsi(OUT LPWSAQUERYSETW UnicodeSet,
                         IN OUT PSIZE_T SetSize,
                         IN LPWSAQUERYSETA AnsiSet);

INT
WSAAPI
CopyQuerySetW(IN LPWSAQUERYSETW UnicodeSet,
              OUT LPWSAQUERYSETW *UnicodeCopy);

INT
WSAAPI
WsSlowProlog(VOID);

INT
WSAAPI
WsSlowPrologTid(OUT LPWSATHREADID *ThreadId);

PWSSOCKET
WSAAPI
WsSockGetSocket(IN SOCKET Handle);

INT
WSAAPI
WsApiProlog(OUT PWSPROCESS *Process,
            OUT PWSTHREAD *Thread);

HKEY
WSAAPI
WsOpenRegistryRoot(VOID);

VOID
WSAAPI
WsCreateStartupSynchronization(VOID);

VOID
WSAAPI
WsDestroyStartupSynchronization(VOID);

INT
WSAAPI
WsSetupCatalogProtection(IN HKEY CatalogKey,
                         IN HANDLE CatalogEvent,
                         OUT LPDWORD UniqueId);

BOOL
WSAAPI
WsCheckCatalogState(IN HANDLE Event);

PNSCATALOG
WSAAPI
WsNcAllocate(VOID);

VOID
WSAAPI
WsNcDelete(IN PNSCATALOG Catalog);

INT
WSAAPI
WsNcInitializeFromRegistry(IN PNSCATALOG Catalog,
                           IN HKEY ParentKey,
                           IN HANDLE CatalogEvent);

INT
WSAAPI
WsNcRefreshFromRegistry(IN PNSCATALOG Catalog,
                        IN HANDLE CatalogEvent);

VOID
WSAAPI
WsNcUpdateNamespaceList(IN PNSCATALOG Catalog,
                        IN PLIST_ENTRY List);

BOOL
WSAAPI
WsNcMatchProtocols(IN DWORD NameSpace,
                   IN LONG AddressFamily,
                   IN LPWSAQUERYSETW QuerySet);

INT
WSAAPI
WsNcLoadProvider(IN PNSCATALOG Catalog,
                 IN PNSCATALOG_ENTRY CatalogEntry);

INT
WSAAPI
WsNcGetCatalogFromProviderId(IN PNSCATALOG Catalog,
                             IN LPGUID ProviderId,
                             OUT PNSCATALOG_ENTRY *CatalogEntry);

VOID
WSAAPI
WsNcEnumerateCatalogItems(IN PNSCATALOG Catalog,
                          IN PNSCATALOG_ENUMERATE_PROC Callback,
                          IN PVOID Context);

INT
WSAAPI
WsNcGetServiceClassInfo(IN PNSCATALOG Catalog,
                        IN OUT LPDWORD BugSize,
                        IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo);

PNSCATALOG_ENTRY
WSAAPI
WsNcEntryAllocate(VOID);

INT
WSAAPI
WsNcEntryInitializeFromRegistry(IN PNSCATALOG_ENTRY CatalogEntry,
                                IN HKEY ParentKey,
                                IN ULONG UniqueId);

VOID
WSAAPI
WsNcEntryDereference(IN PNSCATALOG_ENTRY CatalogEntry);

VOID
WSAAPI
WsNcEntrySetProvider(IN PNSCATALOG_ENTRY Entry,
                     IN PNS_PROVIDER Provider);

DWORD
WSAAPI
WsNqAddProvider(
    IN PNSQUERY NsQuery,
    IN PNS_PROVIDER Provider
);

PNSQUERY
WSAAPI
WsNqAllocate(VOID);

BOOL
WSAAPI
WsNqBeginEnumerationProc(
    PVOID Context,
    PNSCATALOG_ENTRY CatalogEntry
);

VOID
WSAAPI
WsNqDelete(IN PNSQUERY NsQuery);

DWORD
WSAAPI
WsNqInitialize(IN PNSQUERY NsQuery);

DWORD
WSAAPI
WsNqLookupServiceBegin(
    IN PNSQUERY NsQuery,
    IN LPWSAQUERYSETW QuerySet,
    IN DWORD ControlFlags,
    IN PNSCATALOG Catalog
);

DWORD
WSAAPI
WsNqLookupServiceEnd(IN PNSQUERY NsQuery);

DWORD
WSAAPI
WsNqLookupServiceNext(
    PNSQUERY NsQuery,
    DWORD,
    PDWORD,
    OUT LPWSAQUERYSETW QuerySet
);

PNSQUERY_PROVIDER
WSAAPI
WsNqNextProvider(
    PNSQUERY NsQuery,
    IN PNSQUERY_PROVIDER Provider
);

VOID
WSAAPI
WsNqDereference(IN PNSQUERY Query);

BOOL
WSAAPI
WsNqValidateAndReference(IN PNSQUERY Query);

PNSQUERY_PROVIDER
WSAAPI
WsNqPreviousProvider(IN PNSQUERY Query,
                     IN PNSQUERY_PROVIDER Provider);

DWORD
WSAAPI
WsNqProvLookupServiceNext(
    IN PNSQUERY_PROVIDER QueryProvider,
    DWORD,
    PDWORD ,
    LPWSAQUERYSETW QuerySet
);

DWORD
WSAAPI
WsNqProvLookupServiceEnd(IN PNSQUERY_PROVIDER QueryProvider);

DWORD
WSAAPI
WsNqProvInitialize(
    IN PNSQUERY_PROVIDER QueryProvider,
    IN PNS_PROVIDER Provider
);

PNSQUERY_PROVIDER
WSAAPI
WsNqProvAllocate(VOID);

VOID
WSAAPI
WsNqProvDelete(IN PNSQUERY_PROVIDER QueryProvider);

DWORD
WSAAPI
WsNqProvLookupServiceBegin(
    IN PNSQUERY_PROVIDER QueryProvider,
    IN LPWSAQUERYSETW QuerySet,
    IN LPWSASERVICECLASSINFOW ServiceClassInfo,
    IN DWORD 
);

VOID
WSAAPI
WsNpDelete(IN PNS_PROVIDER Provider);

DWORD
WSAAPI
WsNpLookupServiceBegin (
    IN PNS_PROVIDER Provider,
    IN LPWSAQUERYSETW Restrictions,
    struct _WSAServiceClassInfoW *,
    IN DWORD ControlFlags,
    OUT PHANDLE LookupHandle
);

DWORD
WSAAPI
WsNpNSPCleanup(IN PNS_PROVIDER Provider);

DWORD
WSAAPI
WsNpLookupServiceEnd(
    IN PNS_PROVIDER Provider,
    IN HANDLE LookupHandle
);

DWORD
WSAAPI
WsNpInitialize(
    IN PNS_PROVIDER Provider,
    IN LPWSTR DllPath,
    IN LPGUID ProviderGuid
);

PNS_PROVIDER
WSAAPI
WsNpAllocate(VOID);

VOID
WSAAPI
WsNpDereference(IN PNS_PROVIDER Provider);

DWORD
WSAAPI
WsNpLookupServiceNext(
    IN PNS_PROVIDER Provider,
    IN HANDLE LookupHandle,
    IN DWORD ControlFlags,
    OUT PDWORD BufferLength,
    OUT LPWSAQUERYSETW Results
);

VOID
WSAAPI
WsTpDelete(IN PTPROVIDER Provider);

DWORD
WSAAPI
WsTpWSPCleanup(
    IN PTPROVIDER Provider,
    int *
);

PTPROVIDER
WSAAPI
WsTpAllocate(VOID);

DWORD
WSAAPI
WsTpInitialize(
    IN PTPROVIDER Provider,
    IN LPSTR DllName,
    LPWSAPROTOCOL_INFOW ProtocolInfo
);

VOID
WSAAPI
WsTpDereference(IN PTPROVIDER Provider);

VOID
WSAAPI
WsThreadDelete(IN PWSTHREAD Thread);

VOID
WSAAPI
WsThreadDestroyCurrentThread(VOID);

DWORD
WSAAPI
WsThreadCreate(
    IN PWSPROCESS Process,
    IN PWSTHREAD *Thread
);

DWORD
WSAAPI
WsThreadGetCurrentThread(
    IN PWSPROCESS Process,
    IN PWSTHREAD *Thread
);

LPWSATHREADID
WSAAPI
WsThreadGetThreadId(IN PWSPROCESS Process);

DWORD
WSAAPI
WsThreadStartup(VOID);

VOID
WSAAPI
WsThreadCleanup(VOID);

DWORD
WSAAPI
WsThreadCancelBlockingCall(IN PWSTHREAD Thread);

DWORD
WSAAPI
WsThreadUnhookBlockingHook(IN PWSTHREAD Thread);

FARPROC
WSAAPI
WsThreadSetBlockingHook(IN PWSTHREAD Thread,
                        IN FARPROC BlockingHook);


PHOSTENT
WSAAPI
WsThreadBlobToHostent(IN PWSTHREAD Thread,
                      IN LPBLOB Blob);

PSERVENT
WSAAPI
WsThreadBlobToServent(IN PWSTHREAD Thread,
                      IN LPBLOB Blob);

PWSPROTO_BUFFER
WSAAPI
WsThreadGetProtoBuffer(IN PWSTHREAD Thread);

PWSTHREAD
WSAAPI
WsThreadAllocate(VOID);

DWORD
WSAAPI
WsThreadDefaultBlockingHook(VOID);

DWORD
WSAAPI
WsThreadInitialize(
    IN PWSTHREAD Thread,
    IN PWSPROCESS Process
);

DWORD
WSAAPI
WsTcFindIfsProviderForSocket(IN PTCATALOG TCatalog, SOCKET Socket);

DWORD
WSAAPI
WsTcEntryInitializeFromRegistry(IN PTCATALOG_ENTRY CatalogEntry, IN HKEY, unsigned long);

DWORD
WSAAPI
WsTcGetEntryFromAf(IN PTCATALOG TCatalog, IN INT AddressFamily, IN PTCATALOG_ENTRY *CatalogEntry);

PTCATALOG_ENTRY
WSAAPI
WsTcEntryAllocate(VOID);

VOID
WSAAPI
WsTcEntrySetProvider(IN PTCATALOG_ENTRY CatalogEntry, IN PTPROVIDER Provider);

DWORD
WSAAPI
WsTcRefreshFromRegistry(IN PTCATALOG TCatalog, PVOID);

BOOL
WSAAPI
WsTcOpen(IN PTCATALOG TCatalog, IN HKEY);

PTPROVIDER
WSAAPI
WsTcFindProvider(IN PTCATALOG TCatalog, IN LPGUID ProviderId);

VOID
WSAAPI
WsTcEnumerateCatalogItems(IN PTCATALOG Catalog,
                          IN PTCATALOG_ENUMERATE_PROC Callback,
                          IN PVOID Context);

VOID
WSAAPI
WsTcEntryDereference(IN PTCATALOG_ENTRY CatalogEntry);

PTCATALOG
WSAAPI
WsTcAllocate(VOID);

VOID
WSAAPI
WsTcDelete(IN PTCATALOG Catalog);

DWORD
WSAAPI
WsTcGetEntryFromTriplet(IN PTCATALOG TCatalog, IN INT AddressFamily, IN INT SocketType, IN INT Protocol, IN DWORD StartId, IN PTCATALOG_ENTRY *CatalogEntry);

VOID
WSAAPI
WsTcUpdateProtocolList(IN PTCATALOG TCatalog, PLIST_ENTRY ProtocolList);

VOID
WSAAPI
WsTcEntryDelete(IN PTCATALOG_ENTRY CatalogEntry);

DWORD
WSAAPI
WsTcGetEntryFromCatalogEntryId(IN PTCATALOG TCatalog, IN DWORD CatalogEntryId, IN PTCATALOG_ENTRY *CatalogEntry);

DWORD
WSAAPI
WsTcLoadProvider(IN PTCATALOG TCatalog, IN PTCATALOG_ENTRY CatalogEntry);

DWORD
WSAAPI
WsTcInitializeFromRegistry(IN PTCATALOG TCatalog, HKEY, PVOID);

INT
WSAAPI
WsSockStartup(VOID);

VOID
WSAAPI
WsSockCleanup(VOID);

BOOL
WSAAPI
WsSockDeleteSockets(IN LPVOID Context,
                    IN PWAH_HANDLE Handle);

VOID
WSAAPI
WsSockDereference(IN PWSSOCKET Socket);

PWSSOCKET
WSAAPI
WsSockAllocate(VOID);

INT
WSAAPI
WsSockInitialize(IN PWSSOCKET Socket,
                 IN PTCATALOG_ENTRY CatalogEntry);

INT
WSAAPI
WsSockAssociateHandle(IN PWSSOCKET Socket,
                      IN SOCKET Handle,
                      IN BOOLEAN IsProvider);

INT
WSAAPI
WsSockDisassociateHandle(IN PWSSOCKET Socket);

INT
WSAAPI
WsSockAddApiReference(IN SOCKET Handle);

PTCATALOG
WSAAPI
WsProcGetTCatalog(IN PWSPROCESS Process);

BOOL
WSAAPI
WsProcDetachSocket(IN PWSPROCESS Process,
                   IN PWAH_HANDLE Handle);

INT
WSAAPI
WsProcGetAsyncHelper(IN PWSPROCESS Process,
                     OUT PHANDLE Handle);

VOID
WSAAPI
WsProcDelete(IN PWSPROCESS Process);

INT
WSAAPI
WsProcStartup(VOID);

PNSCATALOG
WSAAPI
WsProcGetNsCatalog(IN PWSPROCESS Process);

VOID
WSAAPI
WsProcSetVersion(IN PWSPROCESS Process,
                 IN WORD VersionRequested);

VOID
WSAAPI
WsAsyncQueueRequest(IN PWSASYNCBLOCK AsyncBlock);

BOOL
WSAAPI
WsAsyncCheckAndInitThread(VOID);

INT
WSAAPI
WsAsyncCancelRequest(IN HANDLE TaskHandle);

PWSASYNCBLOCK
WSAAPI
WsAsyncAllocateBlock(IN SIZE_T ExtraLength);

VOID
WSAAPI
WsAsyncTerminateThread(VOID);

VOID
WSAAPI
WsAsyncGlobalTerminate(VOID);

VOID
WSAAPI
WsAsyncGlobalInitialize(VOID);

FORCEINLINE
PWSPROCESS
WsGetProcess()
{
    return CurrentWsProcess;
}

FORCEINLINE
DWORD
WsQuickProlog()
{
    /* Try to see if we're initialized. If not, do the full prolog */
    return WsGetProcess() ? ERROR_SUCCESS : WsSlowProlog();
}

FORCEINLINE
DWORD
WsQuickPrologTid(LPWSATHREADID *Tid)
{
    /* Try to see if we're initialized. If not, do the full prolog */
    if ((WsGetProcess()) && (*Tid = WsThreadGetThreadId(WsGetProcess())))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return WsSlowPrologTid(Tid);
    }
}

#endif

