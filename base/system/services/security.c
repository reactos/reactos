/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/security.c
 * PURPOSE:     Security functions
 * COPYRIGHT:   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

static PSID pNullSid = NULL;
static PSID pWorldSid = NULL;
static PSID pLocalSystemSid = NULL;
static PSID pAuthenticatedUserSid = NULL;
static PSID pAliasAdminsSid = NULL;

static PACL pDefaultDacl = NULL;
static PACL pDefaultSacl = NULL;
static PACL pPipeDacl = NULL;

static PSECURITY_DESCRIPTOR pDefaultSD = NULL;
PSECURITY_DESCRIPTOR pPipeSD = NULL;


/* FUNCTIONS ****************************************************************/

static
VOID
ScmFreeSids(VOID)
{
    if (pNullSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pNullSid);

    if (pWorldSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pWorldSid);

    if (pLocalSystemSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pLocalSystemSid);

    if (pAuthenticatedUserSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pAuthenticatedUserSid);

    if (pAliasAdminsSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pAliasAdminsSid);
}


static
DWORD
ScmCreateSids(VOID)
{
    SID_IDENTIFIER_AUTHORITY NullAuthority = {SECURITY_NULL_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    PULONG pSubAuthority;
    ULONG ulLength1 = RtlLengthRequiredSid(1);
    ULONG ulLength2 = RtlLengthRequiredSid(2);

    /* Create the Null SID */
    pNullSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulLength1);
    if (pNullSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    RtlInitializeSid(pNullSid, &NullAuthority, 1);
    pSubAuthority = RtlSubAuthoritySid(pNullSid, 0);
    *pSubAuthority = SECURITY_NULL_RID;

    /* Create the World SID */
    pWorldSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulLength1);
    if (pWorldSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    RtlInitializeSid(pWorldSid, &WorldAuthority, 1);
    pSubAuthority = RtlSubAuthoritySid(pWorldSid, 0);
    *pSubAuthority = SECURITY_WORLD_RID;

    /* Create the LocalSystem SID */
    pLocalSystemSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulLength1);
    if (pLocalSystemSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    RtlInitializeSid(pLocalSystemSid, &NtAuthority, 1);
    pSubAuthority = RtlSubAuthoritySid(pLocalSystemSid, 0);
    *pSubAuthority = SECURITY_LOCAL_SYSTEM_RID;

    /* Create the AuthenticatedUser SID */
    pAuthenticatedUserSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulLength1);
    if (pAuthenticatedUserSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    RtlInitializeSid(pAuthenticatedUserSid, &NtAuthority, 1);
    pSubAuthority = RtlSubAuthoritySid(pAuthenticatedUserSid, 0);
    *pSubAuthority = SECURITY_AUTHENTICATED_USER_RID;

    /* Create the AliasAdmins SID */
    pAliasAdminsSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulLength2);
    if (pAliasAdminsSid == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    RtlInitializeSid(pAliasAdminsSid, &NtAuthority, 2);
    pSubAuthority = RtlSubAuthoritySid(pAliasAdminsSid, 0);
    *pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    pSubAuthority = RtlSubAuthoritySid(pAliasAdminsSid, 1);
    *pSubAuthority = DOMAIN_ALIAS_RID_ADMINS;

    return ERROR_SUCCESS;
}


static
DWORD
ScmCreateAcls(VOID)
{
    ULONG ulLength;

    /* Create DACL */
    ulLength = sizeof(ACL) +
               (sizeof(ACE) + RtlLengthSid(pLocalSystemSid)) +
               (sizeof(ACE) + RtlLengthSid(pAliasAdminsSid)) +
               (sizeof(ACE) + RtlLengthSid(pAuthenticatedUserSid));

    pDefaultDacl = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
    if (pDefaultDacl == NULL)
        return ERROR_OUTOFMEMORY;

    RtlCreateAcl(pDefaultDacl, ulLength, ACL_REVISION);

    RtlAddAccessAllowedAce(pDefaultDacl,
                           ACL_REVISION,
                           READ_CONTROL | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE |
                           SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS |
                           SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL,
                           pLocalSystemSid);

    RtlAddAccessAllowedAce(pDefaultDacl,
                           ACL_REVISION,
                           SERVICE_ALL_ACCESS,
                           pAliasAdminsSid);

    RtlAddAccessAllowedAce(pDefaultDacl,
                           ACL_REVISION,
                           READ_CONTROL | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE |
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_USER_DEFINED_CONTROL,
                           pAuthenticatedUserSid);

    /* Create SACL */
    ulLength = sizeof(ACL) +
               (sizeof(ACE) + RtlLengthSid(pNullSid));

    pDefaultSacl = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
    if (pDefaultSacl == NULL)
        return ERROR_OUTOFMEMORY;

    RtlCreateAcl(pDefaultSacl, ulLength, ACL_REVISION);

    RtlAddAuditAccessAce(pDefaultSacl,
                         ACL_REVISION,
                         SERVICE_ALL_ACCESS,
                         pNullSid,
                         FALSE,
                         TRUE);

    /* Create the pipe DACL */
    ulLength = sizeof(ACL) +
               (sizeof(ACE) + RtlLengthSid(pWorldSid));

    pPipeDacl = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
    if (pPipeDacl == NULL)
        return ERROR_OUTOFMEMORY;

    RtlCreateAcl(pPipeDacl, ulLength, ACL_REVISION);

    RtlAddAccessAllowedAce(pPipeDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           pWorldSid);

    return ERROR_SUCCESS;
}


static
VOID
ScmFreeAcls(VOID)
{
    if (pDefaultDacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pDefaultDacl);

    if (pDefaultSacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pDefaultSacl);

    if (pPipeDacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pPipeDacl);
}


static
DWORD
ScmCreateDefaultSD(VOID)
{
    NTSTATUS Status;

    /* Create the absolute security descriptor */
    pDefaultSD = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (pDefaultSD == NULL)
        return ERROR_OUTOFMEMORY;

    DPRINT("pDefaultSD %p\n", pDefaultSD);

    Status = RtlCreateSecurityDescriptor(pDefaultSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetOwnerSecurityDescriptor(pDefaultSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetGroupSecurityDescriptor(pDefaultSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetDaclSecurityDescriptor(pDefaultSD,
                                          TRUE,
                                          pDefaultDacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetSaclSecurityDescriptor(pDefaultSD,
                                          TRUE,
                                          pDefaultSacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    return ERROR_SUCCESS;
}


static
VOID
ScmFreeDefaultSD(VOID)
{
    if (pDefaultSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pDefaultSD);
}


static
DWORD
ScmCreatePipeSD(VOID)
{
    NTSTATUS Status;

    /* Create the absolute security descriptor */
    pPipeSD = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (pPipeSD == NULL)
        return ERROR_OUTOFMEMORY;

    DPRINT("pPipeSD %p\n", pDefaultSD);

    Status = RtlCreateSecurityDescriptor(pPipeSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetOwnerSecurityDescriptor(pPipeSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetGroupSecurityDescriptor(pPipeSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    Status = RtlSetDaclSecurityDescriptor(pPipeSD,
                                          TRUE,
                                          pPipeDacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    return ERROR_SUCCESS;
}


static
VOID
ScmFreePipeSD(VOID)
{
    if (pPipeSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pPipeSD);
}


DWORD
ScmCreateDefaultServiceSD(
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR pRelativeSD = NULL;
    DWORD dwBufferLength = 0;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Convert the absolute SD to a self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(pDefaultSD,
                                         NULL,
                                         &dwBufferLength);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    DPRINT("BufferLength %lu\n", dwBufferLength);

    pRelativeSD = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  dwBufferLength);
    if (pRelativeSD == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }
    DPRINT("pRelativeSD %p\n", pRelativeSD);

    Status = RtlAbsoluteToSelfRelativeSD(pDefaultSD,
                                         pRelativeSD,
                                         &dwBufferLength);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    *ppSecurityDescriptor = pRelativeSD;

done:
    if (dwError != ERROR_SUCCESS)
    {
        if (pRelativeSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pRelativeSD);
    }

    return dwError;
}


DWORD
ScmInitializeSecurity(VOID)
{
    DWORD dwError;

    dwError = ScmCreateSids();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = ScmCreateAcls();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = ScmCreateDefaultSD();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = ScmCreatePipeSD();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    return ERROR_SUCCESS;
}


VOID
ScmShutdownSecurity(VOID)
{
    ScmFreePipeSD();
    ScmFreeDefaultSD();
    ScmFreeAcls();
    ScmFreeSids();
}

/* EOF */
