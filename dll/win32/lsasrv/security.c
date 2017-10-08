/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/security.c
 * PURPOSE:     LSA object security functions
 * COPYRIGHT:   Copyright 2012 Eric Kohl
 */

#include "lsasrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
LsapCreatePolicySd(PSECURITY_DESCRIPTOR *PolicySd,
                   PULONG PolicySdSize)
{
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    ULONG RelativeSdSize = 0;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID EveryoneSid = NULL;
    PSID LocalServiceSid = NULL;
    PSID NetworkServiceSid = NULL;
    PSID LocalSystemSid = NULL;
    PACL Dacl = NULL;
    ULONG DaclSize;
    NTSTATUS Status;

    if (PolicySd == NULL || PolicySdSize == NULL)
        return STATUS_INVALID_PARAMETER;

    *PolicySd = NULL;
    *PolicySdSize = 0;

    /* Initialize the SD */
    Status = RtlCreateSecurityDescriptor(&AbsoluteSd,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_ANONYMOUS_LOGON_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AnonymousSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&WorldSidAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_LOCAL_SERVICE_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalServiceSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_NETWORK_SERVICE_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &NetworkServiceSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate and initialize the DACL */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_DENIED_ACE)  - sizeof(ULONG) + RtlLengthSid(AnonymousSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(AdministratorsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(AnonymousSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(LocalServiceSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(NetworkServiceSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessDeniedAce(Dacl,
                                   ACL_REVISION,
                                   POLICY_LOOKUP_NAMES,
                                   AnonymousSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    POLICY_ALL_ACCESS | POLICY_NOTIFICATION,
                                    AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    POLICY_EXECUTE,
                                    EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                    AnonymousSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    POLICY_NOTIFICATION,
                                    LocalServiceSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    POLICY_NOTIFICATION,
                                    NetworkServiceSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetDaclSecurityDescriptor(&AbsoluteSd,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetGroupSecurityDescriptor(&AbsoluteSd,
                                           LocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetOwnerSecurityDescriptor(&AbsoluteSd,
                                           AdministratorsSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    RelativeSd = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 RelativeSdSize);
    if (RelativeSd == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    *PolicySd = RelativeSd;
    *PolicySdSize = RelativeSdSize;

done:
    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (AnonymousSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdministratorsSid);

    if (EveryoneSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, EveryoneSid);

    if (LocalServiceSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalServiceSid);

    if (NetworkServiceSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, NetworkServiceSid);

    if (LocalSystemSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);

    if (!NT_SUCCESS(Status))
    {
        if (RelativeSd != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
    }

    return Status;
}


NTSTATUS
LsapCreateAccountSd(PSECURITY_DESCRIPTOR *AccountSd,
                    PULONG AccountSdSize)
{
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    ULONG RelativeSdSize = 0;
    PSID AdministratorsSid = NULL;
    PSID EveryoneSid = NULL;
    PSID LocalSystemSid = NULL;
    PACL Dacl = NULL;
    ULONG DaclSize;
    NTSTATUS Status;

    if (AccountSd == NULL || AccountSdSize == NULL)
        return STATUS_INVALID_PARAMETER;

    *AccountSd = NULL;
    *AccountSdSize = 0;

    /* Initialize the SD */
    Status = RtlCreateSecurityDescriptor(&AbsoluteSd,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&WorldSidAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate and initialize the DACL */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(AdministratorsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(EveryoneSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    ACCOUNT_ALL_ACCESS,
                                    AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    ACCOUNT_EXECUTE,
                                    EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetDaclSecurityDescriptor(&AbsoluteSd,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetGroupSecurityDescriptor(&AbsoluteSd,
                                           LocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetOwnerSecurityDescriptor(&AbsoluteSd,
                                           AdministratorsSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    RelativeSd = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 RelativeSdSize);
    if (RelativeSd == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    *AccountSd = RelativeSd;
    *AccountSdSize = RelativeSdSize;

done:
    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (AdministratorsSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdministratorsSid);

    if (EveryoneSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, EveryoneSid);

    if (LocalSystemSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);

    if (!NT_SUCCESS(Status))
    {
        if (RelativeSd != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
    }

    return Status;
}


NTSTATUS
LsapCreateSecretSd(PSECURITY_DESCRIPTOR *SecretSd,
                   PULONG SecretSdSize)
{
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    ULONG RelativeSdSize = 0;
    PSID AdministratorsSid = NULL;
    PSID EveryoneSid = NULL;
    PSID LocalSystemSid = NULL;
    PACL Dacl = NULL;
    ULONG DaclSize;
    NTSTATUS Status;

    if (SecretSd == NULL || SecretSdSize == NULL)
        return STATUS_INVALID_PARAMETER;

    *SecretSd = NULL;
    *SecretSdSize = 0;

    /* Initialize the SD */
    Status = RtlCreateSecurityDescriptor(&AbsoluteSd,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&WorldSidAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate and initialize the DACL */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(AdministratorsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + RtlLengthSid(EveryoneSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SECRET_ALL_ACCESS,
                                    AdministratorsSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SECRET_EXECUTE,
                                    EveryoneSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetDaclSecurityDescriptor(&AbsoluteSd,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetGroupSecurityDescriptor(&AbsoluteSd,
                                           LocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlSetOwnerSecurityDescriptor(&AbsoluteSd,
                                           AdministratorsSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    RelativeSd = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 RelativeSdSize);
    if (RelativeSd == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlAbsoluteToSelfRelativeSD(&AbsoluteSd,
                                         RelativeSd,
                                         &RelativeSdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    *SecretSd = RelativeSd;
    *SecretSdSize = RelativeSdSize;

done:
    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (AdministratorsSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdministratorsSid);

    if (EveryoneSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, EveryoneSid);

    if (LocalSystemSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);

    if (!NT_SUCCESS(Status))
    {
        if (RelativeSd != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
    }

    return Status;
}

/* EOF */
