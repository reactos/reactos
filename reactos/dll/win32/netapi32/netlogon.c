/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/netlogon.c
 * PURPOSE:         Netlogon service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include <rpc.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include "netlogon_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
LOGONSRV_HANDLE_bind(LOGONSRV_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("LOGONSRV_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\netlogon",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
LOGONSRV_HANDLE_unbind(LOGONSRV_HANDLE pszSystemName,
                       handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("LOGONSRV_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


DWORD
WINAPI
DsEnumerateDomainTrustsA(
    _In_opt_ LPSTR ServerName,
    _In_ ULONG Flags,
    _Out_ PDS_DOMAIN_TRUSTSA *Domains,
    _Out_ PULONG DomainCount)
{
    FIXME("DsEnumerateDomainTrustsA(%s, %x, %p, %p)\n",
          debugstr_a(ServerName), Flags, Domains, DomainCount);

    return ERROR_NO_LOGON_SERVERS;
}


DWORD
WINAPI
DsEnumerateDomainTrustsW(
    _In_opt_ LPWSTR ServerName,
    _In_ ULONG Flags,
    _Out_ PDS_DOMAIN_TRUSTSW *Domains,
    _Out_ PULONG DomainCount)
{
    FIXME("DsEnumerateDomainTrustsW(%s, %x, %p, %p)\n",
          debugstr_w(ServerName), Flags, Domains, DomainCount);

    return ERROR_NO_LOGON_SERVERS;
}


VOID
WINAPI
DsRoleFreeMemory(
    _In_ PVOID Buffer)
{
    TRACE("DsRoleFreeMemory(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}


DWORD
WINAPI
DsGetDcNameA(
    _In_ LPCSTR ComputerName,
    _In_ LPCSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("DsGetDcNameA(%s, %s, %s, %s, %08x, %p): stub\n",
          debugstr_a(ComputerName), debugstr_a(DomainName), debugstr_guid(DomainGuid),
          debugstr_a(SiteName), Flags, DomainControllerInfo);

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetDcNameW(
    _In_ LPCWSTR ComputerName,
    _In_ LPCWSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCWSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    FIXME("DsGetDcNameW(%s, %s, %s, %s, %08x, %p)\n",
          debugstr_w(ComputerName), debugstr_w(DomainName), debugstr_guid(DomainGuid),
          debugstr_w(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetSiteNameA(
    _In_ LPCSTR ComputerName,
    _Out_ LPSTR *SiteName)
{
    FIXME("DsGetSiteNameA(%s, %p)\n",
          debugstr_a(ComputerName), SiteName);

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetSiteNameW(
    _In_ LPCWSTR ComputerName,
    _Out_ LPWSTR *SiteName)
{
    NET_API_STATUS status;

    TRACE("DsGetSiteNameW(%s, %p)\n",
          debugstr_w(ComputerName), SiteName);

    RpcTryExcept
    {
        status = DsrGetSiteName((PWSTR)ComputerName,
                                SiteName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NTSTATUS
WINAPI
NetEnumerateTrustedDomains(
    _In_ LPWSTR ServerName,
    _Out_ LPWSTR *DomainNames)
{
    DOMAIN_NAME_BUFFER DomainNameBuffer = {0, NULL};
    NTSTATUS Status = 0;

    TRACE("NetEnumerateTrustedDomains(%s, %p)\n",
          debugstr_w(ServerName), DomainNames);

    RpcTryExcept
    {
        Status = NetrEnumerateTrustedDomains(ServerName,
                                             &DomainNameBuffer);
        if (NT_SUCCESS(Status))
        {
            *DomainNames = (LPWSTR)DomainNameBuffer.DomainNames;
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    } RpcEndExcept;

    return Status;
}


NET_API_STATUS
WINAPI
NetGetAnyDCName(
    _In_opt_ LPCWSTR ServerName,
    _In_opt_ LPCWSTR DomainName,
    _Out_ LPBYTE *BufPtr)
{
    NET_API_STATUS status;

    TRACE("NetGetAnyDCName(%s, %s, %p)\n",
          debugstr_w(ServerName), debugstr_w(DomainName), BufPtr);

    *BufPtr = NULL;

    RpcTryExcept
    {
        status = NetrGetAnyDCName((PWSTR)ServerName,
                                  (PWSTR)DomainName,
                                  (PWSTR*)BufPtr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetGetDCName(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR domainname,
    _Out_ LPBYTE *bufptr)
{
    FIXME("NetGetDCName(%s, %s, %p)\n",
          debugstr_w(servername), debugstr_w(domainname), bufptr);

    return NERR_DCNotFound;
}

/* EOF */
