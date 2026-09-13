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
    _Out_ LPWSTR *NameBuffer,
    _Out_ PNETSETUP_JOIN_STATUS BufferType)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    LPWSTR Name = NULL;
    NETSETUP_JOIN_STATUS Type = NetSetupUnknownStatus;
    NTSTATUS Status;

    TRACE("NetpGetJoinInformation(%p, %p)\n", NameBuffer, BufferType);

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
    if (!LSA_SUCCESS(Status))
    {
        ERR("LsaQueryInformationPolicy failed (Status 0x%lx)\n", Status);
        goto done;
    }

    TRACE("Sid: %p\n", PrimaryDomainInfo->Sid);
    TRACE("Name: %hu %S\n", PrimaryDomainInfo->Name.Length, PrimaryDomainInfo->Name.Buffer);

    if (PrimaryDomainInfo->Name.Length > 0)
    {
        TRACE("PD name!\n");
        if (PrimaryDomainInfo->Sid != NULL)
        {
            TRACE("PD SID! -->Domain\n");
            Type = NetSetupDomainName;
        }
        else
        {
            TRACE("No PD SID! --> Workgroup\n");
            Type = NetSetupWorkgroupName;
        }
    }
    else
    {
        TRACE("No PD name!\n");
        Type = NetSetupUnjoined;
    }

    TRACE("BufferType: %u\n", *BufferType);

    if ((Type != NetSetupUnjoined) && (PrimaryDomainInfo->Name.Length > 0))
    {
        Name = midl_user_allocate(PrimaryDomainInfo->Name.Length + sizeof(WCHAR));
        if (Name == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        wcscpy(Name, PrimaryDomainInfo->Name.Buffer);
    }

done:
    if (PrimaryDomainInfo)
    {
        if (PrimaryDomainInfo->Sid)
            LsaFreeMemory(PrimaryDomainInfo->Sid);

        LsaFreeMemory(PrimaryDomainInfo);
    }

    if (PolicyHandle)
        LsaClose(PolicyHandle);

    if (LSA_SUCCESS(Status))
    {
        *NameBuffer = Name;
        *BufferType = Type;
    }

    return LsaNtStatusToWinError(Status);
}

/* EOF */
