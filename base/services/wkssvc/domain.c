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

/* EOF */
