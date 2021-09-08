/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security descriptors (SDs) implementation support
 * COPYRIGHT:       Copyright David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PSECURITY_DESCRIPTOR SePublicDefaultSd = NULL;
PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SeSystemDefaultSd = NULL;
PSECURITY_DESCRIPTOR SeUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SeSystemAnonymousLogonSd = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Initializes the known security descriptors in the system.
 * 
 * @return
 * Returns TRUE if all the security descriptors have been initialized,
 * FALSE otherwise.
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitSDs(VOID)
{
    /* Create PublicDefaultSd */
    SePublicDefaultSd = ExAllocatePoolWithTag(PagedPool,
                                              sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SePublicDefaultSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SePublicDefaultSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SePublicDefaultSd,
                                 TRUE,
                                 SePublicDefaultDacl,
                                 FALSE);

    /* Create PublicDefaultUnrestrictedSd */
    SePublicDefaultUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
                                                          sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SePublicDefaultUnrestrictedSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SePublicDefaultUnrestrictedSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SePublicDefaultUnrestrictedSd,
                                 TRUE,
                                 SePublicDefaultUnrestrictedDacl,
                                 FALSE);

    /* Create PublicOpenSd */
    SePublicOpenSd = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SePublicOpenSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SePublicOpenSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SePublicOpenSd,
                                 TRUE,
                                 SePublicOpenDacl,
                                 FALSE);

    /* Create PublicOpenUnrestrictedSd */
    SePublicOpenUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
                                                       sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SePublicOpenUnrestrictedSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SePublicOpenUnrestrictedSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SePublicOpenUnrestrictedSd,
                                 TRUE,
                                 SePublicOpenUnrestrictedDacl,
                                 FALSE);

    /* Create SystemDefaultSd */
    SeSystemDefaultSd = ExAllocatePoolWithTag(PagedPool,
                                              sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SeSystemDefaultSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SeSystemDefaultSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SeSystemDefaultSd,
                                 TRUE,
                                 SeSystemDefaultDacl,
                                 FALSE);

    /* Create UnrestrictedSd */
    SeUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
                                             sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SeUnrestrictedSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SeUnrestrictedSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SeUnrestrictedSd,
                                 TRUE,
                                 SeUnrestrictedDacl,
                                 FALSE);

    /* Create SystemAnonymousLogonSd */
    SeSystemAnonymousLogonSd = ExAllocatePoolWithTag(PagedPool,
                                                     sizeof(SECURITY_DESCRIPTOR), TAG_SD);
    if (SeSystemAnonymousLogonSd == NULL)
        return FALSE;

    RtlCreateSecurityDescriptor(SeSystemAnonymousLogonSd,
                                SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SeSystemAnonymousLogonSd,
                                 TRUE,
                                 SeSystemAnonymousLogonDacl,
                                 FALSE);

    return TRUE;
}

/**
 * @brief
 * Sets a "World" security descriptor.
 * 
 * @param[in] SecurityInformation
 * Security information details, alongside with the security
 * descriptor to set the World SD.
 * 
 * @param[in] SecurityDescriptor
 * A security descriptor buffer.
 * 
 * @param[in] BufferLength
 * Length size of the buffer.
 * 
 * @return
 * Returns STATUS_SUCCESS if the World security descriptor has been
 * set. STATUS_ACCESS_DENIED is returned if the caller hasn't
 * provided security information details thus the SD cannot
 * be set.
 */
NTSTATUS
NTAPI
SeSetWorldSecurityDescriptor(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PISECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PULONG BufferLength)
{
    ULONG Current;
    ULONG SidSize;
    ULONG SdSize;
    NTSTATUS Status;
    PISECURITY_DESCRIPTOR_RELATIVE SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

    DPRINT("SeSetWorldSecurityDescriptor() called\n");

    if (SecurityInformation == 0)
    {
        return STATUS_ACCESS_DENIED;
    }

    /* calculate the minimum size of the buffer */
    SidSize = RtlLengthSid(SeWorldSid);
    SdSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
        SdSize += SidSize;
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
        SdSize += SidSize;
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        SdSize += sizeof(ACL) + sizeof(ACE) + SidSize;
    }

    if (*BufferLength < SdSize)
    {
        *BufferLength = SdSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    *BufferLength = SdSize;

    Status = RtlCreateSecurityDescriptorRelative(SdRel,
                                                 SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Current = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        RtlCopyMemory((PUCHAR)SdRel + Current, SeWorldSid, SidSize);
        SdRel->Owner = Current;
        Current += SidSize;
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        RtlCopyMemory((PUCHAR)SdRel + Current, SeWorldSid, SidSize);
        SdRel->Group = Current;
        Current += SidSize;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        PACL Dacl = (PACL)((PUCHAR)SdRel + Current);
        SdRel->Control |= SE_DACL_PRESENT;

        Status = RtlCreateAcl(Dacl,
                              sizeof(ACL) + sizeof(ACE) + SidSize,
                              ACL_REVISION);
        if (!NT_SUCCESS(Status))
            return Status;

        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        SeWorldSid);
        if (!NT_SUCCESS(Status))
            return Status;

        SdRel->Dacl = Current;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        /* FIXME - SdRel->Control |= SE_SACL_PRESENT; */
    }

    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Determines the size of a SID.
 * 
 * @param[in] Sid
 * A security identifier where its size is to be determined.
 * 
 * @param[in,out] OutSAC
 * The returned sub authority count of the security
 * identifier.
 * 
 * @param[in] ProcessorMode
 * Processor level access mode.
 * 
 * @return
 * Returns the size length of a security identifier (SID).
 */
