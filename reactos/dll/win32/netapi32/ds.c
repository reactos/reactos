/*
 * Copyright 2005 Paul Vriens
 *
 * netapi32 directory service functions
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

#include "netapi32.h"

#include <dsrole.h>
#include <dsgetdc.h>

WINE_DEFAULT_DEBUG_CHANNEL(ds);

DWORD WINAPI DsGetDcNameW(LPCWSTR ComputerName, LPCWSTR AvoidDCName,
 GUID* DomainGuid, LPCWSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_w(ComputerName),
     debugstr_w(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_w(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetDcNameA(LPCSTR ComputerName, LPCSTR AvoidDCName,
 GUID* DomainGuid, LPCSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_a(ComputerName),
     debugstr_a(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_a(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameW(LPCWSTR ComputerName, LPWSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_w(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************
 *  DsRoleFreeMemory (NETAPI32.@)
 *
 * PARAMS
 *  Buffer [I] Pointer to the to-be-freed buffer.
 *
 * RETURNS
 *  Nothing
 */
VOID WINAPI DsRoleFreeMemory(PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}

/************************************************************
 *  DsRoleGetPrimaryDomainInformation  (NETAPI32.@)
 *
 * PARAMS
 *  lpServer  [I] Pointer to UNICODE string with ComputerName
 *  InfoLevel [I] Type of data to retrieve	
 *  Buffer    [O] Pointer to to the requested data
 *
 * RETURNS
 *
 * NOTES
 *  When lpServer is NULL, use the local computer
 */
DWORD WINAPI DsRoleGetPrimaryDomainInformation(
    LPCWSTR lpServer, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PBYTE* Buffer)
{
    DWORD ret;

    FIXME("(%p, %d, %p) stub\n", lpServer, InfoLevel, Buffer);

    /* Check some input parameters */

    if (!Buffer) return ERROR_INVALID_PARAMETER;
    if ((InfoLevel < DsRolePrimaryDomainInfoBasic) || (InfoLevel > DsRoleOperationState)) return ERROR_INVALID_PARAMETER;

    *Buffer = NULL;
    switch (InfoLevel)
    {
        case DsRolePrimaryDomainInfoBasic:
        {
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
            NTSTATUS NtStatus;
            int logon_domain_sz;
            DWORD size;
            PDSROLE_PRIMARY_DOMAIN_INFO_BASIC basic;

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
            {
                TRACE("LsaOpenPolicyFailed with NT status %x\n",
                    LsaNtStatusToWinError(NtStatus));
                return ERROR_OUTOFMEMORY;
            }
            LsaQueryInformationPolicy(PolicyHandle,
             PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
            logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
            LsaClose(PolicyHandle);

            size = sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC) +
             logon_domain_sz * sizeof(WCHAR);
            basic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            if (basic)
            {
                basic->MachineRole = DsRole_RoleStandaloneWorkstation;
                basic->DomainNameFlat = (LPWSTR)((LPBYTE)basic +
                 sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
                lstrcpyW(basic->DomainNameFlat, DomainInfo->DomainName.Buffer);
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)basic;
            LsaFreeMemory(DomainInfo);
        }
        break;

        case DsRoleUpgradeStatus:
        {
            PDSROLE_UPGRADE_STATUS_INFO buffer;

            buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DSROLE_UPGRADE_STATUS_INFO));
            if (buffer)
            {
                buffer->OperationState = 0;
                buffer->PreviousServerState = 0;
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)buffer;
        }
        break;

        case DsRoleOperationState:
        {
            PDSROLE_OPERATION_STATE_INFO buffer;

            buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DSROLE_OPERATION_STATE_INFO));
            if (buffer)
            {
                buffer->OperationState = DsRoleOperationIdle;
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)buffer;
        }
        break;

    default:
        ret = ERROR_CALL_NOT_IMPLEMENTED;
    }
    return ret;
}
