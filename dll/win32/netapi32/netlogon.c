/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/netlogon.c
 * PURPOSE:         Netlogon service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include <winsock2.h>
#include <rpc.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include "netlogon_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

DWORD
WINAPI
DsGetDcNameWithAccountA(
    _In_opt_ LPCSTR ComputerName,
    _In_opt_ LPCSTR AccountName,
    _In_ ULONG AccountControlBits,
    _In_ LPCSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo);

DWORD
WINAPI
DsGetDcNameWithAccountW(
    _In_opt_ LPCWSTR ComputerName,
    _In_opt_ LPCWSTR AccountName,
    _In_ ULONG AccountControlBits,
    _In_ LPCWSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCWSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);

/* FUNCTIONS *****************************************************************/

handle_t
__RPC_USER
LOGONSRV_HANDLE_bind(
    LOGONSRV_HANDLE pszSystemName)
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


void
__RPC_USER
LOGONSRV_HANDLE_unbind(
    LOGONSRV_HANDLE pszSystemName,
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


/* PUBLIC FUNCTIONS **********************************************************/

DWORD
WINAPI
DsAddressToSiteNamesA(
    _In_opt_ LPCSTR ComputerName,
    _In_ DWORD EntryCount,
    _In_ PSOCKET_ADDRESS SocketAddresses,
    _Out_ LPSTR **SiteNames)
{
    FIXME("DsAddressToSiteNamesA(%s, %lu, %p, %p)\n",
          debugstr_a(ComputerName), EntryCount, SocketAddresses, SiteNames);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsAddressToSiteNamesW(
    _In_opt_ LPCWSTR ComputerName,
    _In_ DWORD EntryCount,
    _In_ PSOCKET_ADDRESS SocketAddresses,
    _Out_ LPWSTR **SiteNames)
{
    PNL_SITE_NAME_ARRAY SiteNameArray = NULL;
    PWSTR *SiteNamesBuffer = NULL, Ptr;
    ULONG BufferSize, i;
    NET_API_STATUS status;

    TRACE("DsAddressToSiteNamesW(%s, %lu, %p, %p)\n",
          debugstr_w(ComputerName), EntryCount, SocketAddresses, SiteNames);

    if (EntryCount == 0)
        return ERROR_INVALID_PARAMETER;

    *SiteNames = NULL;

    RpcTryExcept
    {
        status = DsrAddressToSiteNamesW((PWSTR)ComputerName,
                                        EntryCount,
                                        (PNL_SOCKET_ADDRESS)SocketAddresses,
                                        &SiteNameArray);
        if (status == NERR_Success)
        {
            if (SiteNameArray->EntryCount == 0)
            {
                status = ERROR_INVALID_PARAMETER;
            }
            else
            {
                BufferSize = SiteNameArray->EntryCount * sizeof(PWSTR);
                for (i = 0; i < SiteNameArray->EntryCount; i++)
                    BufferSize += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);

                status = NetApiBufferAllocate(BufferSize, (PVOID*)&SiteNamesBuffer);
                if (status == NERR_Success)
                {
                    ZeroMemory(SiteNamesBuffer, BufferSize);

                    Ptr = (PWSTR)((ULONG_PTR)SiteNamesBuffer + SiteNameArray->EntryCount * sizeof(PWSTR));
                    for (i = 0; i < SiteNameArray->EntryCount; i++)
                    {
                        SiteNamesBuffer[i] = Ptr;
                        CopyMemory(Ptr,
                                   SiteNameArray->SiteNames[i].Buffer,
                                   SiteNameArray->SiteNames[i].Length);

                        Ptr = (PWSTR)((ULONG_PTR)Ptr + SiteNameArray->SiteNames[i].Length + sizeof(WCHAR));
                    }

                    *SiteNames = SiteNamesBuffer;
                }
            }

            MIDL_user_free(SiteNameArray);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


DWORD
WINAPI
DsAddressToSiteNamesExA(
    _In_opt_ LPCSTR ComputerName,
    _In_ DWORD EntryCount,
    _In_ PSOCKET_ADDRESS SocketAddresses,
    _Out_ LPSTR **SiteNames,
    _Out_ LPSTR **SubnetNames)
{
    FIXME("DsAddressToSiteNamesExA(%s, %lu, %p, %p, %p)\n",
          debugstr_a(ComputerName), EntryCount, SocketAddresses,
          SiteNames, SubnetNames);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsAddressToSiteNamesExW(
    _In_opt_ LPCWSTR ComputerName,
    _In_ DWORD EntryCount,
    _In_ PSOCKET_ADDRESS SocketAddresses,
    _Out_ LPWSTR **SiteNames,
    _Out_ LPWSTR **SubnetNames)
{
    PNL_SITE_NAME_EX_ARRAY SiteNameArray = NULL;
    PWSTR *SiteNamesBuffer = NULL, *SubnetNamesBuffer = NULL, Ptr;
    ULONG SiteNameBufferSize, SubnetNameBufferSize, i;
    NET_API_STATUS status;

    TRACE("DsAddressToSiteNamesExW(%s, %lu, %p, %p, %p)\n",
          debugstr_w(ComputerName), EntryCount, SocketAddresses,
          SiteNames, SubnetNames);

    if (EntryCount == 0)
        return ERROR_INVALID_PARAMETER;

    *SiteNames = NULL;
    *SubnetNames = NULL;

    RpcTryExcept
    {
        status = DsrAddressToSiteNamesExW((PWSTR)ComputerName,
                                          EntryCount,
                                          (PNL_SOCKET_ADDRESS)SocketAddresses,
                                          &SiteNameArray);
        if (status == NERR_Success)
        {
            if (SiteNameArray->EntryCount == 0)
            {
                status = ERROR_INVALID_PARAMETER;
            }
            else
            {
                SiteNameBufferSize = SiteNameArray->EntryCount * sizeof(PWSTR);
                SubnetNameBufferSize = SiteNameArray->EntryCount * sizeof(PWSTR);
                for (i = 0; i < SiteNameArray->EntryCount; i++)
                {
                    SiteNameBufferSize += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);
                    SubnetNameBufferSize += SiteNameArray->SubnetNames[i].Length + sizeof(WCHAR);
                }

                status = NetApiBufferAllocate(SiteNameBufferSize, (PVOID*)&SiteNamesBuffer);
                if (status == NERR_Success)
                {
                    ZeroMemory(SiteNamesBuffer, SiteNameBufferSize);

                    Ptr = (PWSTR)((ULONG_PTR)SiteNamesBuffer + SiteNameArray->EntryCount * sizeof(PWSTR));
                    for (i = 0; i < SiteNameArray->EntryCount; i++)
                    {
                        SiteNamesBuffer[i] = Ptr;
                        CopyMemory(Ptr,
                                   SiteNameArray->SiteNames[i].Buffer,
                                   SiteNameArray->SiteNames[i].Length);

                        Ptr = (PWSTR)((ULONG_PTR)Ptr + SiteNameArray->SiteNames[i].Length + sizeof(WCHAR));
                    }

                    *SiteNames = SiteNamesBuffer;
                }

                status = NetApiBufferAllocate(SubnetNameBufferSize, (PVOID*)&SubnetNamesBuffer);
                if (status == NERR_Success)
                {
                    ZeroMemory(SubnetNamesBuffer, SubnetNameBufferSize);

                    Ptr = (PWSTR)((ULONG_PTR)SubnetNamesBuffer + SiteNameArray->EntryCount * sizeof(PWSTR));
                    for (i = 0; i < SiteNameArray->EntryCount; i++)
                    {
                        SubnetNamesBuffer[i] = Ptr;
                        CopyMemory(Ptr,
                                   SiteNameArray->SubnetNames[i].Buffer,
                                   SiteNameArray->SubnetNames[i].Length);

                        Ptr = (PWSTR)((ULONG_PTR)Ptr + SiteNameArray->SubnetNames[i].Length + sizeof(WCHAR));
                    }

                    *SubnetNames = SubnetNamesBuffer;
                }
            }

            MIDL_user_free(SiteNameArray);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


DWORD
WINAPI
DsDeregisterDnsHostRecordsA(
    _In_opt_ LPSTR ServerName,
    _In_opt_ LPSTR DnsDomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ GUID *DsaGuid,
    _In_ LPSTR DnsHostName)
{
    FIXME("DsDeregisterDnsHostRecordsA(%s, %s, %p, %p, %s)\n",
          debugstr_a(ServerName), debugstr_a(DnsDomainName),
          DomainGuid, DsaGuid, debugstr_a(DnsHostName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsDeregisterDnsHostRecordsW(
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR DnsDomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ GUID *DsaGuid,
    _In_ LPWSTR DnsHostName)
{
    NET_API_STATUS status;

    TRACE("DsDeregisterDnsHostRecordsW(%s, %s, %p, %p, %s)\n",
          debugstr_w(ServerName), debugstr_w(DnsDomainName),
          DomainGuid, DsaGuid, debugstr_w(DnsHostName));

    RpcTryExcept
    {
        status = DsrDeregisterDnsHostRecords(ServerName,
                                             DnsDomainName,
                                             DomainGuid,
                                             DsaGuid,
                                             DnsHostName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
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
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetDcNameA(
    _In_opt_ LPCSTR ComputerName,
    _In_ LPCSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    TRACE("DsGetDcNameA(%s, %s, %s, %s, %08lx, %p): stub\n",
          debugstr_a(ComputerName), debugstr_a(DomainName), debugstr_guid(DomainGuid),
          debugstr_a(SiteName), Flags, DomainControllerInfo);
    return DsGetDcNameWithAccountA(ComputerName,
                                   NULL,
                                   0,
                                   DomainName,
                                   DomainGuid,
                                   SiteName,
                                   Flags,
                                   DomainControllerInfo);
}


DWORD
WINAPI
DsGetDcNameW(
    _In_opt_ LPCWSTR ComputerName,
    _In_ LPCWSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCWSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    TRACE("DsGetDcNameW(%s, %s, %s, %s, %08lx, %p)\n",
          debugstr_w(ComputerName), debugstr_w(DomainName), debugstr_guid(DomainGuid),
          debugstr_w(SiteName), Flags, DomainControllerInfo);
    return DsGetDcNameWithAccountW(ComputerName,
                                   NULL,
                                   0,
                                   DomainName,
                                   DomainGuid,
                                   SiteName,
                                   Flags,
                                   DomainControllerInfo);
}


DWORD
WINAPI
DsGetDcNameWithAccountA(
    _In_opt_ LPCSTR ComputerName,
    _In_opt_ LPCSTR AccountName,
    _In_ ULONG AccountControlBits,
    _In_ LPCSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("DsGetDcNameWithAccountA(%s, %s, %08lx, %s, %s, %s, %08lx, %p): stub\n",
          debugstr_a(ComputerName), debugstr_a(AccountName), AccountControlBits,
          debugstr_a(DomainName), debugstr_guid(DomainGuid),
          debugstr_a(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetDcNameWithAccountW(
    _In_opt_ LPCWSTR ComputerName,
    _In_opt_ LPCWSTR AccountName,
    _In_ ULONG AccountControlBits,
    _In_ LPCWSTR DomainName,
    _In_ GUID *DomainGuid,
    _In_ LPCWSTR SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    NET_API_STATUS status;

    FIXME("DsGetDcNameWithAccountW(%s, %s, %08lx, %s, %s, %s, %08lx, %p): stub\n",
          debugstr_w(ComputerName), debugstr_w(AccountName), AccountControlBits,
          debugstr_w(DomainName), debugstr_guid(DomainGuid),
          debugstr_w(SiteName), Flags, DomainControllerInfo);

    RpcTryExcept
    {
        status = DsrGetDcNameEx2((PWSTR)ComputerName,
                                 (PWSTR)AccountName,
                                 AccountControlBits,
                                 (PWSTR)DomainName,
                                 DomainGuid,
                                 (PWSTR)SiteName,
                                 Flags,
                                 DomainControllerInfo);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


DWORD
WINAPI
DsGetDcSiteCoverageA(
    _In_opt_ LPCSTR ServerName,
    _Out_ PULONG EntryCount,
    _Out_ LPSTR **SiteNames)
{
    FIXME("DsGetDcSiteCoverageA(%s, %p, %p)\n",
          debugstr_a(ServerName), EntryCount, SiteNames);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsGetDcSiteCoverageW(
    _In_opt_ LPCWSTR ServerName,
    _Out_ PULONG EntryCount,
    _Out_ LPWSTR **SiteNames)
{
    PNL_SITE_NAME_ARRAY SiteNameArray = NULL;
    PWSTR *SiteNamesBuffer = NULL, Ptr;
    ULONG BufferSize, i;
    NET_API_STATUS status;

    TRACE("DsGetDcSiteCoverageW(%s, %p, %p)\n",
          debugstr_w(ServerName), EntryCount, SiteNames);

    *EntryCount = 0;
    *SiteNames = NULL;

    RpcTryExcept
    {
        status = DsrGetDcSiteCoverageW((PWSTR)ServerName,
                                       &SiteNameArray);
        if (status == NERR_Success)
        {
            if (SiteNameArray->EntryCount == 0)
            {
                status = ERROR_INVALID_PARAMETER;
            }
            else
            {
                BufferSize = SiteNameArray->EntryCount * sizeof(PWSTR);
                for (i = 0; i < SiteNameArray->EntryCount; i++)
                    BufferSize += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);

                status = NetApiBufferAllocate(BufferSize, (PVOID*)&SiteNamesBuffer);
                if (status == NERR_Success)
                {
                    ZeroMemory(SiteNamesBuffer, BufferSize);

                    Ptr = (PWSTR)((ULONG_PTR)SiteNamesBuffer + SiteNameArray->EntryCount * sizeof(PWSTR));
                    for (i = 0; i < SiteNameArray->EntryCount; i++)
                    {
                        SiteNamesBuffer[i] = Ptr;
                        CopyMemory(Ptr,
                                   SiteNameArray->SiteNames[i].Buffer,
                                   SiteNameArray->SiteNames[i].Length);

                        Ptr = (PWSTR)((ULONG_PTR)Ptr + SiteNameArray->SiteNames[i].Length + sizeof(WCHAR));
                    }

                    *EntryCount = SiteNameArray->EntryCount;
                    *SiteNames = SiteNamesBuffer;
                }
            }

            MIDL_user_free(SiteNameArray);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


DWORD
WINAPI
DsGetForestTrustInformationW(
    _In_opt_ LPCWSTR ServerName,
    _In_opt_ LPCWSTR TrustedDomainName,
    _In_ DWORD Flags,
    _Out_ PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    NET_API_STATUS status;

    TRACE("DsGetForestTrustInformationW(%s, %s, 0x%08lx, %p)\n",
          debugstr_w(ServerName), debugstr_w(TrustedDomainName),
          Flags, ForestTrustInfo);

    RpcTryExcept
    {
        status = DsrGetForestTrustInformation((PWSTR)ServerName,
                                              (PWSTR)TrustedDomainName,
                                              Flags,
                                              ForestTrustInfo);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
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


DWORD
WINAPI
DsMergeForestTrustInformationW(
    _In_ LPCWSTR DomainName,
    _In_ PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo,
    _In_opt_ PLSA_FOREST_TRUST_INFORMATION OldForestTrustInfo,
    _Out_ PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    FIXME("DsMergeForestTrustInformationW(%s, %p, %p, %p)\n",
          debugstr_w(DomainName), NewForestTrustInfo,
          OldForestTrustInfo, ForestTrustInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsValidateSubnetNameA(
    _In_ LPCSTR SubnetName)
{
    FIXME("DsValidateSubnetNameA(%s)\n",
          debugstr_a(SubnetName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
DsValidateSubnetNameW(
    _In_ LPCWSTR SubnetName)
{
    FIXME("DsValidateSubnetNameW(%s)\n",
          debugstr_w(SubnetName));
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    NET_API_STATUS Status;

    TRACE("NetGetAnyDCName(%s, %s, %p)\n",
          debugstr_w(ServerName), debugstr_w(DomainName), BufPtr);

    *BufPtr = NULL;

    RpcTryExcept
    {
        Status = NetrGetAnyDCName((PWSTR)ServerName,
                                  (PWSTR)DomainName,
                                  (PWSTR*)BufPtr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NET_API_STATUS
WINAPI
NetGetDCName(
    _In_opt_ LPCWSTR ServerName,
    _In_opt_ LPCWSTR DomainName,
    _Out_ LPBYTE *BufPtr)
{
    PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo = NULL;
    NET_API_STATUS Status;

    FIXME("NetGetDCName(%s, %s, %p)\n",
          debugstr_w(ServerName), debugstr_w(DomainName), BufPtr);

    if (ServerName == NULL || *ServerName == UNICODE_NULL)
    {
        Status = DsGetDcNameWithAccountW(NULL,
                                         NULL,
                                         0,
                                         DomainName,
                                         NULL,
                                         NULL,
                                         0, //???
                                         &pDomainControllerInfo);
        if (Status != NERR_Success)
            goto done;

        Status = NetApiBufferAllocate((wcslen(pDomainControllerInfo->DomainControllerName) + 1) * sizeof(WCHAR),
                                      (PVOID*)BufPtr);
        if (Status != NERR_Success)
            goto done;

        wcscpy((PWSTR)*BufPtr,
               pDomainControllerInfo->DomainControllerName);
    }
    else
    {
        FIXME("Not implemented yet!\n");
        Status = NERR_DCNotFound;
    }

done:
    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    return Status;
}


NET_API_STATUS
WINAPI
NetLogonGetTimeServiceParentDomain(
    _In_ LPWSTR ServerName,
    _Out_ LPWSTR *DomainName,
    _Out_ LPBOOL PdcSameSite)
{
    NET_API_STATUS Status;

    TRACE("NetLogonGetTimeServiceParentDomain(%s, %p, %p)\n",
          debugstr_w(ServerName), DomainName, PdcSameSite);

    RpcTryExcept
    {
        Status = NetrLogonGetTimeServiceParentDomain(ServerName,
                                                     DomainName,
                                                     PdcSameSite);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
WINAPI
NetLogonSetServiceBits(
    _In_ LPWSTR ServerName,
    _In_ DWORD ServiceBitsOfInterest,
    _In_ DWORD ServiceBits)
{
    NTSTATUS Status;

    TRACE("NetLogonSetServiceBits(%s 0x%lx 0x%lx)\n",
          debugstr_w(ServerName), ServiceBitsOfInterest, ServiceBits);

    RpcTryExcept
    {
        Status = NetrLogonSetServiceBits(ServerName,
                                         ServiceBitsOfInterest,
                                         ServiceBits);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept;

    return Status;
}

/* EOF */
