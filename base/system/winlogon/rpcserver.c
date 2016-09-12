/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/rpcserver.c
 * PURPOSE:         RPC server interface for the remote registry calls
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <rpc.h>
#include <winreg_s.h>


/* FUNCTIONS *****************************************************************/

BOOL
StartRpcServer(VOID)
{
    RPC_STATUS Status;

    TRACE("StartRpcServer() called\n");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\winreg",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return FALSE;
    }

    Status = RpcServerRegisterIf(winreg_v1_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return FALSE;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed (Status %lx)\n", Status);
        return FALSE;
    }

    TRACE("StartRpcServer() done\n");
    return TRUE;
}


void __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER RPC_HKEY_rundown(RPC_HKEY hSCObject)
{
}


/* Function 0 */
error_status_t
WINAPI
OpenClassesRoot(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 1 */
error_status_t
WINAPI
OpenCurrentUser(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 2 */
error_status_t
WINAPI
OpenLocalMachine(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 3 */
error_status_t
WINAPI
OpenPerformanceData(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 4 */
error_status_t
WINAPI
OpenUsers(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 5 */
error_status_t
WINAPI
BaseRegCloseKey(
    PRPC_HKEY hKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 6 */
error_status_t
WINAPI
BaseRegCreateKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey,
    PRPC_UNICODE_STRING lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    PRPC_SECURITY_ATTRIBUTES lpSecurityAttributes,
    PRPC_HKEY phkResult,
    LPDWORD lpdwDisposition)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 7 */
error_status_t
WINAPI
BaseRegDeleteKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 8 */
error_status_t
WINAPI
BaseRegDeleteValue(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpValueName)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 9 */
error_status_t
WINAPI
BaseRegEnumKey(
    RPC_HKEY hKey,
    DWORD dwIndex,
    PRPC_UNICODE_STRING lpNameIn,
    PRPC_UNICODE_STRING lpNameOut,
    PRPC_UNICODE_STRING lpClassIn,
    PRPC_UNICODE_STRING *lplpClassOut,
    PFILETIME lpftLastWriteTime)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 10 */
error_status_t
WINAPI
BaseRegEnumValue(
    RPC_HKEY hKey,
    DWORD dwIndex,
    PRPC_UNICODE_STRING lpValueNameIn,
    PRPC_UNICODE_STRING lpValueNameOut,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPDWORD lpcbLen)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 11 */
error_status_t
__stdcall
BaseRegFlushKey(
    RPC_HKEY hKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 12 */
error_status_t
__stdcall
BaseRegGetKeySecurity(
    RPC_HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptorIn,
    PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptorOut)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 13 */
error_status_t
__stdcall
BaseRegLoadKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey,
    PRPC_UNICODE_STRING lpFile)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 14 */
void
__stdcall
Opnum14NotImplemented(
    handle_t IDL_handle)
{
    TRACE("\n");
}


/* Function 15 */
error_status_t
__stdcall
BaseRegOpenKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PRPC_HKEY phkResult)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 16 */
error_status_t
__stdcall
BaseRegQueryInfoKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpClassIn,
    PRPC_UNICODE_STRING lpClassOut,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 17 */
error_status_t
__stdcall
BaseRegQueryValue(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpValueName,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPDWORD lpcbLen)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 18 */
error_status_t
__stdcall
BaseRegReplaceKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey,
    PRPC_UNICODE_STRING lpNewFile,
    PRPC_UNICODE_STRING lpOldFile)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 19 */
error_status_t
__stdcall
BaseRegRestoreKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpFile,
    DWORD Flags)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 20 */
error_status_t
__stdcall
BaseRegSaveKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpFile,
    PRPC_SECURITY_ATTRIBUTES pSecurityAttributes)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 21 */
error_status_t
__stdcall
BaseRegSetKeySecurity(
    RPC_HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 22 */
error_status_t
__stdcall
BaseRegSetValue(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpValueName,
    DWORD dwType,
    LPBYTE lpData,
    DWORD cbData)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 23 */
error_status_t
__stdcall
BaseRegUnLoadKey(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 24 */
ULONG
__stdcall
BaseInitiateSystemShutdown(
    PREGISTRY_SERVER_NAME ServerName,
    PRPC_UNICODE_STRING lpMessage,
    ULONG dwTimeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown)
{
    TRACE("BaseInitiateSystemShutdown()\n");
    return BaseInitiateSystemShutdownEx(ServerName,
                                        lpMessage,
                                        dwTimeout,
                                        bForceAppsClosed,
                                        bRebootAfterShutdown,
                                        0);
}


/* Function 25 */
ULONG
__stdcall
BaseAbortSystemShutdown(
    PREGISTRY_SERVER_NAME ServerName)
{
    TRACE("BaseAbortSystemShutdown()\n");
    return ERROR_SUCCESS;
}


/* Function 26 */
error_status_t
__stdcall
BaseRegGetVersion(
    RPC_HKEY hKey,
    LPDWORD lpdwVersion)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 27 */
error_status_t
__stdcall
OpenCurrentConfig(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 28 */
void
__stdcall
Opnum28NotImplemented(
    handle_t IDL_handle)
{
    TRACE("\n");
}


/* Function 29 */
error_status_t
__stdcall
BaseRegQueryMultipleValues(
    RPC_HKEY hKey,
    PRVALENT val_listIn,
    PRVALENT val_listOut,
    DWORD num_vals,
    char *lpvalueBuf,
    LPDWORD ldwTotsize)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 30 */
ULONG
__stdcall
BaseInitiateSystemShutdownEx(
    PREGISTRY_SERVER_NAME ServerName,
    PRPC_UNICODE_STRING lpMessage,
    ULONG dwTimeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown,
    ULONG dwReason)
{
    TRACE("BaseInitiateSystemShutdownEx()\n");
    TRACE("  Message: %wZ\n", lpMessage);
    TRACE("  Timeout: %lu\n", dwTimeout);
    TRACE("  Force: %d\n", bForceAppsClosed);
    TRACE("  Reboot: %d\n", bRebootAfterShutdown);
    TRACE("  Reason: %lu\n", dwReason);

//    return ERROR_SUCCESS;

    /* FIXME */
    return ExitWindowsEx((bRebootAfterShutdown ? EWX_REBOOT : EWX_SHUTDOWN) |
                         (bForceAppsClosed ? EWX_FORCE : 0),
                         dwReason);
}


/* Function 31 */
error_status_t
__stdcall
BaseRegSaveKeyEx(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpFile,
    PRPC_SECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD Flags)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 32 */
error_status_t
__stdcall
OpenPerformanceText(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 33 */
error_status_t
__stdcall
OpenPerformanceNlsText(
    PREGISTRY_SERVER_NAME ServerName,
    REGSAM samDesired,
    PRPC_HKEY phKey)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 34 */
error_status_t
__stdcall
BaseRegQueryMultipleValues2(
    RPC_HKEY hKey,
    PRVALENT val_listIn,
    PRVALENT val_listOut,
    DWORD num_vals,
    char *lpvalueBuf,
    LPDWORD ldwTotsize,
    LPDWORD ldwRequiredSize)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}


/* Function 35 */
error_status_t
__stdcall
BaseRegDeleteKeyEx(
    RPC_HKEY hKey,
    PRPC_UNICODE_STRING lpSubKey,
    REGSAM AccessMask,
    DWORD Reserved)
{
    TRACE("\n");
    return ERROR_SUCCESS;
}

/* EOF */
