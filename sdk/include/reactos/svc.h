/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Global Header for Service Host
 * PROGRAMMERS: ReactOS Portable Systems Group
 *
 * REFERENCES:
 * https://www.geoffchappell.com/studies/windows/win32/services/svchost/process/globaldata.htm
 */

#ifndef __SVC_H
#define __SVC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __RPC_H__ // For RPC_IF_HANDLE
/* Don't include windows.h if we just need rpc.h */
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif
#include <rpc.h>
#endif // __RPC_H__

#ifndef WINAPI
#define WINAPI __stdcall
#endif

/* Ensure NTSTATUS is defined */
#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS, *PNTSTATUS;
#endif


/*
 * Entrypoints for starting and stopping an RPC server.
 */
typedef NTSTATUS
(WINAPI *PSTART_RPC_SERVER)(
    _In_ PCWSTR IfName,
    _In_ RPC_IF_HANDLE IfSpec);

typedef NTSTATUS
(WINAPI *PSTOP_RPC_SERVER)(
    _In_ RPC_IF_HANDLE IfSpec);

typedef NTSTATUS
(WINAPI *PSTOP_RPC_SERVER_EX)(
    _In_ RPC_IF_HANDLE IfSpec);

/*
 * Entrypoints for NetBIOS service support.
 */
typedef VOID
(WINAPI *PNET_BIOS_OPEN)(VOID);

typedef VOID
(WINAPI *PNET_BIOS_CLOSE)(VOID);

typedef DWORD
(WINAPI *PNET_BIOS_RESET)(
    _In_ UCHAR LanaNum);

/*
 * Callback that a hosted service can register for stop notification.
 * Alias to (RTL_)WAITORTIMERCALLBACK(FUNC).
 */
/*
typedef VOID
(CALLBACK *PSVCHOST_STOP_CALLBACK)(
    _In_ PVOID lpParameter,
    _In_ BOOLEAN TimerOrWaitFired);
*/
typedef WAITORTIMERCALLBACK PSVCHOST_STOP_CALLBACK;

#if (_WIN32_WINNT == _WIN32_WINNT_WINXP && NTDDI_VERSION >= NTDDI_WINXPSP2) || \
    (NTDDI_VERSION >= NTDDI_WS03SP1)
typedef DWORD
(WINAPI *PREGISTER_STOP_CALLBACK)(
    _Out_ PHANDLE phNewWaitObject,
    _In_ PCWSTR pszServiceName,
    _In_ HANDLE hObject,
    _In_ PSVCHOST_STOP_CALLBACK Callback,
    _In_ PVOID Context,
    _In_ ULONG dwFlags);
#endif

/*
 * Hosted Services and SvcHost use this shared global data structure.
 */
typedef struct _SVCHOST_GLOBAL_DATA
{
    PSID NullSid;
    PSID WorldSid;
    PSID LocalSid;
    PSID NetworkSid;
    PSID LocalSystemSid;
    PSID LocalServiceSid;
    PSID NetworkServiceSid;
    PSID BuiltinDomainSid;
    PSID AuthenticatedUserSid;
    PSID AnonymousLogonSid;
    PSID AliasAdminsSid;
    PSID AliasUsersSid;
    PSID AliasGuestsSid;
    PSID AliasPowerUsersSid;
    PSID AliasAccountOpsSid;
    PSID AliasSystemOpsSid;
    PSID AliasPrintOpsSid;
    PSID AliasBackupOpsSid;

    /* SvcHost callbacks for RPC server and NetBIOS service support */
    PSTART_RPC_SERVER   StartRpcServer;
    PSTOP_RPC_SERVER    StopRpcServer;
    PSTOP_RPC_SERVER_EX StopRpcServerEx;
    PNET_BIOS_OPEN      NetBiosOpen;
    PNET_BIOS_CLOSE     NetBiosClose;
    PNET_BIOS_RESET     NetBiosReset;

#if (_WIN32_WINNT == _WIN32_WINNT_WINXP && NTDDI_VERSION >= NTDDI_WINXPSP2) || \
    (NTDDI_VERSION >= NTDDI_WS03SP1)
    PREGISTER_STOP_CALLBACK RegisterStopCallback;
#endif
} SVCHOST_GLOBAL_DATA, *PSVCHOST_GLOBAL_DATA;

#ifdef __cplusplus
}
#endif

#endif /* __SVC_H */
