/*
 *  ReactOS Services
 *  Copyright (C) 2015 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/wkssvc/rpcserver.c
 * PURPOSE:          Workstation service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "lmerr.h"

WINE_DEFAULT_DEBUG_CHANNEL(wkssvc);

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\wkssvc", NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(wkssvc_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


static
NET_API_STATUS
NetpGetClientLogonId(
    _Out_ PLUID LogonId)
{
    HANDLE ThreadToken = NULL;
    TOKEN_STATISTICS Statistics;
    ULONG Length;
    NTSTATUS NtStatus;
    NET_API_STATUS ApiStatus = NERR_Success;

    ApiStatus = RpcImpersonateClient(NULL);
    if (ApiStatus != NERR_Success)
        return ApiStatus;

    NtStatus = NtOpenThreadToken(NtCurrentThread(),
                                 TOKEN_QUERY,
                                 TRUE,
                                 &ThreadToken);
    if (!NT_SUCCESS(NtStatus))
    {
        ApiStatus = RtlNtStatusToDosError(NtStatus);
        goto done;
    }

    NtStatus = NtQueryInformationToken(ThreadToken,
                                       TokenStatistics,
                                       (PVOID)&Statistics,
                                       sizeof(Statistics),
                                       &Length);
    if (!NT_SUCCESS(NtStatus))
    {
        ApiStatus = RtlNtStatusToDosError(NtStatus);
        goto done;
    }

    TRACE("Client LUID: %lx\n", Statistics.AuthenticationId.LowPart);
    RtlCopyLuid(LogonId, &Statistics.AuthenticationId);

done:
    if (ThreadToken != NULL)
        NtClose(ThreadToken);

    RpcRevertToSelf();

    return ApiStatus;
}


/* Function 0 */
unsigned long
__stdcall
NetrWkstaGetInfo(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    unsigned long Level,
    LPWKSTA_INFO *WkstaInfo)
{
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwComputerNameLength;
    LPCWSTR pszLanRoot = L"";
    PWKSTA_INFO pWkstaInfo = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO DomainInfo = NULL;
    ULONG LoggedOnUsers;
    NTSTATUS NtStatus;
    DWORD dwResult = NERR_Success;

    TRACE("NetrWkstaGetInfo level %lu\n", Level);

    dwComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(szComputerName, &dwComputerNameLength);
    dwComputerNameLength++; /* include NULL terminator */

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    NtStatus = LsaOpenPolicy(NULL,
                             &ObjectAttributes,
                             POLICY_VIEW_LOCAL_INFORMATION,
                             &PolicyHandle);
    if (NtStatus != STATUS_SUCCESS)
    {
        WARN("LsaOpenPolicy() failed (Status 0x%08lx)\n", NtStatus);
        return LsaNtStatusToWinError(NtStatus);
    }

    NtStatus = LsaQueryInformationPolicy(PolicyHandle,
                                         PolicyPrimaryDomainInformation,
                                         (PVOID*)&DomainInfo);

    LsaClose(PolicyHandle);

    if (NtStatus != STATUS_SUCCESS)
    {
        WARN("LsaQueryInformationPolicy() failed (Status 0x%08lx)\n", NtStatus);
        return LsaNtStatusToWinError(NtStatus);
    }

    if (Level == 102)
    {
        MSV1_0_ENUMUSERS_REQUEST EnumRequest;
        PMSV1_0_ENUMUSERS_RESPONSE EnumResponseBuffer = NULL;
        DWORD EnumResponseBufferSize = 0;
        NTSTATUS ProtocolStatus;

        /* enumerate all currently logged-on users */
        EnumRequest.MessageType = MsV1_0EnumerateUsers;
        NtStatus = LsaCallAuthenticationPackage(LsaHandle,
                                                LsaAuthenticationPackage,
                                                &EnumRequest,
                                                sizeof(EnumRequest),
                                                (PVOID*)&EnumResponseBuffer,
                                                &EnumResponseBufferSize,
                                                &ProtocolStatus);
        if (!NT_SUCCESS(NtStatus))
        {
            dwResult = RtlNtStatusToDosError(NtStatus);
            goto done;
        }

        LoggedOnUsers = EnumResponseBuffer->NumberOfLoggedOnUsers;

        LsaFreeReturnBuffer(EnumResponseBuffer);
    }

    switch (Level)
    {
        case 100:
            pWkstaInfo = midl_user_allocate(sizeof(WKSTA_INFO_100));
            if (pWkstaInfo == NULL)
            {
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pWkstaInfo->WkstaInfo100.wki100_platform_id = PLATFORM_ID_NT;

            pWkstaInfo->WkstaInfo100.wki100_computername = midl_user_allocate(dwComputerNameLength * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo100.wki100_computername != NULL)
                wcscpy(pWkstaInfo->WkstaInfo100.wki100_computername, szComputerName);

            pWkstaInfo->WkstaInfo100.wki100_langroup = midl_user_allocate((wcslen(DomainInfo->Name.Buffer) + 1) * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo100.wki100_langroup != NULL)
                wcscpy(pWkstaInfo->WkstaInfo100.wki100_langroup, DomainInfo->Name.Buffer);

            pWkstaInfo->WkstaInfo100.wki100_ver_major = VersionInfo.dwMajorVersion;
            pWkstaInfo->WkstaInfo100.wki100_ver_minor = VersionInfo.dwMinorVersion;

            *WkstaInfo = pWkstaInfo;
            break;

        case 101:
            pWkstaInfo = midl_user_allocate(sizeof(WKSTA_INFO_101));
            if (pWkstaInfo == NULL)
            {
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pWkstaInfo->WkstaInfo101.wki101_platform_id = PLATFORM_ID_NT;

            pWkstaInfo->WkstaInfo101.wki101_computername = midl_user_allocate(dwComputerNameLength * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo101.wki101_computername != NULL)
                wcscpy(pWkstaInfo->WkstaInfo101.wki101_computername, szComputerName);

            pWkstaInfo->WkstaInfo101.wki101_langroup = midl_user_allocate((wcslen(DomainInfo->Name.Buffer) + 1) * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo101.wki101_langroup != NULL)
                wcscpy(pWkstaInfo->WkstaInfo101.wki101_langroup, DomainInfo->Name.Buffer);

            pWkstaInfo->WkstaInfo101.wki101_ver_major = VersionInfo.dwMajorVersion;
            pWkstaInfo->WkstaInfo101.wki101_ver_minor = VersionInfo.dwMinorVersion;

            pWkstaInfo->WkstaInfo101.wki101_lanroot = midl_user_allocate((wcslen(pszLanRoot) + 1) * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo101.wki101_lanroot != NULL)
                wcscpy(pWkstaInfo->WkstaInfo101.wki101_lanroot, pszLanRoot);

            *WkstaInfo = pWkstaInfo;
            break;

        case 102:
            pWkstaInfo = midl_user_allocate(sizeof(WKSTA_INFO_102));
            if (pWkstaInfo == NULL)
            {
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pWkstaInfo->WkstaInfo102.wki102_platform_id = PLATFORM_ID_NT;

            pWkstaInfo->WkstaInfo102.wki102_computername = midl_user_allocate(dwComputerNameLength * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo102.wki102_computername != NULL)
                wcscpy(pWkstaInfo->WkstaInfo102.wki102_computername, szComputerName);

            pWkstaInfo->WkstaInfo102.wki102_langroup = midl_user_allocate((wcslen(DomainInfo->Name.Buffer) + 1) * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo102.wki102_langroup != NULL)
                wcscpy(pWkstaInfo->WkstaInfo102.wki102_langroup, DomainInfo->Name.Buffer);

            pWkstaInfo->WkstaInfo102.wki102_ver_major = VersionInfo.dwMajorVersion;
            pWkstaInfo->WkstaInfo102.wki102_ver_minor = VersionInfo.dwMinorVersion;

            pWkstaInfo->WkstaInfo102.wki102_lanroot = midl_user_allocate((wcslen(pszLanRoot) + 1) * sizeof(WCHAR));
            if (pWkstaInfo->WkstaInfo102.wki102_lanroot != NULL)
                wcscpy(pWkstaInfo->WkstaInfo102.wki102_lanroot, pszLanRoot);

            pWkstaInfo->WkstaInfo102.wki102_logged_on_users = LoggedOnUsers;

            *WkstaInfo = pWkstaInfo;
            break;

        case 502:
            pWkstaInfo = midl_user_allocate(sizeof(WKSTA_INFO_502));
            if (pWkstaInfo == NULL)
            {
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            CopyMemory(&pWkstaInfo->WkstaInfo502, &WkstaInfo502, sizeof(WKSTA_INFO_502));

            *WkstaInfo = pWkstaInfo;
            break;

        default:
            FIXME("Level %lu unimplemented\n", Level);
            dwResult = ERROR_INVALID_LEVEL;
            break;
    }

done:
    if (DomainInfo != NULL)
        LsaFreeMemory(DomainInfo);

    return dwResult;
}


/* Function 1 */
unsigned long
__stdcall
NetrWkstaSetInfo(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    unsigned long Level,
    LPWKSTA_INFO WkstaInfo,
    unsigned long *ErrorParameter)
{
    DWORD dwErrParam = 0;
    DWORD dwResult = NERR_Success;

    TRACE("NetrWkstaSetInfo(%lu %p %p)\n",
          Level, WkstaInfo, ErrorParameter);

    switch (Level)
    {
        case 502:
            if (WkstaInfo->WkstaInfo502.wki502_keep_conn >= 1 && WkstaInfo->WkstaInfo502.wki502_keep_conn <= 65535)
            {
                WkstaInfo502.wki502_keep_conn = WkstaInfo->WkstaInfo502.wki502_keep_conn;
            }
            else
            {
                dwErrParam = WKSTA_KEEPCONN_PARMNUM;
                dwResult = ERROR_INVALID_PARAMETER;
            }

            if (dwResult == NERR_Success)
            {
                if (WkstaInfo->WkstaInfo502.wki502_max_cmds >= 50 && WkstaInfo->WkstaInfo502.wki502_max_cmds <= 65535)
                {
                    WkstaInfo502.wki502_max_cmds = WkstaInfo->WkstaInfo502.wki502_max_cmds;
                }
                else
                {
                    dwErrParam = WKSTA_MAXCMDS_PARMNUM;
                    dwResult = ERROR_INVALID_PARAMETER;
                }
            }

            if (dwResult == NERR_Success)
            {
                if (WkstaInfo->WkstaInfo502.wki502_sess_timeout >= 60 && WkstaInfo->WkstaInfo502.wki502_sess_timeout <= 65535)
                {
                    WkstaInfo502.wki502_sess_timeout = WkstaInfo->WkstaInfo502.wki502_sess_timeout;
                }
                else
                {
                    dwErrParam = WKSTA_SESSTIMEOUT_PARMNUM;
                    dwResult = ERROR_INVALID_PARAMETER;
                }
            }

            if (dwResult == NERR_Success)
            {
                if (WkstaInfo->WkstaInfo502.wki502_dormant_file_limit != 0)
                {
                    WkstaInfo502.wki502_dormant_file_limit = WkstaInfo->WkstaInfo502.wki502_dormant_file_limit;
                }
                else
                {
                    dwErrParam = WKSTA_DORMANTFILELIMIT_PARMNUM;
                    dwResult = ERROR_INVALID_PARAMETER;
                }
            }
            break;

        case 1013:
            if (WkstaInfo->WkstaInfo1013.wki1013_keep_conn >= 1 && WkstaInfo->WkstaInfo1013.wki1013_keep_conn <= 65535)
            {
                WkstaInfo502.wki502_keep_conn = WkstaInfo->WkstaInfo1013.wki1013_keep_conn;
            }
            else
            {
                dwErrParam = WKSTA_KEEPCONN_PARMNUM;
                dwResult = ERROR_INVALID_PARAMETER;
            }
            break;

        case 1018:
            if (WkstaInfo->WkstaInfo1018.wki1018_sess_timeout >= 60 && WkstaInfo->WkstaInfo1018.wki1018_sess_timeout <= 65535)
            {
                WkstaInfo502.wki502_sess_timeout = WkstaInfo->WkstaInfo1018.wki1018_sess_timeout;
            }
            else
            {
                dwErrParam = WKSTA_SESSTIMEOUT_PARMNUM;
                dwResult = ERROR_INVALID_PARAMETER;
            }
            break;

        case 1046:
            if (WkstaInfo->WkstaInfo1046.wki1046_dormant_file_limit != 0)
            {
                WkstaInfo502.wki502_dormant_file_limit = WkstaInfo->WkstaInfo1046.wki1046_dormant_file_limit;
            }
            else
            {
                dwErrParam = WKSTA_DORMANTFILELIMIT_PARMNUM;
                dwResult = ERROR_INVALID_PARAMETER;
            }
            break;

        default:
            ERR("Invalid Level %lu\n", Level);
            dwResult = ERROR_INVALID_LEVEL;
            break;
    }

    /* Save the workstation in the registry */
    if (dwResult == NERR_Success)
    {
        SaveWorkstationInfo(Level);

        /* FIXME: Notify the redirector */
    }

    if ((dwResult == ERROR_INVALID_PARAMETER) && (ErrorParameter != NULL))
        *ErrorParameter = dwErrParam;

    return dwResult;
}


/* Function 2 */
unsigned long
__stdcall
NetrWkstaUserEnum(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    LPWKSTA_USER_ENUM_STRUCT UserInfo,
    unsigned long PreferredMaximumLength,
    unsigned long *TotalEntries,
    unsigned long *ResumeHandle)
{
    MSV1_0_ENUMUSERS_REQUEST EnumRequest;
    PMSV1_0_ENUMUSERS_RESPONSE EnumResponseBuffer = NULL;
    MSV1_0_GETUSERINFO_REQUEST UserInfoRequest;
    PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponseBuffer = NULL;
    PMSV1_0_GETUSERINFO_RESPONSE *UserInfoArray = NULL;
    DWORD EnumResponseBufferSize = 0;
    DWORD UserInfoResponseBufferSize = 0;
    NTSTATUS Status, ProtocolStatus;
    ULONG i, start, count;
    PLUID pLogonId;
    PULONG pEnumHandle;
    DWORD dwResult = NERR_Success;

    PWKSTA_USER_INFO_0 pUserInfo0 = NULL;
    PWKSTA_USER_INFO_1 pUserInfo1 = NULL;

    TRACE("NetrWkstaUserEnum(%p %p 0x%lx %p %p)\n",
          ServerName, UserInfo, PreferredMaximumLength, TotalEntries, ResumeHandle);

    if (UserInfo->Level > 1)
    {
        ERR("Invalid Level %lu\n", UserInfo->Level);
        return ERROR_INVALID_LEVEL;
    }

    /* Enumerate all currently logged-on users */
    EnumRequest.MessageType = MsV1_0EnumerateUsers;
    Status = LsaCallAuthenticationPackage(LsaHandle,
                                          LsaAuthenticationPackage,
                                          &EnumRequest,
                                          sizeof(EnumRequest),
                                          (PVOID*)&EnumResponseBuffer,
                                          &EnumResponseBufferSize,
                                          &ProtocolStatus);

    TRACE("LsaCallAuthenticationPackage Status 0x%08lx ResponseBufferSize %lu\n", Status, EnumResponseBufferSize);
    if (!NT_SUCCESS(Status))
    {
        dwResult = RtlNtStatusToDosError(Status);
        goto done;
    }

    TRACE("LoggedOnUsers: %lu\n", EnumResponseBuffer->NumberOfLoggedOnUsers);
    TRACE("ResponseBuffer: 0x%p\n", EnumResponseBuffer);
    TRACE("LogonIds: 0x%p\n", EnumResponseBuffer->LogonIds);
    TRACE("EnumHandles: 0x%p\n", EnumResponseBuffer->EnumHandles);
    if (EnumResponseBuffer->NumberOfLoggedOnUsers > 0)
    {
        pLogonId = EnumResponseBuffer->LogonIds;
        pEnumHandle = EnumResponseBuffer->EnumHandles;
        TRACE("pLogonId: 0x%p\n", pLogonId);
        TRACE("pEnumHandle: 0x%p\n", pEnumHandle);

        UserInfoArray = RtlAllocateHeap(RtlGetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        EnumResponseBuffer->NumberOfLoggedOnUsers * sizeof(PMSV1_0_GETUSERINFO_RESPONSE));
        if (UserInfoArray == NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        for (i = 0; i < EnumResponseBuffer->NumberOfLoggedOnUsers; i++)
        {
            TRACE("Logon %lu: 0x%08lx  %lu\n", i, pLogonId->LowPart, *pEnumHandle);

            UserInfoRequest.MessageType = MsV1_0GetUserInfo;
            UserInfoRequest.LogonId = *pLogonId;
            Status = LsaCallAuthenticationPackage(LsaHandle,
                                                  LsaAuthenticationPackage,
                                                  &UserInfoRequest,
                                                  sizeof(UserInfoRequest),
                                                  (PVOID*)&UserInfoResponseBuffer,
                                                  &UserInfoResponseBufferSize,
                                                  &ProtocolStatus);
            TRACE("LsaCallAuthenticationPackage:MsV1_0GetUserInfo Status 0x%08lx ResponseBufferSize %lu\n", Status, UserInfoResponseBufferSize);
            if (!NT_SUCCESS(Status))
            {
                dwResult = RtlNtStatusToDosError(Status);
                goto done;
            }

            UserInfoArray[i] = UserInfoResponseBuffer;

            TRACE("UserName: %wZ\n", &UserInfoArray[i]->UserName);
            TRACE("LogonDomain: %wZ\n", &UserInfoArray[i]->LogonDomainName);
            TRACE("LogonServer: %wZ\n", &UserInfoArray[i]->LogonServer);

            pLogonId++;
            pEnumHandle++;
        }

        if (PreferredMaximumLength == MAX_PREFERRED_LENGTH)
        {
            start = 0;
            count = EnumResponseBuffer->NumberOfLoggedOnUsers;
        }
        else
        {
            FIXME("Calculate the start index and the number of matching array entries!");
            dwResult = ERROR_CALL_NOT_IMPLEMENTED;
            goto done;
        }

        switch (UserInfo->Level)
        {
            case 0:
                pUserInfo0 = midl_user_allocate(count * sizeof(WKSTA_USER_INFO_0));
                if (pUserInfo0 == NULL)
                {
                    ERR("\n");
                    dwResult = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                ZeroMemory(pUserInfo0, count * sizeof(WKSTA_USER_INFO_0));

                for (i = 0; i < 0 + count; i++) 
                {
                    pUserInfo0[i].wkui0_username = midl_user_allocate(UserInfoArray[start + i]->UserName.Length + sizeof(WCHAR));
                    if (pUserInfo0[i].wkui0_username == NULL)
                    {
                        ERR("\n");
                        dwResult = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    ZeroMemory(pUserInfo0[i].wkui0_username, UserInfoArray[start + i]->UserName.Length + sizeof(WCHAR));
                    CopyMemory(pUserInfo0[i].wkui0_username, UserInfoArray[start + i]->UserName.Buffer, UserInfoArray[start + i]->UserName.Length);
                }

                UserInfo->WkstaUserInfo.Level0.EntriesRead = count;
                UserInfo->WkstaUserInfo.Level0.Buffer = pUserInfo0;
                *TotalEntries = EnumResponseBuffer->NumberOfLoggedOnUsers;
                *ResumeHandle = 0;
                break;

            case 1:
                pUserInfo1 = midl_user_allocate(count * sizeof(WKSTA_USER_INFO_1));
                if (pUserInfo1 == NULL)
                {
                    ERR("\n");
                    dwResult = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                ZeroMemory(pUserInfo1, count * sizeof(WKSTA_USER_INFO_1));

                for (i = 0; i < 0 + count; i++) 
                {
                    pUserInfo1[i].wkui1_username = midl_user_allocate(UserInfoArray[start + i]->UserName.Length + sizeof(WCHAR));
                    if (pUserInfo1[i].wkui1_username == NULL)
                    {
                        ERR("\n");
                        dwResult = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    ZeroMemory(pUserInfo1[i].wkui1_username, UserInfoArray[start + i]->UserName.Length + sizeof(WCHAR));
                    CopyMemory(pUserInfo1[i].wkui1_username, UserInfoArray[start + i]->UserName.Buffer, UserInfoArray[start + i]->UserName.Length);

                    pUserInfo1[i].wkui1_logon_domain = midl_user_allocate(UserInfoArray[start + i]->LogonDomainName.Length + sizeof(WCHAR));
                    if (pUserInfo1[i].wkui1_logon_domain == NULL)
                    {
                        ERR("\n");
                        dwResult = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    ZeroMemory(pUserInfo1[i].wkui1_logon_domain, UserInfoArray[start + i]->LogonDomainName.Length + sizeof(WCHAR));
                    CopyMemory(pUserInfo1[i].wkui1_logon_domain, UserInfoArray[start + i]->LogonDomainName.Buffer, UserInfoArray[start + i]->LogonDomainName.Length);

                    // FIXME: wkui1_oth_domains

                    pUserInfo1[i].wkui1_logon_server = midl_user_allocate(UserInfoArray[start + i]->LogonServer.Length + sizeof(WCHAR));
                    if (pUserInfo1[i].wkui1_logon_server == NULL)
                    {
                        ERR("\n");
                        dwResult = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    ZeroMemory(pUserInfo1[i].wkui1_logon_server, UserInfoArray[start + i]->LogonServer.Length + sizeof(WCHAR));
                    CopyMemory(pUserInfo1[i].wkui1_logon_server, UserInfoArray[start + i]->LogonServer.Buffer, UserInfoArray[start + i]->LogonServer.Length);
                }

                UserInfo->WkstaUserInfo.Level1.EntriesRead = count;
                UserInfo->WkstaUserInfo.Level1.Buffer = pUserInfo1;
                *TotalEntries = EnumResponseBuffer->NumberOfLoggedOnUsers;
                *ResumeHandle = 0;
                break;

                break;
        }
    }
    else
    {
        if (UserInfo->Level == 0)
        {
            UserInfo->WkstaUserInfo.Level0.Buffer = NULL;
            UserInfo->WkstaUserInfo.Level0.EntriesRead = 0;
        }
        else
        {
            UserInfo->WkstaUserInfo.Level1.Buffer = NULL;
            UserInfo->WkstaUserInfo.Level1.EntriesRead = 0;
        }

        *TotalEntries = 0;
        dwResult = NERR_Success;
    }

done:
    if (UserInfoArray !=NULL)
    {

        for (i = 0; i < EnumResponseBuffer->NumberOfLoggedOnUsers; i++)
        {
            if (UserInfoArray[i]->UserName.Buffer != NULL)
                LsaFreeReturnBuffer(UserInfoArray[i]->UserName.Buffer);

            if (UserInfoArray[i]->LogonDomainName.Buffer != NULL)
                LsaFreeReturnBuffer(UserInfoArray[i]->LogonDomainName.Buffer);

            if (UserInfoArray[i]->LogonServer.Buffer != NULL)
                LsaFreeReturnBuffer(UserInfoArray[i]->LogonServer.Buffer);

            LsaFreeReturnBuffer(UserInfoArray[i]);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, UserInfoArray);
    }

    if (EnumResponseBuffer != NULL)
        LsaFreeReturnBuffer(EnumResponseBuffer);

    return dwResult;
}


/* Function 3 */
unsigned long
__stdcall
NetrWkstaUserGetInfo(
    WKSSVC_IDENTIFY_HANDLE Unused,
    unsigned long Level,
    LPWKSTA_USER_INFO *UserInfo)
{
    MSV1_0_GETUSERINFO_REQUEST UserInfoRequest;
    PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponseBuffer = NULL;
    DWORD UserInfoResponseBufferSize = 0;
    NTSTATUS Status, ProtocolStatus;
    LUID LogonId;
    PWKSTA_USER_INFO pUserInfo;
    DWORD dwResult = NERR_Success;

    TRACE("NetrWkstaUserGetInfo(%s, %d, %p)\n", debugstr_w(Unused), Level, UserInfo);

    if (Unused != NULL)
        return ERROR_INVALID_PARAMETER;

    if (Level > 1 && Level != 1101)
        return ERROR_INVALID_LEVEL;

    if (Level != 1101)
    {
        dwResult = NetpGetClientLogonId(&LogonId);
        if (dwResult != NERR_Success)
        {
            ERR("NetpGetClientLogonId() failed (%u)\n", dwResult);
            return dwResult;
        }

        TRACE("LogonId: 0x%08lx\n", LogonId.LowPart);

        UserInfoRequest.MessageType = MsV1_0GetUserInfo;
        UserInfoRequest.LogonId = LogonId;
        Status = LsaCallAuthenticationPackage(LsaHandle,
                                              LsaAuthenticationPackage,
                                              &UserInfoRequest,
                                              sizeof(UserInfoRequest),
                                              (PVOID*)&UserInfoResponseBuffer,
                                              &UserInfoResponseBufferSize,
                                              &ProtocolStatus);
        TRACE("LsaCallAuthenticationPackage:MsV1_0GetUserInfo Status 0x%08lx ResponseBufferSize %lu\n", Status, UserInfoResponseBufferSize);
        if (!NT_SUCCESS(Status))
        {
            ERR("\n");
            return RtlNtStatusToDosError(Status);
        }

        TRACE("UserName: %wZ\n", &UserInfoResponseBuffer->UserName);
        TRACE("LogonDomain: %wZ\n", &UserInfoResponseBuffer->LogonDomainName);
        TRACE("LogonServer: %wZ\n", &UserInfoResponseBuffer->LogonServer);
    }

    switch (Level)
    {
        case 0:
            pUserInfo = midl_user_allocate(sizeof(WKSTA_USER_INFO_0));
            if (pUserInfo == NULL)
            {
                ERR("\n");
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo, sizeof(WKSTA_USER_INFO_0));

            /* User Name */
            pUserInfo->UserInfo0.wkui0_username =
                midl_user_allocate(UserInfoResponseBuffer->UserName.Length + sizeof(WCHAR));
            if (pUserInfo->UserInfo0.wkui0_username == NULL)
            {
                ERR("\n");
                midl_user_free(pUserInfo);
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo->UserInfo0.wkui0_username,
                       UserInfoResponseBuffer->UserName.Length + sizeof(WCHAR));
            CopyMemory(pUserInfo->UserInfo0.wkui0_username,
                       UserInfoResponseBuffer->UserName.Buffer,
                       UserInfoResponseBuffer->UserName.Length);

            *UserInfo = pUserInfo;
            break;

        case 1:
            pUserInfo = midl_user_allocate(sizeof(WKSTA_USER_INFO_1));
            if (pUserInfo == NULL)
            {
                ERR("\n");
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo, sizeof(WKSTA_USER_INFO_1));

            /* User Name */
            pUserInfo->UserInfo1.wkui1_username =
                midl_user_allocate(UserInfoResponseBuffer->UserName.Length + sizeof(WCHAR));
            if (pUserInfo->UserInfo1.wkui1_username == NULL)
            {
                ERR("\n");
                midl_user_free(pUserInfo);
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo->UserInfo1.wkui1_username,
                       UserInfoResponseBuffer->UserName.Length + sizeof(WCHAR));
            CopyMemory(pUserInfo->UserInfo1.wkui1_username,
                       UserInfoResponseBuffer->UserName.Buffer,
                       UserInfoResponseBuffer->UserName.Length);

            /* Logon Domain Name */
            pUserInfo->UserInfo1.wkui1_logon_domain =
                midl_user_allocate(UserInfoResponseBuffer->LogonDomainName.Length + sizeof(WCHAR));
            if (pUserInfo->UserInfo1.wkui1_logon_domain == NULL)
            {
                ERR("\n");
                midl_user_free(pUserInfo->UserInfo1.wkui1_username);
                midl_user_free(pUserInfo);
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo->UserInfo1.wkui1_logon_domain,
                       UserInfoResponseBuffer->LogonDomainName.Length + sizeof(WCHAR));
            CopyMemory(pUserInfo->UserInfo1.wkui1_logon_domain,
                       UserInfoResponseBuffer->LogonDomainName.Buffer,
                       UserInfoResponseBuffer->LogonDomainName.Length);

            /* FIXME: wkui1_oth_domains */

            /* Logon Server */
            pUserInfo->UserInfo1.wkui1_logon_server =
                midl_user_allocate(UserInfoResponseBuffer->LogonServer.Length + sizeof(WCHAR));
            if (pUserInfo->UserInfo1.wkui1_logon_server == NULL)
            {
                ERR("\n");
                midl_user_free(pUserInfo->UserInfo1.wkui1_username);
                midl_user_free(pUserInfo->UserInfo1.wkui1_logon_domain);
                midl_user_free(pUserInfo);
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo->UserInfo1.wkui1_logon_server,
                       UserInfoResponseBuffer->LogonServer.Length + sizeof(WCHAR));
            CopyMemory(pUserInfo->UserInfo1.wkui1_logon_server,
                       UserInfoResponseBuffer->LogonServer.Buffer,
                       UserInfoResponseBuffer->LogonServer.Length);

            *UserInfo = pUserInfo;
            break;

        case 1101:
            pUserInfo = midl_user_allocate(sizeof(WKSTA_USER_INFO_1101));
            if (pUserInfo == NULL)
            {
                ERR("Failed to allocate WKSTA_USER_INFO_1101\n");
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            ZeroMemory(pUserInfo, sizeof(WKSTA_USER_INFO_1101));

            /* FIXME: wkui1101_oth_domains */

            *UserInfo = pUserInfo;
            break;

        default:
            ERR("\n");
            dwResult = ERROR_INVALID_LEVEL;
            break;
    }

    if (UserInfoResponseBuffer)
        LsaFreeReturnBuffer(UserInfoResponseBuffer);

    return dwResult;
}


/* Function 4 */
unsigned long
__stdcall
NetrWkstaUserSetInfo (
    WKSSVC_IDENTIFY_HANDLE Unused,
    unsigned long Level,
    LPWKSTA_USER_INFO UserInfo,
    unsigned long *ErrorParameter)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 5 */
unsigned long
__stdcall
NetrWkstaTransportEnum(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    LPWKSTA_TRANSPORT_ENUM_STRUCT TransportInfo,
    unsigned long PreferredMaximumLength,
    unsigned long* TotalEntries,
    unsigned long *ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 6 */
unsigned long
__stdcall
NetrWkstaTransportAdd(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    unsigned long Level,
    LPWKSTA_TRANSPORT_INFO_0 TransportInfo,
    unsigned long *ErrorParameter)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 7 */
unsigned long
__stdcall
NetrWkstaTransportDel(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    wchar_t *TransportName,
    unsigned long ForceLevel)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 8 */
unsigned long
__stdcall
NetrUseAdd(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    unsigned long Level,
    LPUSE_INFO InfoStruct,
    unsigned long *ErrorParameter)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 9 */
unsigned long
__stdcall
NetrUseGetInfo(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    wchar_t *UseName,
    unsigned long Level,
    LPUSE_INFO InfoStruct)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 10 */
unsigned long
__stdcall
NetrUseDel(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    wchar_t *UseName,
    unsigned long ForceLevel)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 11 */
unsigned long
__stdcall
NetrUseEnum(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    LPUSE_ENUM_STRUCT InfoStruct,
    unsigned long PreferredMaximumLength,
    unsigned long *TotalEntries,
    unsigned long *ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 12 - Not used on wire */
unsigned long
__stdcall
NetrMessageBufferSend(void)
{
    TRACE("NetrMessageBufferSend()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 13 */
unsigned long
__stdcall
NetrWorkstationStatisticsGet(
    WKSSVC_IDENTIFY_HANDLE ServerName,
    wchar_t *ServiceName,
    unsigned long Level,
    unsigned long Options,
    LPSTAT_WORKSTATION_0 *Buffer)
{
    PSTAT_WORKSTATION_0 pStatBuffer;

    TRACE("NetrWorkstationStatisticsGet(%p %p %lu 0x%lx %p)\n",
          ServerName, ServiceName, Level, Options, Buffer);

    if (Level != 0)
        return ERROR_INVALID_LEVEL;

    if (Options != 0)
        return ERROR_INVALID_PARAMETER;

    pStatBuffer = midl_user_allocate(sizeof(STAT_WORKSTATION_0));
    if (pStatBuffer == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    ZeroMemory(pStatBuffer, sizeof(STAT_WORKSTATION_0));

    // FIXME: Return the actual statistcs data!

    *Buffer = pStatBuffer;

    return NERR_Success;
}


/* Function 14 - Not used on wire */
unsigned long
__stdcall
NetrLogonDomainNameAdd(
    WKSSVC_IDENTIFY_HANDLE DomainName)
{
    TRACE("NetrLogonDomainNameAdd(%s)\n",
          debugstr_w(DomainName));
    return ERROR_NOT_SUPPORTED;
}


/* Function 15 - Not used on wire */
unsigned long
__stdcall
NetrLogonDomainNameDel(
    WKSSVC_IDENTIFY_HANDLE DomainName)
{
    TRACE("NetrLogonDomainNameDel(%s)\n",
          debugstr_w(DomainName));
    return ERROR_NOT_SUPPORTED;
}


/* Function 16 - Not used on wire */
unsigned long
__stdcall
NetrJoinDomain(void)
{
    TRACE("NetrJoinDomain()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 17 - Not used on wire */
unsigned long
__stdcall
NetrUnjoinDomain(void)
{
    TRACE("NetrUnjoinDomain()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 18 - Not used on wire */
unsigned long
__stdcall
NetrValidateName(void)
{
    TRACE("NetrValidateName()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 19 - Not used on wire */
unsigned long
__stdcall
NetrRenameMachineInDomain(void)
{
    TRACE("NetrRenameMachineInDomain()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 20 */
unsigned long
__stdcall
NetrGetJoinInformation(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    wchar_t **NameBuffer,
    PNETSETUP_JOIN_STATUS BufferType)
{
    TRACE("NetrGetJoinInformation(%p %p %p)\n",
          ServerName, NameBuffer, BufferType);

    if (NameBuffer == NULL)
        return ERROR_INVALID_PARAMETER;

    return NetpGetJoinInformation(NameBuffer,
                                  BufferType);
}


/* Function 21 - Not used on wire */
unsigned long
__stdcall
NetrGetJoinableOUs(void)
{
    TRACE("NetrGetJoinableOUs()\n");
    return ERROR_NOT_SUPPORTED;
}


/* Function 22 */
unsigned long
__stdcall
NetrJoinDomain2(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *DomainNameParam,
    wchar_t *MachineAccountOU,
    wchar_t *AccountName,
    PJOINPR_ENCRYPTED_USER_PASSWORD Password,
    unsigned long Options)
{
    NET_API_STATUS status;

    FIXME("NetrJoinDomain2(%p %S %S %S %S %p 0x%lx)\n",
          RpcBindingHandle, ServerName, DomainNameParam, MachineAccountOU,
          AccountName, Password, Options);

    if (DomainNameParam == NULL)
        return ERROR_INVALID_PARAMETER;

    if (Options & NETSETUP_JOIN_DOMAIN)
    {
        FIXME("NetrJoinDomain2: NETSETUP_JOIN_DOMAIN is not supported yet!\n");
        status = ERROR_CALL_NOT_IMPLEMENTED;
    }
    else
    {
        status = NetpJoinWorkgroup(DomainNameParam);
    }

    return status;
}


/* Function 23 */
unsigned long
__stdcall
NetrUnjoinDomain2(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *AccountName,
    PJOINPR_ENCRYPTED_USER_PASSWORD Password,
    unsigned long Options)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 24 */
unsigned long
__stdcall
NetrRenameMachineInDomain2(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *MachineName,
    wchar_t *AccountName,
    PJOINPR_ENCRYPTED_USER_PASSWORD Password,
    unsigned long Options)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 25 */
unsigned long
__stdcall
NetrValidateName2(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *NameToValidate,
    wchar_t *AccountName,
    PJOINPR_ENCRYPTED_USER_PASSWORD Password,
    NETSETUP_NAME_TYPE NameType)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 26 */
unsigned long
__stdcall
NetrGetJoinableOUs2(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *DomainNameParam,
    wchar_t *AccountName,
    PJOINPR_ENCRYPTED_USER_PASSWORD Password,
    unsigned long* OUCount,
    wchar_t ***OUs)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 27 */
unsigned long
__stdcall
NetrAddAlternateComputerName(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *AlternateName,
    wchar_t *DomainAccount,
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    unsigned long Reserved)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 28 */
unsigned long
__stdcall
NetrRemoveAlternateComputerName(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *AlternateName,
    wchar_t *DomainAccount,
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    unsigned long Reserved)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 29 */
unsigned long
__stdcall
NetrSetPrimaryComputerName(
    handle_t RpcBindingHandle,
    wchar_t *ServerName,
    wchar_t *PrimaryName,
    wchar_t *DomainAccount,
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    unsigned long Reserved)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 30 */
unsigned long
__stdcall
NetrEnumerateComputerNames(
    WKSSVC_IMPERSONATE_HANDLE ServerName,
    NET_COMPUTER_NAME_TYPE NameType,
    unsigned long Reserved,
    PNET_COMPUTER_NAME_ARRAY *ComputerNames)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
