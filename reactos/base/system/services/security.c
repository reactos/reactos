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

PSECURITY_DESCRIPTOR pDefaultServiceSD = NULL;

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


static
DWORD
ScmCreateDefaultServiceSD(VOID)
{
    PSECURITY_DESCRIPTOR pServiceSD = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    ULONG ulLength;
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


    pServiceSD = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (pServiceSD == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }
DPRINT1("pServiceSD %p\n", pServiceSD);

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


    pDefaultServiceSD = pServiceSD;
DPRINT1("pDefaultServiceSD %p\n", pDefaultServiceSD);

done:
    if (dwError != ERROR_SUCCESS)
    {
        if (pDacl != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pDacl);

        if (pSacl != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pSacl);

        if (pServiceSD != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pServiceSD);
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

    dwError = ScmCreateDefaultServiceSD();
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
