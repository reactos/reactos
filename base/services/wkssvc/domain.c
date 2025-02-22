/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/wkssvc/domain.c
 * PURPOSE:          Workstation service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wkssvc);

/* FUNCTIONS *****************************************************************/

static
NET_API_STATUS
NetpSetPrimaryDomain(
    _In_ LPCWSTR lpWorkgroupName)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    POLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;

    ZeroMemory(&ObjectAttributes, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_TRUST_ADMIN,
                           &PolicyHandle);
    if (!LSA_SUCCESS(Status))
        return LsaNtStatusToWinError(Status);

    RtlInitUnicodeString(&PrimaryDomainInfo.Name,
                         lpWorkgroupName);
    PrimaryDomainInfo.Sid = NULL;

    Status = LsaSetInformationPolicy(PolicyHandle,
                                     PolicyPrimaryDomainInformation,
                                     &PrimaryDomainInfo);

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}


NET_API_STATUS
NetpJoinWorkgroup(
    _In_ LPCWSTR lpWorkgroupName)
{
    NET_API_STATUS status;

    FIXME("NetpJoinWorkgroup(%S)\n", lpWorkgroupName);

    status = NetpSetPrimaryDomain(lpWorkgroupName);
    if (status != NERR_Success)
    {
        ERR("NetpSetPrimaryDomain failed (Status %lu)\n", status);
        return status;
    }

    return NERR_Success;
}


NET_API_STATUS
NetpGetJoinInformation(
    LPWSTR *NameBuffer,
    PNETSETUP_JOIN_STATUS BufferType)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;

    *BufferType = NetSetupUnknownStatus;
    *NameBuffer = NULL;

    ZeroMemory(&ObjectAttributes, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if (!LSA_SUCCESS(Status))
        return LsaNtStatusToWinError(Status);

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyPrimaryDomainInformation,
                                       (PVOID*)&PrimaryDomainInfo);
    if (LSA_SUCCESS(Status))
    {
        TRACE("Sid: %p\n", PrimaryDomainInfo->Sid);
        TRACE("Name: %S\n", PrimaryDomainInfo->Name.Buffer);

        if (PrimaryDomainInfo->Name.Length > 0)
        {
            if (PrimaryDomainInfo->Sid != NULL)
                *BufferType = NetSetupDomainName;
            else
                *BufferType = NetSetupWorkgroupName;

            *NameBuffer = midl_user_allocate(PrimaryDomainInfo->Name.Length + sizeof(WCHAR));
            if (*NameBuffer)
                wcscpy(*NameBuffer, PrimaryDomainInfo->Name.Buffer);
        }
        else
        {
            *BufferType = NetSetupUnjoined;
        }

        if (PrimaryDomainInfo->Sid)
            LsaFreeMemory(PrimaryDomainInfo->Sid);

        LsaFreeMemory(PrimaryDomainInfo);
    }

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}

/* EOF */
