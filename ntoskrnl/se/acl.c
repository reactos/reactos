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

    return TRUE;
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

    if (AccessMode != KernelMode)
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
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        NewAcl = ExAllocatePoolWithTag(PoolType,
                                       AclSize,
                                       TAG_ACL);
        if (NewAcl != NULL)
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
                /* Free the ACL and return the exception code */
                ExFreePoolWithTag(NewAcl, TAG_ACL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else if (!CaptureIfKernel)
    {
        *CapturedAcl = InputAcl;
    }
    else
    {
        AclSize = InputAcl->AclSize;

        NewAcl = ExAllocatePoolWithTag(PoolType,
                                       AclSize,
                                       TAG_ACL);

        if (NewAcl != NULL)
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

    if (CapturedAcl != NULL &&
        (AccessMode != KernelMode ||
         (AccessMode == KernelMode && CaptureIfKernel)))
    {
        ExFreePoolWithTag(CapturedAcl, TAG_ACL);
    }
}

BOOLEAN
SepShouldPropagateAce(
    _In_ UCHAR AceFlags,
    _Out_ PUCHAR NewAceFlags,
    _In_ BOOLEAN IsInherited,
    _In_ BOOLEAN IsDirectoryObject)
{
    if (!IsInherited)
    {
        *NewAceFlags = AceFlags;
        return TRUE;
    }

    if (!IsDirectoryObject)
    {
        if (AceFlags & OBJECT_INHERIT_ACE)
        {
            *NewAceFlags = AceFlags & ~VALID_INHERIT_FLAGS;
            return TRUE;
        }
        return FALSE;
    }

    if (AceFlags & NO_PROPAGATE_INHERIT_ACE)
    {
        if (AceFlags & CONTAINER_INHERIT_ACE)
        {
            *NewAceFlags = AceFlags & ~VALID_INHERIT_FLAGS;
            return TRUE;
        }
        return FALSE;
    }

    if (AceFlags & CONTAINER_INHERIT_ACE)
    {
        *NewAceFlags = CONTAINER_INHERIT_ACE | (AceFlags & OBJECT_INHERIT_ACE) | (AceFlags & ~VALID_INHERIT_FLAGS);
        return TRUE;
    }

    if (AceFlags & OBJECT_INHERIT_ACE)
    {
        *NewAceFlags = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | (AceFlags & ~VALID_INHERIT_FLAGS);
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
SepPropagateAcl(
    _Out_writes_bytes_opt_(DaclLength) PACL AclDest,
    _Inout_ PULONG AclLength,
    _In_reads_bytes_(AclSource->AclSize) PACL AclSource,
    _In_ PSID Owner,
    _In_ PSID Group,
    _In_ BOOLEAN IsInherited,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    ACCESS_MASK Mask;
    PACCESS_ALLOWED_ACE AceSource;
    PACCESS_ALLOWED_ACE AceDest;
    PUCHAR CurrentDest;
    PUCHAR CurrentSource;
    ULONG i;
    ULONG Written;
    UCHAR AceFlags;
    USHORT AceSize;
    USHORT AceCount = 0;
    PSID Sid;
    BOOLEAN WriteTwoAces;

    if (AclSource->AclRevision != ACL_REVISION)
    {
        NT_ASSERT(AclSource->AclRevision == ACL_REVISION);
        return STATUS_UNKNOWN_REVISION;
    }

    NT_ASSERT(AclSource->AclSize % sizeof(ULONG) == 0);
    NT_ASSERT(AclSource->Sbz1 == 0);
    NT_ASSERT(AclSource->Sbz2 == 0);

    Written = 0;
    if (*AclLength >= Written + sizeof(ACL))
    {
        RtlCopyMemory(AclDest,
                      AclSource,
                      sizeof(ACL));
    }
    Written += sizeof(ACL);

    CurrentDest = (PUCHAR)(AclDest + 1);
    CurrentSource = (PUCHAR)(AclSource + 1);
    for (i = 0; i < AclSource->AceCount; i++)
    {
        NT_ASSERT((ULONG_PTR)CurrentDest % sizeof(ULONG) == 0);
        NT_ASSERT((ULONG_PTR)CurrentSource % sizeof(ULONG) == 0);
        AceDest = (PACCESS_ALLOWED_ACE)CurrentDest;
        AceSource = (PACCESS_ALLOWED_ACE)CurrentSource;

        /* These all have the same structure */
        NT_ASSERT(AceSource->Header.AceType == ACCESS_ALLOWED_ACE_TYPE ||
                  AceSource->Header.AceType == ACCESS_DENIED_ACE_TYPE ||
                  AceSource->Header.AceType == SYSTEM_AUDIT_ACE_TYPE);

        NT_ASSERT(AceSource->Header.AceSize % sizeof(ULONG) == 0);
        NT_ASSERT(AceSource->Header.AceSize >= sizeof(*AceSource));
        if (!SepShouldPropagateAce(AceSource->Header.AceFlags,
                                   &AceFlags,
                                   IsInherited,
                                   IsDirectoryObject))
        {
            CurrentSource += AceSource->Header.AceSize;
            continue;
        }

        /* FIXME: filter out duplicate ACEs */
        AceSize = AceSource->Header.AceSize;
        Mask = AceSource->Mask;
        Sid = (PSID)&AceSource->SidStart;
        NT_ASSERT(AceSize >= FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + RtlLengthSid(Sid));

        WriteTwoAces = FALSE;
        /* Map effective ACE to specific rights */
        if (!(AceFlags & INHERIT_ONLY_ACE))
        {
            RtlMapGenericMask(&Mask, GenericMapping);
            Mask &= GenericMapping->GenericAll;

            if (IsInherited)
            {
                if (RtlEqualSid(Sid, SeCreatorOwnerSid))
                    Sid = Owner;
                else if (RtlEqualSid(Sid, SeCreatorGroupSid))
                    Sid = Group;
                AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + RtlLengthSid(Sid);

                /*
                 * A generic container ACE becomes two ACEs:
                 * - a specific effective ACE with no inheritance flags
                 * - an inherit-only ACE that keeps the generic rights
                 */
                if (IsDirectoryObject &&
                    (AceFlags & CONTAINER_INHERIT_ACE) &&
                    (Mask != AceSource->Mask || Sid != (PSID)&AceSource->SidStart))
                {
                    WriteTwoAces = TRUE;
                }
            }
        }

        while (1)
        {
            if (*AclLength >= Written + AceSize)
            {
                AceDest->Header.AceType = AceSource->Header.AceType;
                AceDest->Header.AceFlags = WriteTwoAces ? AceFlags & ~VALID_INHERIT_FLAGS
                                                        : AceFlags;
                AceDest->Header.AceSize = AceSize;
                AceDest->Mask = Mask;
                RtlCopySid(AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart),
                           (PSID)&AceDest->SidStart,
                           Sid);
            }
            Written += AceSize;

            AceCount++;
            CurrentDest += AceSize;

            if (!WriteTwoAces)
                break;

            /* Second ACE keeps all the generics from the source ACE */
            WriteTwoAces = FALSE;
            AceDest = (PACCESS_ALLOWED_ACE)CurrentDest;
            AceSize = AceSource->Header.AceSize;
            Mask = AceSource->Mask;
            Sid = (PSID)&AceSource->SidStart;
            AceFlags |= INHERIT_ONLY_ACE;
        }

        CurrentSource += AceSource->Header.AceSize;
    }

    if (*AclLength >= sizeof(ACL))
    {
        AclDest->AceCount = AceCount;
        AclDest->AclSize = Written;
    }

    if (Written > *AclLength)
    {
        *AclLength = Written;
        return STATUS_BUFFER_TOO_SMALL;
    }
    *AclLength = Written;
    return STATUS_SUCCESS;
}

PACL
SepSelectAcl(
    _In_opt_ PACL ExplicitAcl,
    _In_ BOOLEAN ExplicitPresent,
    _In_ BOOLEAN ExplicitDefaulted,
    _In_opt_ PACL ParentAcl,
    _In_opt_ PACL DefaultAcl,
    _Out_ PULONG AclLength,
    _In_ PSID Owner,
    _In_ PSID Group,
    _Out_ PBOOLEAN AclPresent,
    _Out_ PBOOLEAN IsInherited,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PACL Acl;
    NTSTATUS Status;

    *AclPresent = TRUE;
    if (ExplicitPresent && !ExplicitDefaulted)
    {
        Acl = ExplicitAcl;
    }
    else
    {
        if (ParentAcl)
        {
            *IsInherited = TRUE;
            *AclLength = 0;
            Status = SepPropagateAcl(NULL,
                                     AclLength,
                                     ParentAcl,
                                     Owner,
                                     Group,
                                     *IsInherited,
                                     IsDirectoryObject,
                                     GenericMapping);
            NT_ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

            /* Use the parent ACL only if it's not empty */
            if (*AclLength != sizeof(ACL))
                return ParentAcl;
        }

        if (ExplicitPresent)
        {
            Acl = ExplicitAcl;
        }
        else if (DefaultAcl)
        {
            Acl = DefaultAcl;
        }
        else
        {
            *AclPresent = FALSE;
            Acl = NULL;
        }
    }

    *IsInherited = FALSE;
    *AclLength = 0;
    if (Acl)
    {
        /* Get the length */
        Status = SepPropagateAcl(NULL,
                                 AclLength,
                                 Acl,
                                 Owner,
                                 Group,
                                 *IsInherited,
                                 IsDirectoryObject,
                                 GenericMapping);
        NT_ASSERT(Status == STATUS_BUFFER_TOO_SMALL);
    }
    return Acl;
}

/* EOF */
