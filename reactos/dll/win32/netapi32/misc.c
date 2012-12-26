/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/misc.c
 * PURPOSE:         Helper functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"


WINE_DEFAULT_DEBUG_CHANNEL(netapi32);


/* GLOBALS *******************************************************************/

static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};


/* FUNCTIONS *****************************************************************/

NTSTATUS
GetAccountDomainSid(IN PUNICODE_STRING ServerName,
                    OUT PSID *AccountDomainSid)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    Status = LsaOpenPolicy(ServerName,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaOpenPolicy failed (Status %08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)&AccountDomainInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaQueryInformationPolicy failed (Status %08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(AccountDomainInfo->DomainSid);

    *AccountDomainSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*AccountDomainSid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*AccountDomainSid, AccountDomainInfo->DomainSid, Length);

done:
    if (AccountDomainInfo != NULL)
        LsaFreeMemory(AccountDomainInfo);

    LsaClose(PolicyHandle);

    return Status;
}


NTSTATUS
GetBuiltinDomainSid(OUT PSID *BuiltinDomainSid)
{
    PSID Sid = NULL;
    PULONG Ptr;
    NTSTATUS Status = STATUS_SUCCESS;

    *BuiltinDomainSid = NULL;

    Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                          0,
                          RtlLengthRequiredSid(1));
    if (Sid == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = RtlInitializeSid(Sid,
                              &NtAuthority,
                              1);
    if (!NT_SUCCESS(Status))
        goto done;

    Ptr = RtlSubAuthoritySid(Sid, 0);
    *Ptr = SECURITY_BUILTIN_DOMAIN_RID;

    *BuiltinDomainSid = Sid;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Sid != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, Sid);
    }

    return Status;
}


NTSTATUS
OpenAccountDomain(IN SAM_HANDLE ServerHandle,
                  IN PUNICODE_STRING ServerName,
                  IN ULONG DesiredAccess,
                  OUT PSAM_HANDLE DomainHandle)
{
    PSID DomainSid = NULL;
    NTSTATUS Status;

    Status = GetAccountDomainSid(ServerName,
                                 &DomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
        return Status;
    }

    Status = SamOpenDomain(ServerHandle,
                           DesiredAccess,
                           DomainSid,
                           DomainHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenDomain failed (Status %08lx)\n", Status);
    }

    return Status;
}


NTSTATUS
OpenBuiltinDomain(IN SAM_HANDLE ServerHandle,
                  IN ULONG DesiredAccess,
                  OUT PSAM_HANDLE DomainHandle)
{
    PSID DomainSid = NULL;
    NTSTATUS Status;

    Status = GetBuiltinDomainSid(&DomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetBuiltinDomainSid failed (Status %08lx)\n", Status);
        return Status;
    }

    Status = SamOpenDomain(ServerHandle,
                           DesiredAccess,
                           DomainSid,
                           DomainHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

    if (!NT_SUCCESS(Status))
    {
        ERR("SamOpenDomain failed (Status %08lx)\n", Status);
    }

    return Status;
}

/* EOF */