static
ULONG
DetermineSIDSize(
    _In_ PISID Sid,
    _Inout_ PULONG OutSAC,
    _In_ KPROCESSOR_MODE ProcessorMode)
{
    ULONG Size;

    if (!Sid)
    {
        *OutSAC = 0;
        return 0;
    }

    if (ProcessorMode != KernelMode)
    {
        /* Securely access the buffers! */
        *OutSAC = ProbeForReadUchar(&Sid->SubAuthorityCount);
        Size = RtlLengthRequiredSid(*OutSAC);
        ProbeForRead(Sid, Size, sizeof(ULONG));
    }
    else
    {
        *OutSAC = Sid->SubAuthorityCount;
        Size = RtlLengthRequiredSid(*OutSAC);
    }

    return Size;
}

/**
 * @brief
 * Determines the size of an ACL.
 * 
 * @param[in] Acl
 * An access control list where its size is to be
 * determined.
 * 
 * @param[in] ProcessorMode
 * Processor level access mode.
 * 
 * @return
 * Returns the size length of a an access control
 * list (ACL).
 */
static
ULONG
DetermineACLSize(
    _In_ PACL Acl,
    _In_ KPROCESSOR_MODE ProcessorMode)
{
    ULONG Size;

    if (!Acl) return 0;

    if (ProcessorMode == KernelMode) return Acl->AclSize;

    /* Probe the buffers! */
    Size = ProbeForReadUshort(&Acl->AclSize);
    ProbeForRead(Acl, Size, sizeof(ULONG));

    return Size;
}

