/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/acl.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitDACLs)
#endif

/* GLOBALS ********************************************************************/

PACL SePublicDefaultDacl = NULL;
PACL SeSystemDefaultDacl = NULL;
PACL SePublicDefaultUnrestrictedDacl = NULL;
PACL SePublicOpenDacl = NULL;
PACL SePublicOpenUnrestrictedDacl = NULL;
PACL SeUnrestrictedDacl = NULL;

/* FUNCTIONS ******************************************************************/

BOOLEAN
INIT_FUNCTION
NTAPI
SepInitDACLs(VOID)
{
    ULONG AclLength;
    
    /* create PublicDefaultDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid));
    
    SePublicDefaultDacl = ExAllocatePoolWithTag(PagedPool,
                                                AclLength,
                                                TAG_ACL);
    if (SePublicDefaultDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SePublicDefaultDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SePublicDefaultDacl,
                           ACL_REVISION,
                           GENERIC_EXECUTE,
                           SeWorldSid);
    
    RtlAddAccessAllowedAce(SePublicDefaultDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);
    
    
    /* create PublicDefaultUnrestrictedDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
    (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid)) +
    (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));
    
    SePublicDefaultUnrestrictedDacl = ExAllocatePoolWithTag(PagedPool,
                                                            AclLength,
                                                            TAG_ACL);
    if (SePublicDefaultUnrestrictedDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SePublicDefaultUnrestrictedDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_EXECUTE,
                           SeWorldSid);
    
    RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);
    
    RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeAliasAdminsSid);
    
    RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL,
                           SeRestrictedCodeSid);
    
    /* create PublicOpenDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
    (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));
    
    SePublicOpenDacl = ExAllocatePoolWithTag(PagedPool,
                                             AclLength,
                                             TAG_ACL);
    if (SePublicOpenDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SePublicOpenDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SePublicOpenDacl,
                           ACL_REVISION,
                           GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                           SeWorldSid);
    
    RtlAddAccessAllowedAce(SePublicOpenDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);
    
    RtlAddAccessAllowedAce(SePublicOpenDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeAliasAdminsSid);
    
    /* create PublicOpenUnrestrictedDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
    (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid)) +
    (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));
    
    SePublicOpenUnrestrictedDacl = ExAllocatePoolWithTag(PagedPool,
                                                         AclLength,
                                                         TAG_ACL);
    if (SePublicOpenUnrestrictedDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SePublicOpenUnrestrictedDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeWorldSid);
    
    RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);
    
    RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeAliasAdminsSid);
    
    RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_READ | GENERIC_EXECUTE,
                           SeRestrictedCodeSid);
    
    /* create SystemDefaultDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
    (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));
    
    SeSystemDefaultDacl = ExAllocatePoolWithTag(PagedPool,
                                                AclLength,
                                                TAG_ACL);
    if (SeSystemDefaultDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SeSystemDefaultDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SeSystemDefaultDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeLocalSystemSid);
    
    RtlAddAccessAllowedAce(SeSystemDefaultDacl,
                           ACL_REVISION,
                           GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL,
                           SeAliasAdminsSid);
    
    /* create UnrestrictedDacl */
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
    (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));
    
    SeUnrestrictedDacl = ExAllocatePoolWithTag(PagedPool,
                                               AclLength,
                                               TAG_ACL);
    if (SeUnrestrictedDacl == NULL)
        return FALSE;
    
    RtlCreateAcl(SeUnrestrictedDacl,
                 AclLength,
                 ACL_REVISION);
    
    RtlAddAccessAllowedAce(SeUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           SeWorldSid);
    
    RtlAddAccessAllowedAce(SeUnrestrictedDacl,
                           ACL_REVISION,
                           GENERIC_READ | GENERIC_EXECUTE,
                           SeRestrictedCodeSid);
    
    return(TRUE);
}

NTSTATUS NTAPI
SepCreateImpersonationTokenDacl(PTOKEN Token,
                                PTOKEN PrimaryToken,
                                PACL *Dacl)
{
    ULONG AclLength;
    PVOID TokenDacl;
    
    PAGED_CODE();
    
    AclLength = sizeof(ACL) +
    (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid)) +
    (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid)) +
    (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
    (sizeof(ACE) + RtlLengthSid(Token->UserAndGroups->Sid)) +
    (sizeof(ACE) + RtlLengthSid(PrimaryToken->UserAndGroups->Sid));
    
    TokenDacl = ExAllocatePoolWithTag(PagedPool, AclLength, TAG_ACL);
    if (TokenDacl == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCreateAcl(TokenDacl, AclLength, ACL_REVISION);
    RtlAddAccessAllowedAce(TokenDacl, ACL_REVISION, GENERIC_ALL,
                           Token->UserAndGroups->Sid);
    RtlAddAccessAllowedAce(TokenDacl, ACL_REVISION, GENERIC_ALL,
                           PrimaryToken->UserAndGroups->Sid);
    RtlAddAccessAllowedAce(TokenDacl, ACL_REVISION, GENERIC_ALL,
                           SeAliasAdminsSid);
    RtlAddAccessAllowedAce(TokenDacl, ACL_REVISION, GENERIC_ALL,
                           SeLocalSystemSid);
    
    /* FIXME */
#if 0
    if (Token->RestrictedSids != NULL || PrimaryToken->RestrictedSids != NULL)
    {
        RtlAddAccessAllowedAce(TokenDacl, ACL_REVISION, GENERIC_ALL,
                               SeRestrictedCodeSid);
    }
#endif
    
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SepCaptureAcl(IN PACL InputAcl,
              IN KPROCESSOR_MODE AccessMode,
              IN POOL_TYPE PoolType,
              IN BOOLEAN CaptureIfKernel,
              OUT PACL *CapturedAcl)
{
    PACL NewAcl;
    ULONG AclSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    if(AccessMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(InputAcl,
                         sizeof(ACL),
                         sizeof(ULONG));
            AclSize = InputAcl->AclSize;
            ProbeForRead(InputAcl,
                         AclSize,
                         sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if(NT_SUCCESS(Status))
        {
            NewAcl = ExAllocatePool(PoolType,
                                    AclSize);
            if(NewAcl != NULL)
            {
                _SEH2_TRY
                {
                    RtlCopyMemory(NewAcl,
                                  InputAcl,
                                  AclSize);
                    
                    *CapturedAcl = NewAcl;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    ExFreePool(NewAcl);
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    else if(!CaptureIfKernel)
    {
        *CapturedAcl = InputAcl;
    }
    else
    {
        AclSize = InputAcl->AclSize;
        
        NewAcl = ExAllocatePool(PoolType,
                                AclSize);
        
        if(NewAcl != NULL)
        {
            RtlCopyMemory(NewAcl,
                          InputAcl,
                          AclSize);
            
            *CapturedAcl = NewAcl;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    return Status;
}

VOID
NTAPI
SepReleaseAcl(IN PACL CapturedAcl,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();
    
    if(CapturedAcl != NULL &&
       (AccessMode != KernelMode ||
        (AccessMode == KernelMode && CaptureIfKernel)))
    {
        ExFreePool(CapturedAcl);
    }
}

/* EOF */
