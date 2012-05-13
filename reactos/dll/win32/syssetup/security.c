/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              dll/win32/syssetup/security.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS
SetAccountDomain(LPCWSTR DomainName,
                 PSID DomainSid)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO OrigInfo = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO Info;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_TRUST_ADMIN,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaOpenPolicy failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)&OrigInfo);
    if (Status == STATUS_SUCCESS && OrigInfo != NULL)
    {
        if (DomainName == NULL)
        {
            Info.DomainName.Buffer = OrigInfo->DomainName.Buffer;
            Info.DomainName.Length = OrigInfo->DomainName.Length;
            Info.DomainName.MaximumLength = OrigInfo->DomainName.MaximumLength;
        }
        else
        {
            Info.DomainName.Buffer = (LPWSTR)DomainName;
            Info.DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
            Info.DomainName.MaximumLength = Info.DomainName.Length + sizeof(WCHAR);
        }

        if (DomainSid == NULL)
            Info.DomainSid = OrigInfo->DomainSid;
        else
            Info.DomainSid = DomainSid;
    }
    else
    {
        Info.DomainName.Buffer = (LPWSTR)DomainName;
        Info.DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
        Info.DomainName.MaximumLength = Info.DomainName.Length + sizeof(WCHAR);
        Info.DomainSid = DomainSid;
    }

    Status = LsaSetInformationPolicy(PolicyHandle,
                                     PolicyAccountDomainInformation,
                                     (PVOID)&Info);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaSetInformationPolicy failed (Status: 0x%08lx)\n", Status);
    }

    if (OrigInfo != NULL)
        LsaFreeMemory(OrigInfo);

    LsaClose(PolicyHandle);

    return Status;
}