/**
 * @brief
 * Captures a security descriptor.
 * 
 * @param[in] _OriginalSecurityDescriptor
 * An already existing and valid security descriptor
 * to be captured.
 * 
 * @param[in] CurrentMode
 * Processor level access mode.
 * 
 * @param[in] PoolType
 * Pool type to be used when allocating the captured
 * buffer.
 * 
 * @param[in] CaptureIfKernel
 * Set this to TRUE if capturing is done within the
 * kernel.
 * 
 * @param[out] CapturedSecurityDescriptor
 * The captured security descriptor.
 * 
 * @return
 * Returns STATUS_SUCCESS if the operations have been
 * completed successfully and that the security descriptor
 * has been captured. STATUS_UNKNOWN_REVISION is returned
 * if the security descriptor has an unknown revision.
 * STATUS_INSUFFICIENT_RESOURCES is returned if memory
 * pool allocation for the captured buffer has failed.
 * A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeCaptureSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR _OriginalSecurityDescriptor,
    _In_ KPROCESSOR_MODE CurrentMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor)
{
    PISECURITY_DESCRIPTOR OriginalDescriptor = _OriginalSecurityDescriptor;
    SECURITY_DESCRIPTOR DescriptorCopy;
    PISECURITY_DESCRIPTOR_RELATIVE NewDescriptor;
    ULONG OwnerSAC = 0, GroupSAC = 0;
    ULONG OwnerSize = 0, GroupSize = 0;
    ULONG SaclSize = 0, DaclSize = 0;
    ULONG DescriptorSize = 0;
    ULONG Offset;

    if (!OriginalDescriptor)
    {
        /* Nothing to do... */
        *CapturedSecurityDescriptor = NULL;
        return STATUS_SUCCESS;
    }

    /* Quick path */
    if (CurrentMode == KernelMode && !CaptureIfKernel)
    {
        /* Check descriptor version */
        if (OriginalDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
        {
            return STATUS_UNKNOWN_REVISION;
        }

        *CapturedSecurityDescriptor = _OriginalSecurityDescriptor;
        return STATUS_SUCCESS;
    }

    _SEH2_TRY
    {
        if (CurrentMode != KernelMode)
        {
            ProbeForRead(OriginalDescriptor,
                         sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                         sizeof(ULONG));
        }

        /* Check the descriptor version */
        if (OriginalDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
        {
            _SEH2_YIELD(return STATUS_UNKNOWN_REVISION);
        }

        if (CurrentMode != KernelMode)
        {
            /* Get the size of the descriptor */
            DescriptorSize = (OriginalDescriptor->Control & SE_SELF_RELATIVE) ?
                sizeof(SECURITY_DESCRIPTOR_RELATIVE) : sizeof(SECURITY_DESCRIPTOR);

            /* Probe the entire security descriptor structure. The SIDs
             * and ACLs will be probed and copied later though */
            ProbeForRead(OriginalDescriptor, DescriptorSize, sizeof(ULONG));
        }

        /* Now capture all fields and convert to an absolute descriptor */
        DescriptorCopy.Revision = OriginalDescriptor->Revision;
        DescriptorCopy.Sbz1 = OriginalDescriptor->Sbz1;
        DescriptorCopy.Control = OriginalDescriptor->Control & ~SE_SELF_RELATIVE;
        DescriptorCopy.Owner = SepGetOwnerFromDescriptor(OriginalDescriptor);
        DescriptorCopy.Group = SepGetGroupFromDescriptor(OriginalDescriptor);
        DescriptorCopy.Sacl = SepGetSaclFromDescriptor(OriginalDescriptor);
        DescriptorCopy.Dacl = SepGetDaclFromDescriptor(OriginalDescriptor);
        DescriptorSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

        /* Determine owner and group sizes */
        OwnerSize = DetermineSIDSize(DescriptorCopy.Owner, &OwnerSAC, CurrentMode);
        DescriptorSize += ROUND_UP(OwnerSize, sizeof(ULONG));
        GroupSize = DetermineSIDSize(DescriptorCopy.Group, &GroupSAC, CurrentMode);
        DescriptorSize += ROUND_UP(GroupSize, sizeof(ULONG));

        /* Determine the size of the ACLs */
        if (DescriptorCopy.Control & SE_SACL_PRESENT)
        {
            /* Get the size and probe if user mode */
            SaclSize = DetermineACLSize(DescriptorCopy.Sacl, CurrentMode);
            DescriptorSize += ROUND_UP(SaclSize, sizeof(ULONG));
        }

        if (DescriptorCopy.Control & SE_DACL_PRESENT)
        {
            /* Get the size and probe if user mode */
            DaclSize = DetermineACLSize(DescriptorCopy.Dacl, CurrentMode);
            DescriptorSize += ROUND_UP(DaclSize, sizeof(ULONG));
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /*
     * Allocate enough memory to store a complete copy of a self-relative
     * security descriptor
     */
    NewDescriptor = ExAllocatePoolWithTag(PoolType,
                                          DescriptorSize,
                                          TAG_SD);
    if (!NewDescriptor) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(NewDescriptor, DescriptorSize);
    NewDescriptor->Revision = DescriptorCopy.Revision;
    NewDescriptor->Sbz1 = DescriptorCopy.Sbz1;
    NewDescriptor->Control = DescriptorCopy.Control | SE_SELF_RELATIVE;

    _SEH2_TRY
    {
        /*
         * Setup the offsets and copy the SIDs and ACLs to the new
         * self-relative security descriptor. Probing the pointers is not
         * neccessary anymore as we did that when collecting the sizes!
         * Make sure to validate the SIDs and ACLs *again* as they could have
         * been modified in the meanwhile!
         */
        Offset = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

        if (DescriptorCopy.Owner)
        {
            if (!RtlValidSid(DescriptorCopy.Owner)) RtlRaiseStatus(STATUS_INVALID_SID);
            NewDescriptor->Owner = Offset;
            RtlCopyMemory((PUCHAR)NewDescriptor + Offset,
                          DescriptorCopy.Owner,
                          OwnerSize);
            Offset += ROUND_UP(OwnerSize, sizeof(ULONG));
        }

        if (DescriptorCopy.Group)
        {
            if (!RtlValidSid(DescriptorCopy.Group)) RtlRaiseStatus(STATUS_INVALID_SID);
            NewDescriptor->Group = Offset;
            RtlCopyMemory((PUCHAR)NewDescriptor + Offset,
                          DescriptorCopy.Group,
                          GroupSize);
            Offset += ROUND_UP(GroupSize, sizeof(ULONG));
        }

        if (DescriptorCopy.Sacl)
        {
            if (!RtlValidAcl(DescriptorCopy.Sacl)) RtlRaiseStatus(STATUS_INVALID_ACL);
            NewDescriptor->Sacl = Offset;
            RtlCopyMemory((PUCHAR)NewDescriptor + Offset,
                          DescriptorCopy.Sacl,
                          SaclSize);
            Offset += ROUND_UP(SaclSize, sizeof(ULONG));
        }

        if (DescriptorCopy.Dacl)
        {
            if (!RtlValidAcl(DescriptorCopy.Dacl)) RtlRaiseStatus(STATUS_INVALID_ACL);
            NewDescriptor->Dacl = Offset;
            RtlCopyMemory((PUCHAR)NewDescriptor + Offset,
                          DescriptorCopy.Dacl,
                          DaclSize);
            Offset += ROUND_UP(DaclSize, sizeof(ULONG));
        }

        /* Make sure the size was correct */
        ASSERT(Offset == DescriptorSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We failed to copy the data to the new descriptor */
        ExFreePoolWithTag(NewDescriptor, TAG_SD);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /*
     * We're finally done!
     * Copy the pointer to the captured descriptor to to the caller.
     */
    *CapturedSecurityDescriptor = NewDescriptor;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Queries information details about a security
 * descriptor.
 * 
 * @param[in] SecurityInformation
 * Security information details to be queried
 * from a security descriptor.
 * 
 * @param[out] SecurityDescriptor
 * The returned security descriptor with security information
 * data.
 * 
 * @param[in,out] Length
 * The returned length of a security descriptor.
 * 
 * @param[in,out] ObjectsSecurityDescriptor
 * The returned object security descriptor.
 * 
 * @return
 * Returns STATUS_SUCCESS if the operations have been
 * completed successfully and that the specific information
 * about the security descriptor has been queried.
 * STATUS_BUFFER_TOO_SMALL is returned if the buffer size
 * is too small to contain the queried info about the
 * security descriptor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeQuerySecurityDescriptorInfo(
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_(*Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PULONG Length,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor)
{
    PISECURITY_DESCRIPTOR ObjectSd;
    PISECURITY_DESCRIPTOR_RELATIVE RelSD;
    PSID Owner = NULL;
    PSID Group = NULL;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    ULONG OwnerLength = 0;
    ULONG GroupLength = 0;
    ULONG DaclLength = 0;
    ULONG SaclLength = 0;
    SECURITY_DESCRIPTOR_CONTROL Control = 0;
    ULONG_PTR Current;
    ULONG SdLength;

    PAGED_CODE();

    RelSD = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

    if (*ObjectsSecurityDescriptor == NULL)
    {
        if (*Length < sizeof(SECURITY_DESCRIPTOR_RELATIVE))
        {
            *Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
            return STATUS_BUFFER_TOO_SMALL;
        }

        *Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
        RtlCreateSecurityDescriptorRelative(RelSD,
                                            SECURITY_DESCRIPTOR_REVISION);
        return STATUS_SUCCESS;
    }

    ObjectSd = *ObjectsSecurityDescriptor;

    /* Calculate the required security descriptor length */
    Control = SE_SELF_RELATIVE;
    if (*SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        Owner = SepGetOwnerFromDescriptor(ObjectSd);
        if (Owner != NULL)
        {
            OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
            Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
        }
    }

    if (*SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        Group = SepGetGroupFromDescriptor(ObjectSd);
        if (Group != NULL)
        {
            GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
            Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
        }
    }

    if ((*SecurityInformation & DACL_SECURITY_INFORMATION) &&
        (ObjectSd->Control & SE_DACL_PRESENT))
    {
        Dacl = SepGetDaclFromDescriptor(ObjectSd);
        if (Dacl != NULL)
        {
            DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
        }

        Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
    }

    if ((*SecurityInformation & SACL_SECURITY_INFORMATION) &&
        (ObjectSd->Control & SE_SACL_PRESENT))
    {
        Sacl = SepGetSaclFromDescriptor(ObjectSd);
        if (Sacl != NULL)
        {
            SaclLength = ROUND_UP(Sacl->AclSize, 4);
        }

        Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
    }

    SdLength = OwnerLength + GroupLength + DaclLength +
    SaclLength + sizeof(SECURITY_DESCRIPTOR_RELATIVE);
    if (*Length < SdLength)
    {
        *Length = SdLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Build the new security descrtiptor */
    RtlCreateSecurityDescriptorRelative(RelSD,
                                        SECURITY_DESCRIPTOR_REVISION);
    RelSD->Control = Control;

    Current = (ULONG_PTR)(RelSD + 1);

    if (OwnerLength != 0)
    {
        RtlCopyMemory((PVOID)Current,
                      Owner,
                      OwnerLength);
        RelSD->Owner = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
        Current += OwnerLength;
    }

    if (GroupLength != 0)
    {
        RtlCopyMemory((PVOID)Current,
                      Group,
                      GroupLength);
        RelSD->Group = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
        Current += GroupLength;
    }

    if (DaclLength != 0)
    {
        RtlCopyMemory((PVOID)Current,
                      Dacl,
                      DaclLength);
        RelSD->Dacl = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
        Current += DaclLength;
    }

    if (SaclLength != 0)
    {
        RtlCopyMemory((PVOID)Current,
                      Sacl,
                      SaclLength);
        RelSD->Sacl = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
        Current += SaclLength;
    }

    *Length = SdLength;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Releases a captured security descriptor buffer.
 * 
 * @param[in] CapturedSecurityDescriptor
 * The captured security descriptor to be freed.
 * 
 * @param[in] CurrentMode
 * Processor level access mode.
 * 
 * @param[in] CaptureIfKernelMode
 * Set this to TRUE if the releasing is to be done within
 * the kernel.
 * 
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
SeReleaseSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    _In_ KPROCESSOR_MODE CurrentMode,
    _In_ BOOLEAN CaptureIfKernelMode)
{
    PAGED_CODE();

    /*
     * WARNING! You need to call this function with the same value for CurrentMode
     * and CaptureIfKernelMode that you previously passed to
     * SeCaptureSecurityDescriptor() in order to avoid memory leaks!
     */
    if (CapturedSecurityDescriptor != NULL &&
        (CurrentMode != KernelMode ||
         (CurrentMode == KernelMode && CaptureIfKernelMode)))
    {
        /* Only delete the descriptor when SeCaptureSecurityDescriptor() allocated one! */
        ExFreePoolWithTag(CapturedSecurityDescriptor, TAG_SD);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Modifies some information data about a security
 * descriptor.
 * 
 * @param[in] Object
 * If specified, the function will use this arbitrary
 * object that points to an object security descriptor.
 * 
 * @param[in] SecurityInformation
 * Security information details to be set.
 * 
 * @param[in] SecurityDescriptor
 * A security descriptor where its info is to be changed.
 * 
 * @param[in,out] ObjectsSecurityDescriptor
 * The returned pointer to security descriptor objects.
 * 
 * @param[in] PoolType
 * Pool type for the new security descriptor to allocate.
 * 
 * @param[in] GenericMapping
 * The generic mapping of access rights masks.
 * 
 * @return
 * See SeSetSecurityDescriptorInfoEx.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfo(
    _In_opt_ PVOID Object,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    return SeSetSecurityDescriptorInfoEx(Object,
                                         SecurityInformation,
                                         SecurityDescriptor,
                                         ObjectsSecurityDescriptor,
                                         0,
                                         PoolType,
                                         GenericMapping);
}

/**
 * @brief
 * An extended function that sets new information data to
 * a security descriptor.
 * 
 * @param[in] Object
 * If specified, the function will use this arbitrary
 * object that points to an object security descriptor.
 * 
 * @param[in] SecurityInformation
 * Security information details to be set.
 * 
 * @param[in] SecurityDescriptor
 * A security descriptor where its info is to be changed.
 * 
 * @param[in,out] ObjectsSecurityDescriptor
 * The returned pointer to security descriptor objects.
 * 
 * @param[in] AutoInheritFlags
 * Flags bitmask inheritation, influencing how the security
 * descriptor can be inherited and if it can be in the first
 * place.
 * 
 * @param[in] PoolType
 * Pool type for the new security descriptor to allocate.
 * 
 * @param[in] GenericMapping
 * The generic mapping of access rights masks.
 * 
 * @return
 * Returns STATUS_SUCCESS if the operations have been
 * completed without problems and that new info has been
 * set to the security descriptor. STATUS_NO_SECURITY_ON_OBJECT
 * is returned if the object does not have a security descriptor.
 * STATUS_INSUFFICIENT_RESOURCES is returned if memory pool allocation
 * for the new security descriptor with new info set has failed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfoEx(
    _In_opt_ PVOID Object,
    _In_ PSECURITY_INFORMATION _SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR _SecurityDescriptor,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    _In_ ULONG AutoInheritFlags,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PISECURITY_DESCRIPTOR_RELATIVE ObjectSd;
    PISECURITY_DESCRIPTOR_RELATIVE NewSd;
    PISECURITY_DESCRIPTOR SecurityDescriptor = _SecurityDescriptor;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;
    ULONG OwnerLength;
    ULONG GroupLength;
    ULONG DaclLength;
    ULONG SaclLength;
    SECURITY_DESCRIPTOR_CONTROL Control = 0;
    ULONG Current;
    SECURITY_INFORMATION SecurityInformation;

    PAGED_CODE();

    ObjectSd = *ObjectsSecurityDescriptor;

    /* The object does not have a security descriptor. */
    if (!ObjectSd)
        return STATUS_NO_SECURITY_ON_OBJECT;

    ASSERT(ObjectSd->Control & SE_SELF_RELATIVE);

    SecurityInformation = *_SecurityInformation;

    /* Get owner and owner size */
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        Owner = SepGetOwnerFromDescriptor(SecurityDescriptor);
        Control |= (SecurityDescriptor->Control & SE_OWNER_DEFAULTED);
    }
    else
    {
        Owner = SepGetOwnerFromDescriptor(ObjectSd);
        Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
    }
    OwnerLength = Owner ? RtlLengthSid(Owner) : 0;
    ASSERT(OwnerLength % sizeof(ULONG) == 0);

    /* Get group and group size */
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        Group = SepGetGroupFromDescriptor(SecurityDescriptor);
        Control |= (SecurityDescriptor->Control & SE_GROUP_DEFAULTED);
    }
    else
    {
        Group = SepGetGroupFromDescriptor(ObjectSd);
        Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
    }
    GroupLength = Group ? RtlLengthSid(Group) : 0;
    ASSERT(GroupLength % sizeof(ULONG) == 0);

    /* Get DACL and DACL size */
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        Dacl = SepGetDaclFromDescriptor(SecurityDescriptor);
        Control |= (SecurityDescriptor->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
    }
    else
    {
        Dacl = SepGetDaclFromDescriptor(ObjectSd);
        Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
    }
    DaclLength = Dacl ? ROUND_UP((ULONG)Dacl->AclSize, 4) : 0;

    /* Get SACL and SACL size */
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        Sacl = SepGetSaclFromDescriptor(SecurityDescriptor);
        Control |= (SecurityDescriptor->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
    }
    else
    {
        Sacl = SepGetSaclFromDescriptor(ObjectSd);
        Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
    }
    SaclLength = Sacl ? ROUND_UP((ULONG)Sacl->AclSize, 4) : 0;

    NewSd = ExAllocatePoolWithTag(PoolType,
                                  sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                                  OwnerLength + GroupLength +
                                  DaclLength + SaclLength,
                                  TAG_SD);
    if (NewSd == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCreateSecurityDescriptorRelative(NewSd, SECURITY_DESCRIPTOR_REVISION1);

    Current = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if (OwnerLength != 0)
    {
        RtlCopyMemory((PUCHAR)NewSd + Current, Owner, OwnerLength);
        NewSd->Owner = Current;
        Current += OwnerLength;
    }

    if (GroupLength != 0)
    {
        RtlCopyMemory((PUCHAR)NewSd + Current, Group, GroupLength);
        NewSd->Group = Current;
        Current += GroupLength;
    }

    if (DaclLength != 0)
    {
        RtlCopyMemory((PUCHAR)NewSd + Current, Dacl, DaclLength);
        NewSd->Dacl = Current;
        Current += DaclLength;
    }

    if (SaclLength != 0)
    {
        RtlCopyMemory((PUCHAR)NewSd + Current, Sacl, SaclLength);
        NewSd->Sacl = Current;
        Current += SaclLength;
    }

    NewSd->Control |= Control;
    *ObjectsSecurityDescriptor = NewSd;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Determines if a security descriptor is valid according
 * to the general security requirements and conditions
 * set by the kernel.
 * 
 * @param[in] Length
 * The length of a security descriptor.
 * 
 * @param[in] _SecurityDescriptor
 * A security descriptor where its properties are to be
 * checked for validity.
 * 
 * @return
 * Returns TRUE if the given security descriptor is valid,
 * FALSE otherwise.
 */
BOOLEAN NTAPI
SeValidSecurityDescriptor(
    _In_ ULONG Length,
    _In_ PSECURITY_DESCRIPTOR _SecurityDescriptor)
{
    ULONG SdLength;
    PISID Sid;
    PACL Acl;
    PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor = _SecurityDescriptor;

    if (Length < SECURITY_DESCRIPTOR_MIN_LENGTH)
    {
        DPRINT1("Invalid Security Descriptor revision\n");
        return FALSE;
    }

    if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
    {
        DPRINT1("Invalid Security Descriptor revision\n");
        return FALSE;
    }

    if (!(SecurityDescriptor->Control & SE_SELF_RELATIVE))
    {
        DPRINT1("No self-relative Security Descriptor\n");
        return FALSE;
    }

    SdLength = sizeof(SECURITY_DESCRIPTOR);

    /* Check Owner SID */
    if (!SecurityDescriptor->Owner)
    {
        DPRINT1("No Owner SID\n");
        return FALSE;
    }

    if (SecurityDescriptor->Owner % sizeof(ULONG))
    {
        DPRINT1("Invalid Owner SID alignment\n");
        return FALSE;
    }

    Sid = (PISID)((ULONG_PTR)SecurityDescriptor + SecurityDescriptor->Owner);
    if (Sid->Revision != SID_REVISION)
    {
        DPRINT1("Invalid Owner SID revision\n");
        return FALSE;
    }

    SdLength += (sizeof(SID) + (Sid->SubAuthorityCount - 1) * sizeof(ULONG));
    if (Length < SdLength)
    {
        DPRINT1("Invalid Owner SID size\n");
        return FALSE;
    }

    /* Check Group SID */
    if (SecurityDescriptor->Group)
    {
        if (SecurityDescriptor->Group % sizeof(ULONG))
        {
            DPRINT1("Invalid Group SID alignment\n");
            return FALSE;
        }

        Sid = (PSID)((ULONG_PTR)SecurityDescriptor + SecurityDescriptor->Group);
        if (Sid->Revision != SID_REVISION)
        {
            DPRINT1("Invalid Group SID revision\n");
            return FALSE;
        }

        SdLength += (sizeof(SID) + (Sid->SubAuthorityCount - 1) * sizeof(ULONG));
        if (Length < SdLength)
        {
            DPRINT1("Invalid Group SID size\n");
            return FALSE;
        }
    }

    /* Check DACL */
    if (SecurityDescriptor->Dacl)
    {
        if (SecurityDescriptor->Dacl % sizeof(ULONG))
        {
            DPRINT1("Invalid DACL alignment\n");
            return FALSE;
        }

        Acl = (PACL)((ULONG_PTR)SecurityDescriptor + SecurityDescriptor->Dacl);
        if ((Acl->AclRevision < MIN_ACL_REVISION) ||
            (Acl->AclRevision > MAX_ACL_REVISION))
        {
            DPRINT1("Invalid DACL revision\n");
            return FALSE;
        }

        SdLength += Acl->AclSize;
        if (Length < SdLength)
        {
            DPRINT1("Invalid DACL size\n");
            return FALSE;
        }
    }

    /* Check SACL */
    if (SecurityDescriptor->Sacl)
    {
        if (SecurityDescriptor->Sacl % sizeof(ULONG))
        {
            DPRINT1("Invalid SACL alignment\n");
            return FALSE;
        }

        Acl = (PACL)((ULONG_PTR)SecurityDescriptor + SecurityDescriptor->Sacl);
        if ((Acl->AclRevision < MIN_ACL_REVISION) ||
            (Acl->AclRevision > MAX_ACL_REVISION))
        {
            DPRINT1("Invalid SACL revision\n");
            return FALSE;
        }

        SdLength += Acl->AclSize;
        if (Length < SdLength)
        {
            DPRINT1("Invalid SACL size\n");
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief
 * Frees a security descriptor.
 * 
 * @param[in] SecurityDescriptor
 * A security descriptor to be freed from memory.
 * 
 * @return
 * Returns STATUS_SUCCESS.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeDeassignSecurity(
    _Inout_ PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
    PAGED_CODE();

    if (*SecurityDescriptor != NULL)
    {
        ExFreePoolWithTag(*SecurityDescriptor, TAG_SD);
        *SecurityDescriptor = NULL;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * An extended function that assigns a security descriptor for a new
 * object.
 * 
 * @param[in] _ParentDescriptor
 * A security descriptor of the parent object that is being
 * created.
 * 
 * @param[in] _ExplicitDescriptor
 * An explicit security descriptor that is applied to a new
 * object.
 * 
 * @param[out] NewDescriptor
 * The new allocated security descriptor.
 * 
 * @param[in] ObjectType
 * The type of the new object.
 * 
 * @param[in] IsDirectoryObject
 * Set this to TRUE if the newly created object is a directory
 * object, otherwise set this to FALSE.
 * 
 * @param[in] AutoInheritFlags
 * Automatic inheritance flags that influence how access control
 * entries within ACLs from security descriptors are inherited.
 * 
 * @param[in] SubjectContext
 * Security subject context of the new object.
 * 
 * @param[in] GenericMapping
 * Generic mapping of access mask rights.
 * 
 * @param[in] PoolType
 * This parameter is unused.
 * 
 * @return
 * Returns STATUS_SUCCESS if the operations have been completed
 * successfully and that the security descriptor has been
 * assigned to the new object. STATUS_NO_TOKEN is returned
 * if the caller hasn't supplied a valid argument to a security
 * subject context. STATUS_INVALID_OWNER is returned if the caller
 * hasn't supplied a parent descriptor that belongs to the main
 * user (owner). STATUS_INVALID_PRIMARY_GROUP is returned
 * by the same reason as with the previous NTSTATUS code.
 * The two NTSTATUS codes are returned if the calling thread
 * stated that the owner and/or group is defaulted to the
 * parent descriptor (SEF_DEFAULT_OWNER_FROM_PARENT and/or
 * SEF_DEFAULT_GROUP_FROM_PARENT respectively).
 * STATUS_INSUFFICIENT_RESOURCES is returned if memory pool allocation
 * for the descriptor buffer has failed. A failure NTSTATUS is returned
 * otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeAssignSecurityEx(
    _In_opt_ PSECURITY_DESCRIPTOR _ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR _ExplicitDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_opt_ GUID *ObjectType,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ ULONG AutoInheritFlags,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ POOL_TYPE PoolType)
{
    PISECURITY_DESCRIPTOR ParentDescriptor = _ParentDescriptor;
    PISECURITY_DESCRIPTOR ExplicitDescriptor = _ExplicitDescriptor;
    PISECURITY_DESCRIPTOR_RELATIVE Descriptor;
    PTOKEN Token;
    ULONG OwnerLength;
    ULONG GroupLength;
    ULONG DaclLength;
    ULONG SaclLength;
    ULONG Length;
    SECURITY_DESCRIPTOR_CONTROL Control = 0;
    ULONG Current;
    PSID Owner = NULL;
    PSID Group = NULL;
    PACL ExplicitAcl;
    BOOLEAN ExplicitPresent;
    BOOLEAN ExplicitDefaulted;
    PACL ParentAcl;
    PACL Dacl = NULL;
    PACL Sacl = NULL;
    BOOLEAN DaclIsInherited;
    BOOLEAN SaclIsInherited;
    BOOLEAN DaclPresent;
    BOOLEAN SaclPresent;
    NTSTATUS Status;

    DBG_UNREFERENCED_PARAMETER(ObjectType);
    DBG_UNREFERENCED_PARAMETER(AutoInheritFlags);
    UNREFERENCED_PARAMETER(PoolType);

    PAGED_CODE();

    *NewDescriptor = NULL;

    if (!ARGUMENT_PRESENT(SubjectContext))
    {
        return STATUS_NO_TOKEN;
    }

    /* Lock subject context */
    SeLockSubjectContext(SubjectContext);

    if (SubjectContext->ClientToken != NULL)
    {
        Token = SubjectContext->ClientToken;
    }
    else
    {
        Token = SubjectContext->PrimaryToken;
    }

    /* Inherit the Owner SID */
    if (ExplicitDescriptor != NULL)
    {
        DPRINT("Use explicit owner sid!\n");
        Owner = SepGetOwnerFromDescriptor(ExplicitDescriptor);
    }
    if (!Owner)
    {
        if (AutoInheritFlags & SEF_DEFAULT_OWNER_FROM_PARENT)
        {
            DPRINT("Use parent owner sid!\n");
            if (!ARGUMENT_PRESENT(ParentDescriptor))
            {
                SeUnlockSubjectContext(SubjectContext);
                return STATUS_INVALID_OWNER;
            }

            Owner = SepGetOwnerFromDescriptor(ParentDescriptor);
            if (!Owner)
            {
                SeUnlockSubjectContext(SubjectContext);
                return STATUS_INVALID_OWNER;
            }
        }
        else
        {
            DPRINT("Use token owner sid!\n");
            Owner = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
        }
    }
    OwnerLength = RtlLengthSid(Owner);
    ASSERT(OwnerLength % sizeof(ULONG) == 0);

    /* Inherit the Group SID */
    if (ExplicitDescriptor != NULL)
    {
        Group = SepGetGroupFromDescriptor(ExplicitDescriptor);
    }
    if (!Group)
    {
        if (AutoInheritFlags & SEF_DEFAULT_GROUP_FROM_PARENT)
        {
            DPRINT("Use parent group sid!\n");
            if (!ARGUMENT_PRESENT(ParentDescriptor))
            {
                SeUnlockSubjectContext(SubjectContext);
                return STATUS_INVALID_PRIMARY_GROUP;
            }

            Group = SepGetGroupFromDescriptor(ParentDescriptor);
            if (!Group)
            {
                SeUnlockSubjectContext(SubjectContext);
                return STATUS_INVALID_PRIMARY_GROUP;
            }
        }
        else
        {
            DPRINT("Use token group sid!\n");
            Group = Token->PrimaryGroup;
        }
    }
    if (!Group)
    {
        SeUnlockSubjectContext(SubjectContext);
        return STATUS_INVALID_PRIMARY_GROUP;
    }
    GroupLength = RtlLengthSid(Group);
    ASSERT(GroupLength % sizeof(ULONG) == 0);

    /* Inherit the DACL */
    DaclLength = 0;
    ExplicitAcl = NULL;
    ExplicitPresent = FALSE;
    ExplicitDefaulted = FALSE;
    if (ExplicitDescriptor != NULL &&
        (ExplicitDescriptor->Control & SE_DACL_PRESENT))
    {
        ExplicitAcl = SepGetDaclFromDescriptor(ExplicitDescriptor);
        ExplicitPresent = TRUE;
        if (ExplicitDescriptor->Control & SE_DACL_DEFAULTED)
            ExplicitDefaulted = TRUE;
    }
    ParentAcl = NULL;
    if (ParentDescriptor != NULL &&
        (ParentDescriptor->Control & SE_DACL_PRESENT))
    {
        ParentAcl = SepGetDaclFromDescriptor(ParentDescriptor);
    }
    Dacl = SepSelectAcl(ExplicitAcl,
                        ExplicitPresent,
                        ExplicitDefaulted,
                        ParentAcl,
                        Token->DefaultDacl,
                        &DaclLength,
                        Owner,
                        Group,
                        &DaclPresent,
                        &DaclIsInherited,
                        IsDirectoryObject,
                        GenericMapping);
    if (DaclPresent)
        Control |= SE_DACL_PRESENT;
    ASSERT(DaclLength % sizeof(ULONG) == 0);

    /* Inherit the SACL */
    SaclLength = 0;
    ExplicitAcl = NULL;
    ExplicitPresent = FALSE;
    ExplicitDefaulted = FALSE;
    if (ExplicitDescriptor != NULL &&
        (ExplicitDescriptor->Control & SE_SACL_PRESENT))
    {
        ExplicitAcl = SepGetSaclFromDescriptor(ExplicitDescriptor);
        ExplicitPresent = TRUE;
        if (ExplicitDescriptor->Control & SE_SACL_DEFAULTED)
            ExplicitDefaulted = TRUE;
    }
    ParentAcl = NULL;
    if (ParentDescriptor != NULL &&
        (ParentDescriptor->Control & SE_SACL_PRESENT))
    {
        ParentAcl = SepGetSaclFromDescriptor(ParentDescriptor);
    }
    Sacl = SepSelectAcl(ExplicitAcl,
                        ExplicitPresent,
                        ExplicitDefaulted,
                        ParentAcl,
                        NULL,
                        &SaclLength,
                        Owner,
                        Group,
                        &SaclPresent,
                        &SaclIsInherited,
                        IsDirectoryObject,
                        GenericMapping);
    if (SaclPresent)
        Control |= SE_SACL_PRESENT;
    ASSERT(SaclLength % sizeof(ULONG) == 0);

    /* Allocate and initialize the new security descriptor */
    Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
        OwnerLength + GroupLength + DaclLength + SaclLength;

    DPRINT("L: sizeof(SECURITY_DESCRIPTOR) %u OwnerLength %lu GroupLength %lu DaclLength %lu SaclLength %lu\n",
           sizeof(SECURITY_DESCRIPTOR),
           OwnerLength,
           GroupLength,
           DaclLength,
           SaclLength);

    Descriptor = ExAllocatePoolWithTag(PagedPool, Length, TAG_SD);
    if (Descriptor == NULL)
    {
        DPRINT1("ExAlloctePool() failed\n");
        SeUnlockSubjectContext(SubjectContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Descriptor, Length);
    RtlCreateSecurityDescriptor(Descriptor, SECURITY_DESCRIPTOR_REVISION);

    Descriptor->Control = Control | SE_SELF_RELATIVE;

    Current = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if (SaclLength != 0)
    {
        Status = SepPropagateAcl((PACL)((PUCHAR)Descriptor + Current),
                                 &SaclLength,
                                 Sacl,
                                 Owner,
                                 Group,
                                 SaclIsInherited,
                                 IsDirectoryObject,
                                 GenericMapping);
        ASSERT(Status == STATUS_SUCCESS);
        Descriptor->Sacl = Current;
        Current += SaclLength;
    }

    if (DaclLength != 0)
    {
        Status = SepPropagateAcl((PACL)((PUCHAR)Descriptor + Current),
                                 &DaclLength,
                                 Dacl,
                                 Owner,
                                 Group,
                                 DaclIsInherited,
                                 IsDirectoryObject,
                                 GenericMapping);
        ASSERT(Status == STATUS_SUCCESS);
        Descriptor->Dacl = Current;
        Current += DaclLength;
    }

    if (OwnerLength != 0)
    {
        RtlCopyMemory((PUCHAR)Descriptor + Current, Owner, OwnerLength);
        Descriptor->Owner = Current;
        Current += OwnerLength;
        DPRINT("Owner of %p at %x\n", Descriptor, Descriptor->Owner);
    }
    else
    {
        DPRINT("Owner of %p is zero length\n", Descriptor);
    }

    if (GroupLength != 0)
    {
        RtlCopyMemory((PUCHAR)Descriptor + Current, Group, GroupLength);
        Descriptor->Group = Current;
    }

    /* Unlock subject context */
    SeUnlockSubjectContext(SubjectContext);

    *NewDescriptor = Descriptor;

    DPRINT("Descriptor %p\n", Descriptor);
    ASSERT(RtlLengthSecurityDescriptor(Descriptor));

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Assigns a security descriptor for a new object.
 * 
 * @param[in] ParentDescriptor
 * A security descriptor of the parent object that is being
 * created.
 * 
 * @param[in] ExplicitDescriptor
 * An explicit security descriptor that is applied to a new
 * object.
 * 
 * @param[out] NewDescriptor
 * The new allocated security descriptor.
 * 
 * @param[in] IsDirectoryObject
 * Set this to TRUE if the newly created object is a directory
 * object, otherwise set this to FALSE.
 * 
 * @param[in] SubjectContext
 * Security subject context of the new object.
 * 
 * @param[in] GenericMapping
 * Generic mapping of access mask rights.
 * 
 * @param[in] PoolType
 * This parameter is unused.
 * 
 * @return
 * See SeAssignSecurityEx.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeAssignSecurity(
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR ExplicitDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ POOL_TYPE PoolType)
{
    PAGED_CODE();

    return SeAssignSecurityEx(ParentDescriptor,
                              ExplicitDescriptor,
                              NewDescriptor,
                              NULL,
                              IsDirectoryObject,
                              0,
                              SubjectContext,
                              GenericMapping,
                              PoolType);
}

/**
 * @brief
 * Computes the quota size of a security descriptor.
 * 
 * @param[in] SecurityDescriptor
 * A security descriptor.
 * 
 * @param[out] QuotaInfoSize
 * The returned quota size of the given security descriptor to
 * the caller. The function may return 0 to this parameter if
 * the descriptor doesn't have a group or a discretionary
 * access control list (DACL) even.
 * 
 * @return
 * Returns STATUS_SUCCESS if the quota size of a security
 * descriptor has been computed successfully. STATUS_UNKNOWN_REVISION
 * is returned if the security descriptor has an invalid revision.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
SeComputeQuotaInformationSize(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PULONG QuotaInfoSize)
{
    PSID Group;
    PACL Dacl;

    PAGED_CODE();

    *QuotaInfoSize = 0;

    /* Validate security descriptor revision */
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION1)
    {
        return STATUS_UNKNOWN_REVISION;
    }

    /* Get group and DACL, if any */
    Group = SepGetGroupFromDescriptor(SecurityDescriptor);
    Dacl = SepGetDaclFromDescriptor(SecurityDescriptor);

    /* Return SID length if any */
    if (Group != NULL)
    {
        *QuotaInfoSize = ALIGN_UP_BY(RtlLengthSid(Group), sizeof(ULONG));
    }

    /* Return DACL if any */
    if (Dacl != NULL)
    {
        *QuotaInfoSize += ALIGN_UP_BY(Dacl->AclSize, sizeof(ULONG));
    }

    return STATUS_SUCCESS;
}

/* EOF */
