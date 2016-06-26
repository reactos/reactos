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
static PSID pLocalSystemSid = NULL;
static PSID pAuthenticatedUserSid = NULL;
static PSID pAliasAdminsSid = NULL;


/* FUNCTIONS ****************************************************************/

static
VOID
ScmFreeSids(VOID)
{
    if (pNullSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pNullSid);

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


DWORD
ScmCreateDefaultServiceSD(
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR pServiceSD = NULL;
    PSECURITY_DESCRIPTOR pRelativeSD = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    ULONG ulLength;
    DWORD dwBufferLength = 0;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Create DACL */
    ulLength = sizeof(ACL) +
               (sizeof(ACE) + RtlLengthSid(pLocalSystemSid)) +
               (sizeof(ACE) + RtlLengthSid(pAliasAdminsSid)) +
               (sizeof(ACE) + RtlLengthSid(pAuthenticatedUserSid));

    pDacl = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
    if (pDacl == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    RtlCreateAcl(pDacl, ulLength, ACL_REVISION);

    RtlAddAccessAllowedAce(pDacl,
                           ACL_REVISION,
                           READ_CONTROL | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE |
                           SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS |
                           SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL,
                           pLocalSystemSid);

    RtlAddAccessAllowedAce(pDacl,
                           ACL_REVISION,
                           SERVICE_ALL_ACCESS,
                           pAliasAdminsSid);

    RtlAddAccessAllowedAce(pDacl,
                           ACL_REVISION,
                           READ_CONTROL | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE |
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_USER_DEFINED_CONTROL,
                           pAuthenticatedUserSid);

    /* Create SACL */
    ulLength = sizeof(ACL) +
               (sizeof(ACE) + RtlLengthSid(pNullSid));

    pSacl = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
    if (pSacl == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    RtlCreateAcl(pSacl, ulLength, ACL_REVISION);

    RtlAddAuditAccessAce(pSacl,
                         ACL_REVISION,
                         SERVICE_ALL_ACCESS,
                         pNullSid,
                         FALSE,
                         TRUE);

    /* Create the absolute security descriptor */
    pServiceSD = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (pServiceSD == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }
    DPRINT("pServiceSD %p\n", pServiceSD);

    Status = RtlCreateSecurityDescriptor(pServiceSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    Status = RtlSetOwnerSecurityDescriptor(pServiceSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    Status = RtlSetGroupSecurityDescriptor(pServiceSD,
                                           pLocalSystemSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    Status = RtlSetDaclSecurityDescriptor(pServiceSD,
                                          TRUE,
                                          pDacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    Status = RtlSetSaclSecurityDescriptor(pServiceSD,
                                          TRUE,
                                          pSacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto done;
    }

    /* Convert the absolute SD to a self-relative SD */
    Status = RtlAbsoluteToSelfRelativeSD(pServiceSD,
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

    Status = RtlAbsoluteToSelfRelativeSD(pServiceSD,
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

    if (pServiceSD != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pServiceSD);

    if (pSacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pSacl);

    if (pDacl != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pDacl);

    return dwError;
}


DWORD
ScmInitializeSecurity(VOID)
{
    DWORD dwError;

    dwError = ScmCreateSids();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    return ERROR_SUCCESS;
}


VOID
ScmShutdownSecurity(VOID)
{
    ScmFreeSids();
}

/* EOF */
