/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/security.c
 * PURPOSE:         Security descriptor functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "samsrv.h"

/* GLOBALS *****************************************************************/

static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};


/* FUNCTIONS ***************************************************************/

NTSTATUS
SampCreateServerSD(OUT PSECURITY_DESCRIPTOR *ServerSd,
                   OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;


    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SAM_SERVER_ALL_ACCESS,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  SAM_SERVER_CREATE_DOMAIN | SAM_SERVER_INITIALIZE |
                                  SAM_SERVER_SHUTDOWN,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *ServerSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}


NTSTATUS
SampCreateBuiltinDomainSD(OUT PSECURITY_DESCRIPTOR *ServerSd,
                          OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;


    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DOMAIN_READ | DOMAIN_EXECUTE,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SAM_SERVER_ALL_ACCESS,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  SAM_SERVER_CREATE_DOMAIN | SAM_SERVER_INITIALIZE |
                                  SAM_SERVER_SHUTDOWN,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *ServerSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}


NTSTATUS
SampCreateAccountDomainSD(OUT PSECURITY_DESCRIPTOR *ServerSd,
                          OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID UsersSid = NULL;
    PSID GuestsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Users SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_USERS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &UsersSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Guests SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_GUESTS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &GuestsSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;


    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               4 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid) +
               RtlLengthSid(UsersSid) +
               RtlLengthSid(GuestsSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DOMAIN_READ | DOMAIN_EXECUTE,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DOMAIN_READ | DOMAIN_EXECUTE,
                                    UsersSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DOMAIN_ALL_ACCESS & ~DOMAIN_CREATE_GROUP,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DOMAIN_READ | DOMAIN_EXECUTE | DOMAIN_CREATE_USER | DOMAIN_CREATE_ALIAS,
                                    GuestsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  SAM_SERVER_CREATE_DOMAIN | SAM_SERVER_INITIALIZE |
                                  SAM_SERVER_SHUTDOWN,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *ServerSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}


NTSTATUS
SampCreateAliasSD(OUT PSECURITY_DESCRIPTOR *AliasSd,
                  OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID AccountOperatorsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Account Operators SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AccountOperatorsSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               3 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid) +
               RtlLengthSid(AccountOperatorsSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    READ_CONTROL | ALIAS_READ_INFORMATION | ALIAS_LIST_MEMBERS,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    ALIAS_ALL_ACCESS,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    ALIAS_ALL_ACCESS,
                                    AccountOperatorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  ALIAS_WRITE_ACCOUNT | ALIAS_REMOVE_MEMBER |
                                  ALIAS_ADD_MEMBER,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *AliasSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}


NTSTATUS
SampCreateGroupSD(OUT PSECURITY_DESCRIPTOR *GroupSd,
                  OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID AccountOperatorsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Account Operators SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AccountOperatorsSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               3 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid) +
               RtlLengthSid(AccountOperatorsSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    READ_CONTROL | GROUP_LIST_MEMBERS | GROUP_READ_INFORMATION,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    GROUP_ALL_ACCESS,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    GROUP_ALL_ACCESS,
                                    AccountOperatorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  GROUP_REMOVE_MEMBER | GROUP_ADD_MEMBER |
                                  GROUP_WRITE_ACCOUNT,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *GroupSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}


NTSTATUS
SampCreateUserSD(IN PSID UserSid,
                 OUT PSECURITY_DESCRIPTOR *UserSd,
                 OUT PULONG Size)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSID EveryoneSid = NULL;
    PSID AnonymousSid = NULL;
    PSID AdministratorsSid = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG DaclSize;
    ULONG SaclSize;
    ULONG RelSDSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;


    /* Create the Everyone SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Anonymous SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Administrators SID */
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
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Allocate a buffer for the absolute SD */
    AbsSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SECURITY_DESCRIPTOR));
    if (AbsSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Create the absolute SD */
    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) +
               3 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AdministratorsSid) +
               RtlLengthSid(UserSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    READ_CONTROL | USER_READ_GROUP_INFORMATION | USER_LIST_GROUPS |
                                    USER_CHANGE_PASSWORD | USER_READ_ACCOUNT | USER_READ_LOGON |
                                    USER_READ_PREFERENCES | USER_READ_GENERAL,
                                    EveryoneSid);
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    USER_ALL_ACCESS,
                                    AdministratorsSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    READ_CONTROL | USER_CHANGE_PASSWORD | USER_WRITE_PREFERENCES,
                                    UserSid);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* allocate and create the SACL */
    SaclSize = sizeof(ACL) +
               2 * sizeof(ACE) +
               RtlLengthSid(EveryoneSid) +
               RtlLengthSid(AnonymousSid);

    Sacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Sacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    Status = RtlCreateAcl(Sacl,
                          SaclSize,
                          ACL_REVISION);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  ACCESS_SYSTEM_SECURITY | WRITE_DAC | DELETE |
                                  USER_CHANGE_PASSWORD | USER_WRITE_PREFERENCES,
                                  EveryoneSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlAddAuditAccessAce(Sacl,
                                  ACL_REVISION,
                                  STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                  AnonymousSid,
                                  TRUE,
                                  TRUE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SACL */
    Status = RtlSetSaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Sacl,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the group SID */
    Status = RtlSetGroupSecurityDescriptor(AbsSD,
                                           AdministratorsSid,
                                           FALSE);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the reqired buffer size for the self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        goto done;

    /* Allocate a buffer for the self-relative SD */
    RelSD = RtlAllocateHeap(RtlGetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            RelSDSize);
    if (RelSD == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    /* Convert the absolute SD to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
    ASSERT(Status == STATUS_SUCCESS);
        goto done;
    }

    *UserSd = RelSD;
    *Size = RelSDSize;

done:
    if (!NT_SUCCESS(Status))
    {
        if (RelSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelSD);
    }

    if (EveryoneSid != NULL)
        RtlFreeSid(EveryoneSid);

    if (AnonymousSid != NULL)
        RtlFreeSid(AnonymousSid);

    if (AdministratorsSid != NULL)
        RtlFreeSid(AdministratorsSid);

    if (Dacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);

    if (Sacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sacl);

    if (AbsSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AbsSD);

    return Status;
}

/* EOF */
