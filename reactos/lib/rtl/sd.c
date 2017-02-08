/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include "../../ntoskrnl/include/internal/se.h"
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
RtlpValidateSDOffsetAndSize(IN ULONG Offset,
                            IN ULONG Length,
                            IN ULONG MinLength,
                            OUT PULONG MaxLength)
{
    /* Assume failure */
    *MaxLength = 0;

    /* Reject out of bounds lengths */
    if (Offset < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) return FALSE;
    if (Offset >= Length) return FALSE;

    /* Reject insufficient lengths */
    if ((Length - Offset) < MinLength) return FALSE;

    /* Reject unaligned offsets */
    if (ALIGN_DOWN(Offset, ULONG) != Offset) return FALSE;

    /* Return length that is safe to read */
    *MaxLength = Length - Offset;
    return TRUE;
}

VOID
NTAPI
RtlpQuerySecurityDescriptor(IN PISECURITY_DESCRIPTOR SecurityDescriptor,
                            OUT PSID *Owner,
                            OUT PULONG OwnerSize,
                            OUT PSID *PrimaryGroup,
                            OUT PULONG PrimaryGroupSize,
                            OUT PACL *Dacl,
                            OUT PULONG DaclSize,
                            OUT PACL *Sacl,
                            OUT PULONG SaclSize)
{
    PAGED_CODE_RTL();

    /* Get the owner */
    *Owner = SepGetOwnerFromDescriptor(SecurityDescriptor);
    if (*Owner)
    {
        /* There's an owner, so align the size */
        *OwnerSize = ROUND_UP(RtlLengthSid(*Owner), sizeof(ULONG));
    }
    else
    {
        /* No owner, no size */
        *OwnerSize = 0;
    }

    /* Get the group */
    *PrimaryGroup = SepGetGroupFromDescriptor(SecurityDescriptor);
    if (*PrimaryGroup)
    {
        /* There's a group, so align the size */
        *PrimaryGroupSize = ROUND_UP(RtlLengthSid(*PrimaryGroup), sizeof(ULONG));
    }
    else
    {
        /* No group, no size */
        *PrimaryGroupSize = 0;
    }

    /* Get the DACL */
    *Dacl = SepGetDaclFromDescriptor(SecurityDescriptor);
    if (*Dacl)
    {
        /* There's a DACL, align the size */
        *DaclSize = ROUND_UP((*Dacl)->AclSize, sizeof(ULONG));
    }
    else
    {
        /* No DACL, no size */
        *DaclSize = 0;
    }

    /* Get the SACL */
    *Sacl = SepGetSaclFromDescriptor(SecurityDescriptor);
    if (*Sacl)
    {
        /* There's a SACL, align the size */
        *SaclSize = ROUND_UP((*Sacl)->AclSize, sizeof(ULONG));
    }
    else
    {
        /* No SACL, no size */
        *SaclSize = 0;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                            IN ULONG Revision)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revisions */
    if (Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Setup an empty SD */
    RtlZeroMemory(Sd, sizeof(*Sd));
    Sd->Revision = SECURITY_DESCRIPTOR_REVISION;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(IN PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
                                    IN ULONG Revision)
{
    PAGED_CODE_RTL();

    /* Fail on invalid revisions */
    if (Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Setup an empty SD */
    RtlZeroMemory(SecurityDescriptor, sizeof(*SecurityDescriptor));
    SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;
    SecurityDescriptor->Control = SE_SELF_RELATIVE;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlLengthSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PISECURITY_DESCRIPTOR Sd;
    PSID Owner, Group;
    PACL Sacl, Dacl;
    ULONG Length;
    PAGED_CODE_RTL();

    /* Start with the initial length of the SD itself */
    Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    if (Sd->Control & SE_SELF_RELATIVE)
    {
        Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
    }
    else
    {
        Length = sizeof(SECURITY_DESCRIPTOR);
    }

    /* Add the length of the individual subcomponents */
    Owner = SepGetOwnerFromDescriptor(Sd);
    if (Owner) Length += ROUND_UP(RtlLengthSid(Owner), sizeof(ULONG));
    Group = SepGetGroupFromDescriptor(Sd);
    if (Group) Length += ROUND_UP(RtlLengthSid(Group), sizeof(ULONG));
    Dacl = SepGetDaclFromDescriptor(Sd);
    if (Dacl) Length += ROUND_UP(Dacl->AclSize, sizeof(ULONG));
    Sacl = SepGetSaclFromDescriptor(Sd);
    if (Sacl) Length += ROUND_UP(Sacl->AclSize, sizeof(ULONG));

    /* Return the final length */
    return Length;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                             OUT PBOOLEAN DaclPresent,
                             OUT PACL* Dacl,
                             OUT PBOOLEAN DaclDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revisions */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Is there a DACL? */
    *DaclPresent = (Sd->Control & SE_DACL_PRESENT) == SE_DACL_PRESENT;
    if (*DaclPresent)
    {
        /* Yes, return it, and check if defaulted */
        *Dacl = SepGetDaclFromDescriptor(Sd);
        *DaclDefaulted = (Sd->Control & SE_DACL_DEFAULTED) == SE_DACL_DEFAULTED;
    }

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                             OUT PBOOLEAN SaclPresent,
                             OUT PACL* Sacl,
                             OUT PBOOLEAN SaclDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revisions */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Is there a SACL? */
    *SaclPresent = (Sd->Control & SE_SACL_PRESENT) == SE_SACL_PRESENT;
    if (*SaclPresent)
    {
        /* Yes, return it, and check if defaulted */
        *Sacl = SepGetSaclFromDescriptor(Sd);
        *SaclDefaulted = (Sd->Control & SE_SACL_DEFAULTED) == SE_SACL_DEFAULTED;
    }

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              OUT PSID* Owner,
                              OUT PBOOLEAN OwnerDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Get the owner and if defaulted */
    *Owner = SepGetOwnerFromDescriptor(Sd);
    *OwnerDefaulted = (Sd->Control & SE_OWNER_DEFAULTED) == SE_OWNER_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              OUT PSID* Group,
                              OUT PBOOLEAN GroupDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Get the group and if defaulted */
    *Group = SepGetGroupFromDescriptor(Sd);
    *GroupDefaulted = (Sd->Control & SE_GROUP_DEFAULTED) == SE_GROUP_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                             IN BOOLEAN DaclPresent,
                             IN PACL Dacl,
                             IN BOOLEAN DaclDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Fail on relative descriptors */
    if (Sd->Control & SE_SELF_RELATIVE) return STATUS_INVALID_SECURITY_DESCR;

    /* Is there a DACL? */
    if (!DaclPresent)
    {
        /* Caller is destroying the DACL, unset the flag and we're done */
        Sd->Control = Sd->Control & ~SE_DACL_PRESENT;
        return STATUS_SUCCESS;
    }

    /* Caller is setting a new DACL, set the pointer and flag */
    Sd->Dacl = Dacl;
    Sd->Control |= SE_DACL_PRESENT;

    /* Set if defaulted */
    Sd->Control &= ~SE_DACL_DEFAULTED;
    if (DaclDefaulted) Sd->Control |= SE_DACL_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                             IN BOOLEAN SaclPresent,
                             IN PACL Sacl,
                             IN BOOLEAN SaclDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Fail on relative descriptors */
    if (Sd->Control & SE_SELF_RELATIVE) return STATUS_INVALID_SECURITY_DESCR;

    /* Is there a SACL? */
    if (!SaclPresent)
    {
        /* Caller is clearing the SACL, unset the flag and we're done */
        Sd->Control = Sd->Control & ~SE_SACL_PRESENT;
        return STATUS_SUCCESS;
    }

    /* Caller is setting a new SACL, set it and the flag */
    Sd->Sacl = Sacl;
    Sd->Control |= SE_SACL_PRESENT;

    /* Set if defaulted */
    Sd->Control &= ~SE_SACL_DEFAULTED;
    if (SaclDefaulted) Sd->Control |= SE_SACL_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID Owner,
                              IN BOOLEAN OwnerDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Fail on relative descriptors */
    if (Sd->Control & SE_SELF_RELATIVE) return STATUS_INVALID_SECURITY_DESCR;

    /* Owner being set or cleared */
    Sd->Owner = Owner;

    /* Set if defaulted */
    Sd->Control &= ~SE_OWNER_DEFAULTED;
    if (OwnerDefaulted) Sd->Control |= SE_OWNER_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID Group,
                              IN BOOLEAN GroupDefaulted)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Fail on relative descriptors */
    if (Sd->Control & SE_SELF_RELATIVE) return STATUS_INVALID_SECURITY_DESCR;

    /* Group being set or cleared */
    Sd->Group = Group;

    /* Set if defaulted */
    Sd->Control &= ~SE_GROUP_DEFAULTED;
    if (GroupDefaulted) Sd->Control |= SE_GROUP_DEFAULTED;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                OUT PSECURITY_DESCRIPTOR_CONTROL Control,
                                OUT PULONG Revision)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Read current revision, even if invalid */
    *Revision = Sd->Revision;

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Read current control */
    *Control = Sd->Control;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

    /* Check for invalid bits */
    if ((ControlBitsOfInterest & ~(SE_DACL_UNTRUSTED |
                                   SE_SERVER_SECURITY |
                                   SE_DACL_AUTO_INHERIT_REQ |
                                   SE_SACL_AUTO_INHERIT_REQ |
                                   SE_DACL_AUTO_INHERITED |
                                   SE_SACL_AUTO_INHERITED |
                                   SE_DACL_PROTECTED |
                                   SE_SACL_PROTECTED)) ||
        (ControlBitsToSet & ~ControlBitsOfInterest))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Zero the 'bits of interest' */
    Sd->Control &= ~ControlBitsOfInterest;

    /* Set the 'bits to set' */
    Sd->Control |= (ControlBitsToSet & ControlBitsOfInterest);

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  OUT PUCHAR RMControl)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Check if there's no valid RM control */
    if (!(Sd->Control & SE_RM_CONTROL_VALID))
    {
        /* Fail and return nothing */
        *RMControl = 0;
        return FALSE;
    }

    /* Return it, ironically the member is "should be zero" */
    *RMControl = Sd->Sbz1;
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  IN PUCHAR RMControl)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* RM Control is being cleared or set */
    if (!RMControl)
    {
        /* Clear it */
        Sd->Control &= ~SE_RM_CONTROL_VALID;
        Sd->Sbz1 = 0;
    }
    else
    {
        /* Set it */
        Sd->Control |= SE_RM_CONTROL_VALID;
        Sd->Sbz1 = *RMControl;
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                   IN SECURITY_DESCRIPTOR_CONTROL Control,
                                   OUT PULONG Revision)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PAGED_CODE_RTL();

    /* Always return revision, even if invalid */
    *Revision = Sd->Revision;

    /* Fail on invalid revision */
    if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) return STATUS_UNKNOWN_REVISION;

    /* Mask out flags which are not attributes */
    Control &= SE_DACL_UNTRUSTED |
               SE_SERVER_SECURITY |
               SE_DACL_AUTO_INHERIT_REQ |
               SE_SACL_AUTO_INHERIT_REQ |
               SE_DACL_AUTO_INHERITED |
               SE_SACL_AUTO_INHERITED |
               SE_DACL_PROTECTED |
               SE_SACL_PROTECTED;

    /* Call the newer API */
    return RtlSetControlSecurityDescriptor(SecurityDescriptor, Control, Control);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(IN PSECURITY_DESCRIPTOR pSourceSecurityDescriptor,
                          OUT PSECURITY_DESCRIPTOR *pDestinationSecurityDescriptor)
{
    PSID Owner, Group;
    PACL Dacl, Sacl;
    DWORD OwnerLength, GroupLength, DaclLength, SaclLength, TotalLength;
    PISECURITY_DESCRIPTOR Sd = pSourceSecurityDescriptor;

    /* Get all the components */
    RtlpQuerySecurityDescriptor(Sd,
                                &Owner,
                                &OwnerLength,
                                &Group,
                                &GroupLength,
                                &Dacl,
                                &DaclLength,
                                &Sacl,
                                &SaclLength);

    /* Add up their lengths */
    TotalLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                  OwnerLength +
                  GroupLength +
                  DaclLength +
                  SaclLength;

    /* Allocate a copy */
    *pDestinationSecurityDescriptor = RtlAllocateHeap(RtlGetProcessHeap(),
                                                      0,
                                                          TotalLength);
    if (*pDestinationSecurityDescriptor == NULL) return STATUS_NO_MEMORY;

    /* Copy the old in the new */
    RtlCopyMemory(*pDestinationSecurityDescriptor, Sd, TotalLength);

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
                            IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
                            IN PULONG BufferLength)
{
   PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)AbsoluteSecurityDescriptor;
   PAGED_CODE_RTL();

   /* Can't already be relative */
   if (Sd->Control & SE_SELF_RELATIVE) return STATUS_BAD_DESCRIPTOR_FORMAT;

   /* Call the other API */
   return RtlMakeSelfRelativeSD(AbsoluteSecurityDescriptor,
                                SelfRelativeSecurityDescriptor,
                                BufferLength);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(IN PSECURITY_DESCRIPTOR AbsoluteSD,
                      OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
                      IN OUT PULONG BufferLength)
{
    PSID Owner, Group;
    PACL Sacl, Dacl;
    ULONG OwnerLength, GroupLength, SaclLength, DaclLength, TotalLength;
    ULONG_PTR Current;
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)AbsoluteSD;
    PISECURITY_DESCRIPTOR_RELATIVE RelSd = (PISECURITY_DESCRIPTOR_RELATIVE)SelfRelativeSD;
    PAGED_CODE_RTL();

    /* Query all components */
    RtlpQuerySecurityDescriptor(Sd,
                                &Owner,
                                &OwnerLength,
                                &Group,
                                &GroupLength,
                                &Dacl,
                                &DaclLength,
                                &Sacl,
                                &SaclLength);

    /* Calculate final length */
    TotalLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                  OwnerLength +
                  GroupLength +
                  SaclLength +
                  DaclLength;

    /* Is there enough space? */
    if (*BufferLength < TotalLength)
    {
        /* Nope, return how much is needed */
        *BufferLength = TotalLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Start fresh */
    RtlZeroMemory(RelSd, TotalLength);

    /* Copy the header fields */
    RtlCopyMemory(RelSd,
                  Sd,
                  FIELD_OFFSET(SECURITY_DESCRIPTOR_RELATIVE, Owner));

    /* Set the current copy pointer */
    Current = (ULONG_PTR)(RelSd + 1);

    /* Is there a SACL? */
    if (SaclLength)
    {
        /* Copy it */
        RtlCopyMemory((PVOID)Current, Sacl, SaclLength);
        RelSd->Sacl = (ULONG_PTR)Current - (ULONG_PTR)RelSd;
        Current += SaclLength;
    }

    /* Is there a DACL? */
    if (DaclLength)
    {
        /* Copy it */
        RtlCopyMemory((PVOID)Current, Dacl, DaclLength);
        RelSd->Dacl = (ULONG_PTR)Current - (ULONG_PTR)RelSd;
        Current += DaclLength;
    }

    /* Is there an owner? */
    if (OwnerLength)
    {
        /* Copy it */
        RtlCopyMemory((PVOID)Current, Owner, OwnerLength);
        RelSd->Owner = (ULONG_PTR)Current - (ULONG_PTR)RelSd;
        Current += OwnerLength;
    }

    /* Is there a group? */
    if (GroupLength)
    {
        /* Copy it */
        RtlCopyMemory((PVOID)Current, Group, GroupLength);
        RelSd->Group = (ULONG_PTR)Current - (ULONG_PTR)RelSd;
    }

    /* Mark it as relative */
    RelSd->Control |= SE_SELF_RELATIVE;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(IN PSECURITY_DESCRIPTOR SelfRelativeSD,
                            OUT PSECURITY_DESCRIPTOR AbsoluteSD,
                            IN PULONG AbsoluteSDSize,
                            IN PACL Dacl,
                            IN PULONG DaclSize,
                            IN PACL Sacl,
                            IN PULONG SaclSize,
                            IN PSID Owner,
                            IN PULONG OwnerSize,
                            IN PSID PrimaryGroup,
                            IN PULONG PrimaryGroupSize)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)AbsoluteSD;
    PISECURITY_DESCRIPTOR RelSd = (PISECURITY_DESCRIPTOR)SelfRelativeSD;
    ULONG OwnerLength, GroupLength, DaclLength, SaclLength;
    PSID pOwner, pGroup;
    PACL pDacl, pSacl;
    PAGED_CODE_RTL();

    /* Must be relative, otherwiise fail */
    if (!(RelSd->Control & SE_SELF_RELATIVE)) return STATUS_BAD_DESCRIPTOR_FORMAT;

    /* Get all the components */
    RtlpQuerySecurityDescriptor(RelSd,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

    /* Fail if there's not enough space */
    if (!(Sd) ||
        (sizeof(SECURITY_DESCRIPTOR) > *AbsoluteSDSize) ||
        (OwnerLength > *OwnerSize) ||
        (GroupLength > *PrimaryGroupSize) ||
        (DaclLength > *DaclSize) ||
        (SaclLength > *SaclSize))
    {
        /* Return how much space is needed for each components */
        *AbsoluteSDSize = sizeof(SECURITY_DESCRIPTOR);
        *OwnerSize = OwnerLength;
        *PrimaryGroupSize = GroupLength;
        *DaclSize = DaclLength;
        *SaclSize = SaclLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy the header fields */
    RtlMoveMemory(Sd, RelSd, sizeof(SECURITY_DESCRIPTOR_RELATIVE));

    /* Wipe out the pointers and the relative flag */
    Sd->Owner = NULL;
    Sd->Group = NULL;
    Sd->Sacl = NULL;
    Sd->Dacl = NULL;
    Sd->Control &= ~SE_SELF_RELATIVE;

    /* Is there an owner? */
    if (pOwner)
    {
        /* Copy it */
        RtlMoveMemory(Owner, pOwner, RtlLengthSid(pOwner));
        Sd->Owner = Owner;
    }

    /* Is there a group? */
    if (pGroup)
    {
        /* Copy it */
        RtlMoveMemory(PrimaryGroup, pGroup, RtlLengthSid(pGroup));
        Sd->Group = PrimaryGroup;
    }

    /* Is there a DACL? */
    if (pDacl)
    {
        /* Copy it */
        RtlMoveMemory(Dacl, pDacl, pDacl->AclSize);
        Sd->Dacl = Dacl;
    }

    /* Is there a SACL? */
    if (pSacl)
    {
        /* Copy it */
        RtlMoveMemory(Sacl, pSacl, pSacl->AclSize);
        Sd->Sacl = Sacl;
    }

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD2(IN OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
                             OUT PULONG BufferSize)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SelfRelativeSD;
    PISECURITY_DESCRIPTOR_RELATIVE RelSd = (PISECURITY_DESCRIPTOR_RELATIVE)SelfRelativeSD;
    PVOID DataStart, DataEnd;
    LONG MoveDelta;
    ULONG DataSize, OwnerLength, GroupLength, DaclLength, SaclLength;
    PSID pOwner, pGroup;
    PACL pDacl, pSacl;
    PAGED_CODE_RTL();

    /* Need input */
    if (!RelSd) return STATUS_INVALID_PARAMETER_1;

    /* Need to know how much space we have */
    if (!BufferSize) return STATUS_INVALID_PARAMETER_2;

    /* Input must be relative */
    if (!(RelSd->Control & SE_SELF_RELATIVE)) return STATUS_BAD_DESCRIPTOR_FORMAT;

    /* Query all the component sizes */
    RtlpQuerySecurityDescriptor(Sd,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

    /*
     * Check if there's a difference in structure layout between relatiev and
     * absolute descriptors. On 32-bit, there won't be, since an offset is the
     * same size as a pointer (32-bit), but on 64-bit, the offsets remain 32-bit
     * as they are not SIZE_T, but ULONG, while the pointers now become 64-bit
     * and thus the structure is different */
    MoveDelta = sizeof(SECURITY_DESCRIPTOR) - sizeof(SECURITY_DESCRIPTOR_RELATIVE);
    if (!MoveDelta)
    {
        /* So on 32-bit, simply clear the flag... */
        Sd->Control &= ~SE_SELF_RELATIVE;

        /* Ensure we're *really* on 32-bit */
        ASSERT(sizeof(Sd->Owner) == sizeof(RelSd->Owner));
        ASSERT(sizeof(Sd->Group) == sizeof(RelSd->Group));
        ASSERT(sizeof(Sd->Sacl) == sizeof(RelSd->Sacl));
        ASSERT(sizeof(Sd->Dacl) == sizeof(RelSd->Dacl));

        /* And simply set pointers where there used to be offsets */
        Sd->Owner = pOwner;
        Sd->Group = pGroup;
        Sd->Sacl = pSacl;
        Sd->Dacl = pDacl;
        return STATUS_SUCCESS;
    }

    /*
     * Calculate the start and end of the data area, we simply just move the
     * data by the difference between the size of the relative and absolute
     * security descriptor structure
     */
    DataStart = pOwner;
    DataEnd = (PVOID)((ULONG_PTR)pOwner + OwnerLength);

    /* Is there a group? */
    if (pGroup)
    {
        /* Is the group higher than where we started? */
        if (((ULONG_PTR)pGroup < (ULONG_PTR)DataStart) || !DataStart)
        {
            /* Update the start pointer */
            DataStart = pGroup;
        }

        /* Is the group beyond where we ended? */
        if (((ULONG_PTR)pGroup + GroupLength > (ULONG_PTR)DataEnd) || !DataEnd)
        {
            /* Update the end pointer */
            DataEnd = (PVOID)((ULONG_PTR)pGroup + GroupLength);
        }
    }

    /* Is there a DACL? */
    if (pDacl)
    {
        /* Is the DACL higher than where we started? */
        if (((ULONG_PTR)pDacl < (ULONG_PTR)DataStart) || !DataStart)
        {
            /* Update the start pointer */
            DataStart = pDacl;
        }

        /* Is the DACL beyond where we ended? */
        if (((ULONG_PTR)pDacl + DaclLength > (ULONG_PTR)DataEnd) || !DataEnd)
        {
            /* Update the end pointer */
            DataEnd = (PVOID)((ULONG_PTR)pDacl + DaclLength);
        }
    }

    /* Is there a SACL? */
    if (pSacl)
    {
        /* Is the SACL higher than where we started? */
        if (((ULONG_PTR)pSacl < (ULONG_PTR)DataStart) || !DataStart)
        {
            /* Update the start pointer */
            DataStart = pSacl;
        }

        /* Is the SACL beyond where we ended? */
        if (((ULONG_PTR)pSacl + SaclLength > (ULONG_PTR)DataEnd) || !DataEnd)
        {
            /* Update the end pointer */
            DataEnd = (PVOID)((ULONG_PTR)pSacl + SaclLength);
        }
    }

    /* Sanity check */
    ASSERT((ULONG_PTR)DataEnd >= (ULONG_PTR)DataStart);

    /* Now compute the difference between relative and absolute */
    DataSize = (ULONG)((ULONG_PTR)DataEnd - (ULONG_PTR)DataStart);

    /* Is the new buffer large enough for this difference? */
    if (*BufferSize < sizeof(SECURITY_DESCRIPTOR) + DataSize)
    {
        /* Nope, bail out */
        *BufferSize = sizeof(SECURITY_DESCRIPTOR) + DataSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Is there anything actually to copy? */
    if (DataSize)
    {
        /*
         * There must be at least one SID or ACL in the security descriptor!
         * Also the data area must be located somewhere after the end of the
         * SECURITY_DESCRIPTOR_RELATIVE structure
         */
        ASSERT(DataStart != NULL);
        ASSERT((ULONG_PTR)DataStart >= (ULONG_PTR)(RelSd + 1));

        /* It's time to move the data */
        RtlMoveMemory((PVOID)(Sd + 1),
                      DataStart,
                      DataSize);
    }

    /* Is there an owner? */
    if (pOwner)
    {
        /* Set the pointer to the relative position */
        Sd->Owner = (PSID)((LONG_PTR)pOwner + MoveDelta);
    }
    else
    {
        /* No owner, clear the pointer */
        Sd->Owner = NULL;
    }

    /* Is there a group */
    if (pGroup)
    {
        /* Set the pointer to the relative position */
        Sd->Group = (PSID)((LONG_PTR)pGroup + MoveDelta);
    }
    else
    {
        /* No group, clear the pointer */
        Sd->Group = NULL;
    }

    /* Is there a SACL? */
    if (pSacl)
    {
        /* Set the pointer to the relative position */
        Sd->Sacl = (PACL)((LONG_PTR)pSacl + MoveDelta);
    }
    else
    {
        /* No SACL, clear the pointer */
        Sd->Sacl = NULL;
    }

    /* Is there a DACL? */
    if (pDacl)
    {
        /* Set the pointer to the relative position */
        Sd->Dacl = (PACL)((LONG_PTR)pDacl + MoveDelta);
    }
    else
    {
        /* No DACL, clear the pointer */
        Sd->Dacl = NULL;
    }

    /* Clear the self-relative flag */
    Sd->Control &= ~SE_SELF_RELATIVE;

    /* All good */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PISECURITY_DESCRIPTOR Sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    PSID Owner, Group;
    PACL Sacl, Dacl;
    PAGED_CODE_RTL();

    _SEH2_TRY
    {
        /* Fail on bad revisions */
        if (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) _SEH2_YIELD(return FALSE);

        /* Owner SID must be valid if present */
        Owner = SepGetOwnerFromDescriptor(Sd);
        if ((Owner) && (!RtlValidSid(Owner))) _SEH2_YIELD(return FALSE);

        /* Group SID must be valid if present */
        Group = SepGetGroupFromDescriptor(Sd);
        if ((Group) && (!RtlValidSid(Group))) _SEH2_YIELD(return FALSE);

        /* DACL must be valid if present */
        Dacl = SepGetDaclFromDescriptor(Sd);
        if ((Dacl) && (!RtlValidAcl(Dacl))) _SEH2_YIELD(return FALSE);

        /* SACL must be valid if present */
        Sacl = SepGetSaclFromDescriptor(Sd);
        if ((Sacl) && (!RtlValidAcl(Sacl))) _SEH2_YIELD(return FALSE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Access fault, bail out */
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    /* All good */
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
                                   IN ULONG SecurityDescriptorLength,
                                   IN SECURITY_INFORMATION RequiredInformation)
{
    PISECURITY_DESCRIPTOR_RELATIVE Sd = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptorInput;
    PSID Owner, Group;
    PACL Dacl, Sacl;
    ULONG Length;
    PAGED_CODE_RTL();

    /* Note that Windows allows no DACL/SACL even if RequiredInfo wants it */

    /* Do we have enough space, is the revision vaild, and is this SD relative? */
    if ((SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) ||
        (Sd->Revision != SECURITY_DESCRIPTOR_REVISION) ||
        !(Sd->Control & SE_SELF_RELATIVE))
    {
        /* Nope, bail out */
        return FALSE;
    }

    /* Is there an owner? */
    if (Sd->Owner)
    {
        /* Try to access it */
        if (!RtlpValidateSDOffsetAndSize(Sd->Owner,
                                         SecurityDescriptorLength,
                                         sizeof(SID),
                                         &Length))
        {
            /* It's beyond the buffer, fail */
            return FALSE;
        }

        /* Read the owner, check if it's valid and if the buffer contains it */
        Owner = (PSID)((ULONG_PTR)Sd->Owner + (ULONG_PTR)Sd);
        if (!RtlValidSid(Owner) || (Length < RtlLengthSid(Owner))) return FALSE;
    }
    else if (RequiredInformation & OWNER_SECURITY_INFORMATION)
    {
        /* No owner but the caller expects one, fail */
        return FALSE;
    }

    /* Is there a group? */
    if (Sd->Group)
    {
        /* Try to access it */
        if (!RtlpValidateSDOffsetAndSize(Sd->Group,
                                         SecurityDescriptorLength,
                                         sizeof(SID),
                                         &Length))
        {
            /* It's beyond the buffer, fail */
            return FALSE;
        }

        /* Read the group, check if it's valid and if the buffer contains it */
        Group = (PSID)((ULONG_PTR)Sd->Group + (ULONG_PTR)Sd);
        if (!RtlValidSid(Group) || (Length < RtlLengthSid(Group))) return FALSE;
    }
    else if (RequiredInformation & GROUP_SECURITY_INFORMATION)
    {
        /* No group, but the caller expects one, fail */
        return FALSE;
    }

    /* Is there a DACL? */
    if ((Sd->Control & SE_DACL_PRESENT) == SE_DACL_PRESENT)
    {
        /* Try to access it */
        if (!RtlpValidateSDOffsetAndSize(Sd->Dacl,
                                         SecurityDescriptorLength,
                                         sizeof(ACL),
                                         &Length))
        {
            /* It's beyond the buffer, fail */
            return FALSE;
        }

        /* Read the DACL, check if it's valid and if the buffer contains it */
        Dacl = (PSID)((ULONG_PTR)Sd->Dacl + (ULONG_PTR)Sd);
        if (!(RtlValidAcl(Dacl)) || (Length < Dacl->AclSize)) return FALSE;
    }

    /* Is there a SACL? */
    if ((Sd->Control & SE_SACL_PRESENT) == SE_SACL_PRESENT)
    {
        /* Try to access it */
        if (!RtlpValidateSDOffsetAndSize(Sd->Sacl,
                                         SecurityDescriptorLength,
                                         sizeof(ACL),
                                         &Length))
        {
            /* It's beyond the buffer, fail */
            return FALSE;
        }

        /* Read the SACL, check if it's valid and if the buffer contains it */
        Sacl = (PSID)((ULONG_PTR)Sd->Sacl + (ULONG_PTR)Sd);
        if (!(RtlValidAcl(Sacl)) || (Length < Sacl->AclSize)) return FALSE;
    }

    /* All good */
    return TRUE;
}

/* EOF */
