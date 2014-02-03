/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/svchost.h
 * PURPOSE:     Precompiled Header for Service Host
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

#ifndef _SVCHOST_PCH_
#define _SVCHOST_PCH_

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN

#include <rpc.h>
#include <ndk/rtlfuncs.h>
#include <ndk/kdtypes.h>

//
// FIXME: Should go in public headers
//
#define DPFLTR_SVCHOST_ID 28

//
// This prints out a SVCHOST-specific debug print, with the PID/TID
//
#define SvchostDbgPrint(lev, fmt, ...)              \
    DbgPrintEx(DPFLTR_SVCHOST_ID,                   \
               DPFLTR_MASK | lev,                   \
               "[SVCHOST] %lx.%lx: " fmt,           \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               __VA_ARGS__);

#define DBG_ERR(fmt, ...)   SvchostDbgPrint(1, fmt, __VA_ARGS__)
#define DBG_TRACE(fmt, ...) SvchostDbgPrint(4, fmt, __VA_ARGS__)

//
// This is the callback that a hosted service can register for stop notification
// FIXME: GLOBAL HEADER
//
typedef VOID
    (CALLBACK *PSVCHOST_STOP_CALLBACK) (
    _In_ PVOID lpParameter,
    _In_ BOOLEAN TimerOrWaitFired
    );

//
// Hosted Services and SvcHost Use this Structure
// FIXME: GLOBAL HEADER
//
typedef struct _SVCHOST_GLOBALS
{
    PVOID NullSid;
    PVOID WorldSid;
    PVOID LocalSid;
    PVOID NetworkSid;
    PVOID LocalSystemSid;
    PVOID LocalServiceSid;
    PVOID NetworkServiceSid;
    PVOID BuiltinDomainSid;
    PVOID AuthenticatedUserSid;
    PVOID AnonymousLogonSid;
    PVOID AliasAdminsSid;
    PVOID AliasUsersSid;
    PVOID AliasGuestsSid;
    PVOID AliasPowerUsersSid;
    PVOID AliasAccountOpsSid;
    PVOID AliasSystemOpsSid;
    PVOID AliasPrintOpsSid;
    PVOID AliasBackupOpsSid;
    PVOID RpcpStartRpcServer;
    PVOID RpcpStopRpcServer;
    PVOID RpcpStopRpcServerEx;
    PVOID SvcNetBiosOpen;
    PVOID SvcNetBiosClose;
    PVOID SvcNetBiosReset;
    PVOID SvcRegisterStopCallback;
} SVCHOST_GLOBALS, *PSVCHOST_GLOBALS;

//
// This is the callback for them to receive it
//
typedef VOID
(WINAPI *PSVCHOST_INIT_GLOBALS) (
    _In_ PSVCHOST_GLOBALS Globals
);
//
// Initialization Stages
//
#define SVCHOST_RPC_INIT_COMPLETE   1
#define SVCHOST_NBT_INIT_COMPLETE   2
#define SVCHOST_SID_INIT_COMPLETE   4

//
// Domain Alias SID Structure
//
typedef struct _DOMAIN_SID_DATA
{
    PSID Sid;
    DWORD SubAuthority;
} DOMAIN_SID_DATA;

//
// Well-known SID Structure
//
typedef struct _SID_DATA
{
    PSID Sid;
    SID_IDENTIFIER_AUTHORITY Authority;
    DWORD SubAuthority;
} SID_DATA;

//
// This contains all the settings (from the registry) for a hosted service
//
typedef struct _SVCHOST_OPTIONS
{
    PWCHAR CmdLineBuffer;
    PWCHAR CmdLine;
    BOOL HasServiceGroup;
    LPWSTR ServiceGroupName;
    DWORD CoInitializeSecurityParam;
    DWORD AuthenticationLevel;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    DWORD AuthenticationCapabilities;
    DWORD DefaultRpcStackSize;
    BOOLEAN SystemCritical;
} SVCHOST_OPTIONS, *PSVCHOST_OPTIONS;

//
// This represents the DLL used to hold a hosted service
//
typedef struct _SVCHOST_DLL
{
    LIST_ENTRY DllList;
    HINSTANCE hModule;
    LPCWSTR pszDllPath;
    LPCWSTR pszManifestPath;
    HANDLE hActCtx;
    struct _SVCHOST_SERVICE* pService;
} SVCHOST_DLL, *PSVCHOST_DLL;

//
// This represents an actual hosted service
//
typedef struct _SVCHOST_SERVICE
{
    LPCWSTR pszServiceName;
    LPCSTR pszServiceMain;
    LONG cServiceActiveThreadCount;
    PSVCHOST_DLL pDll;
    PSVCHOST_STOP_CALLBACK pfnStopCallback;
} SVCHOST_SERVICE, *PSVCHOST_SERVICE;

//
// This is the context that a hosted service with a stop callback has
//
typedef struct _SVCHOST_CALLBACK_CONTEXT
{
    PVOID pContext;
    PSVCHOST_SERVICE pService;
} SVCHOST_CALLBACK_CONTEXT, *PSVCHOST_CALLBACK_CONTEXT;

NTSTATUS
NTAPI
RpcpInitRpcServer (
    VOID
);

NTSTATUS
NTAPI
RpcpStopRpcServer (
    _In_ RPC_IF_HANDLE IfSpec
);

NTSTATUS
NTAPI
RpcpStopRpcServerEx (
    _In_ RPC_IF_HANDLE IfSpec
);

NTSTATUS
NTAPI
RpcpStartRpcServer (
    _In_ LPCWSTR IfName,
    _In_ RPC_IF_HANDLE IfSpec
);

VOID
WINAPI
SvchostBuildSharedGlobals (
    VOID
);

VOID
WINAPI
SvchostCharLowerW (
    _In_ LPCWSTR lpSrcStr
);

NTSTATUS
NTAPI
ScCreateWellKnownSids (
    VOID
);

VOID
WINAPI
MemInit (
    _In_ HANDLE Heap
);

BOOL
WINAPI
MemFree (
    _In_ LPVOID lpMem
);

PVOID
WINAPI
MemAlloc (
    _In_ DWORD dwFlags,
    _In_ DWORD dwBytes
);

VOID
WINAPI
SvcNetBiosInit (
    VOID
);

VOID
WINAPI
SvcNetBiosClose (
VOID
);

VOID
WINAPI
SvcNetBiosOpen (
    VOID
);

DWORD
WINAPI
SvcNetBiosReset (
    _In_ UCHAR LanaNum
);

BOOL
WINAPI
InitializeSecurity (
    _In_ DWORD dwParam,
    _In_ DWORD dwAuthnLevel,
    _In_ DWORD dwImpLevel,
    _In_ DWORD dwCapabilities
);


DWORD
WINAPI
RegQueryDword (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _Out_ PDWORD pdwValue
);

DWORD
WINAPI
RegQueryString (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _In_ DWORD dwExpectedType,
    _Out_ PBYTE* ppbData
);

DWORD
WINAPI
RegQueryStringA (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _In_ DWORD dwExpectedType,
    _Out_ LPCSTR* ppszData
);

DWORD
WINAPI
SvcRegisterStopCallback (
    _In_ PHANDLE phNewWaitObject,
    _In_ LPCWSTR ServiceName,
    _In_ HANDLE hObject,
    _In_ PSVCHOST_STOP_CALLBACK pfnStopCallback,
    _In_ PVOID pContext,
    _In_ ULONG dwFlags
);

extern PSVCHOST_GLOBALS g_pSvchostSharedGlobals;

#endif /* _SVCHOST_PCH_ */
