/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

/*
* @implemented
*/
NTSTATUS STDCALL
RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                            ULONG Revision)
{
   if (Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   SecurityDescriptor->Revision = Revision;
   SecurityDescriptor->Sbz1 = 0;
   SecurityDescriptor->Control = 0;
   SecurityDescriptor->Owner = NULL;
   SecurityDescriptor->Group = NULL;
   SecurityDescriptor->Sacl = NULL;
   SecurityDescriptor->Dacl = NULL;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   ULONG Length = sizeof(SECURITY_DESCRIPTOR);

   if (SecurityDescriptor->Owner != NULL)
   {
      PSID Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Owner = (PSID)((ULONG_PTR)Owner + (ULONG_PTR)SecurityDescriptor);
      }
      Length = Length + ROUND_UP(RtlLengthSid(Owner), 4);
   }

   if (SecurityDescriptor->Group != NULL)
   {
      PSID Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Group = (PSID)((ULONG_PTR)Group + (ULONG_PTR)SecurityDescriptor);
      }
      Length = Length + ROUND_UP(RtlLengthSid(Group), 4);
   }

   if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
         SecurityDescriptor->Dacl != NULL)
   {
      PACL Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Dacl = (PACL)((ULONG_PTR)Dacl + (ULONG_PTR)SecurityDescriptor);
      }
      Length = Length + ROUND_UP(Dacl->AclSize, 4);
   }

   if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
         SecurityDescriptor->Sacl != NULL)
   {
      PACL Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Sacl = (PACL)((ULONG_PTR)Sacl + (ULONG_PTR)SecurityDescriptor);
      }
      Length = Length + ROUND_UP(Sacl->AclSize, 4);
   }

   return Length;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PBOOLEAN DaclPresent,
                             PACL* Dacl,
                             PBOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(SecurityDescriptor->Control & SE_DACL_PRESENT))
   {
      *DaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *DaclPresent = TRUE;

   if (SecurityDescriptor->Dacl == NULL)
   {
      *Dacl = NULL;
   }
   else
   {
      *Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Dacl = (PACL)((ULONG_PTR)*Dacl + (ULONG_PTR)SecurityDescriptor);
      }
   }

   if (SecurityDescriptor->Control & SE_DACL_DEFAULTED)
   {
      *DaclDefaulted = TRUE;
   }
   else
   {
      *DaclDefaulted = FALSE;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             BOOLEAN DaclPresent,
                             PACL Dacl,
                             BOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!DaclPresent)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_PRESENT);
      return STATUS_SUCCESS;
   }

   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_PRESENT;
   SecurityDescriptor->Dacl = Dacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_DEFAULTED);

   if (DaclDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return FALSE;
   }

   if (SecurityDescriptor->Owner != NULL)
   {
      PSID Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Owner = (PSID)((ULONG_PTR)Owner + (ULONG_PTR)SecurityDescriptor);
      }

      if (!RtlValidSid(Owner))
      {
         return FALSE;
      }
   }

   if (SecurityDescriptor->Group != NULL)
   {
      PSID Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Group = (PSID)((ULONG_PTR)Group + (ULONG_PTR)SecurityDescriptor);
      }

      if (!RtlValidSid(Group))
      {
         return FALSE;
      }
   }

   if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
       SecurityDescriptor->Dacl != NULL)
   {
      PACL Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Dacl = (PACL)((ULONG_PTR)Dacl + (ULONG_PTR)SecurityDescriptor);
      }

      if (!RtlValidAcl(Dacl))
      {
         return FALSE;
      }
   }

   if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
       SecurityDescriptor->Sacl != NULL)
   {
      PACL Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         Sacl = (PACL)((ULONG_PTR)Sacl + (ULONG_PTR)SecurityDescriptor);
      }

      if (!RtlValidAcl(Sacl))
      {
         return FALSE;
      }
   }

   return TRUE;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID Owner,
                              BOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   SecurityDescriptor->Owner = Owner;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_OWNER_DEFAULTED);

   if (OwnerDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_OWNER_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID* Owner,
                              PBOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Owner != NULL)
   {
      *Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Owner = (PSID)((ULONG_PTR)*Owner + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Owner = NULL;
   }

   if (SecurityDescriptor->Control & SE_OWNER_DEFAULTED)
   {
      *OwnerDefaulted = TRUE;
   }
   else
   {
      *OwnerDefaulted = FALSE;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID Group,
                              BOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   SecurityDescriptor->Group = Group;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_GROUP_DEFAULTED);
   if (GroupDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_GROUP_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID* Group,
                              PBOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Group != NULL)
   {
      *Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Group = (PSID)((ULONG_PTR)*Group + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Group = NULL;
   }

   if (SecurityDescriptor->Control & SE_GROUP_DEFAULTED)
   {
      *GroupDefaulted = TRUE;
   }
   else
   {
      *GroupDefaulted = FALSE;
   }

   return STATUS_SUCCESS;
}


static VOID
RtlpQuerySecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                            PSID* Owner,
                            PULONG OwnerLength,
                            PSID* Group,
                            PULONG GroupLength,
                            PACL* Dacl,
                            PULONG DaclLength,
                            PACL* Sacl,
                            PULONG SaclLength)
{
   if (SecurityDescriptor->Owner != NULL)
   {
      *Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Owner = (PSID)((ULONG_PTR)*Owner + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Owner = NULL;
   }

   if (*Owner != NULL)
   {
      *OwnerLength = ROUND_UP(RtlLengthSid(*Owner), 4);
   }
   else
   {
      *OwnerLength = 0;
   }

   if ((SecurityDescriptor->Control & SE_DACL_PRESENT) &&
         SecurityDescriptor->Dacl != NULL)
   {
      *Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Dacl = (PACL)((ULONG_PTR)*Dacl + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Dacl = NULL;
   }

   if (*Dacl != NULL)
   {
      *DaclLength = ROUND_UP((*Dacl)->AclSize, 4);
   }
   else
   {
      *DaclLength = 0;
   }

   if (SecurityDescriptor->Group != NULL)
   {
      *Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Group = (PSID)((ULONG_PTR)*Group + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Group = NULL;
   }

   if (*Group != NULL)
   {
      *GroupLength = ROUND_UP(RtlLengthSid(*Group), 4);
   }
   else
   {
      *GroupLength = 0;
   }

   if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
         SecurityDescriptor->Sacl != NULL)
   {
      *Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Sacl = (PACL)((ULONG_PTR)*Sacl + (ULONG_PTR)SecurityDescriptor);
      }
   }
   else
   {
      *Sacl = NULL;
   }

   if (*Sacl != NULL)
   {
      *SaclLength = ROUND_UP((*Sacl)->AclSize, 4);
   }
   else
   {
      *SaclLength = 0;
   }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
                      PSECURITY_DESCRIPTOR RelSD,
                      PULONG BufferLength)
{
   PSID Owner;
   PSID Group;
   PACL Sacl;
   PACL Dacl;
   ULONG OwnerLength;
   ULONG GroupLength;
   ULONG SaclLength;
   ULONG DaclLength;
   ULONG TotalLength;
   ULONG_PTR Current;

   RtlpQuerySecurityDescriptor(AbsSD,
                               &Owner,
                               &OwnerLength,
                               &Group,
                               &GroupLength,
                               &Dacl,
                               &DaclLength,
                               &Sacl,
                               &SaclLength);

   TotalLength = OwnerLength + GroupLength + SaclLength + DaclLength + sizeof(SECURITY_DESCRIPTOR);
   if (*BufferLength < TotalLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   RtlZeroMemory(RelSD,
                 TotalLength);
   memmove(RelSD,
           AbsSD,
           sizeof(SECURITY_DESCRIPTOR));
   Current = (ULONG_PTR)RelSD + sizeof(SECURITY_DESCRIPTOR);

   if (SaclLength != 0)
   {
      memmove((PVOID)Current,
              Sacl,
              SaclLength);
      RelSD->Sacl = (PACL)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += SaclLength;
   }

   if (DaclLength != 0)
   {
      memmove((PVOID)Current,
              Dacl,
              DaclLength);
      RelSD->Dacl = (PACL)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += DaclLength;
   }

   if (OwnerLength != 0)
   {
      memmove((PVOID)Current,
              Owner,
              OwnerLength);
      RelSD->Owner = (PSID)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += OwnerLength;
   }

   if (GroupLength != 0)
   {
      memmove((PVOID)Current,
              Group,
              GroupLength);
      RelSD->Group = (PSID)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
   }

   RelSD->Control |= SE_SELF_RELATIVE;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
                            PSECURITY_DESCRIPTOR RelSD,
                            PULONG BufferLength)
{
   if (AbsSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   return RtlMakeSelfRelativeSD(AbsSD, RelSD, BufferLength);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                PSECURITY_DESCRIPTOR_CONTROL Control,
                                PULONG Revision)
{
   *Revision = SecurityDescriptor->Revision;

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   *Control = SecurityDescriptor->Control;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
  {
    return STATUS_UNKNOWN_REVISION;
  }

  /* Zero the 'bits of interest' */
  SecurityDescriptor->Control &= ~ControlBitsOfInterest;

  /* Set the 'bits to set' */
  SecurityDescriptor->Control |= (ControlBitsToSet & ControlBitsOfInterest);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PBOOLEAN SaclPresent,
                             PACL *Sacl,
                             PBOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(SecurityDescriptor->Control & SE_SACL_PRESENT))
   {
      *SaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *SaclPresent = TRUE;

   if (SecurityDescriptor->Sacl == NULL)
   {
      *Sacl = NULL;
   }
   else
   {
      *Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
      {
         *Sacl = (PACL)((ULONG_PTR)*Sacl + (ULONG_PTR)SecurityDescriptor);
      }
   }

   if (SecurityDescriptor->Control & SE_SACL_DEFAULTED)
   {
      *SaclDefaulted = TRUE;
   }
   else
   {
      *SaclDefaulted = FALSE;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             BOOLEAN SaclPresent,
                             PACL Sacl,
                             BOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!SaclPresent)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_PRESENT);
      return STATUS_SUCCESS;
   }

   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_PRESENT;
   SecurityDescriptor->Sacl = Sacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_DEFAULTED);

   if (SaclDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD(PSECURITY_DESCRIPTOR RelSD,
                            PSECURITY_DESCRIPTOR AbsSD,
                            PDWORD AbsSDSize,
                            PACL Dacl,
                            PDWORD DaclSize,
                            PACL Sacl,
                            PDWORD SaclSize,
                            PSID Owner,
                            PDWORD OwnerSize,
                            PSID Group,
                            PDWORD GroupSize)
{
   ULONG OwnerLength;
   ULONG GroupLength;
   ULONG DaclLength;
   ULONG SaclLength;
   PSID pOwner;
   PSID pGroup;
   PACL pDacl;
   PACL pSacl;

   if (!(RelSD->Control & SE_SELF_RELATIVE))
      return STATUS_BAD_DESCRIPTOR_FORMAT;

   RtlpQuerySecurityDescriptor (RelSD,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

   if (OwnerLength > *OwnerSize ||
       GroupLength > *GroupSize ||
       DaclLength > *DaclSize ||
       SaclLength > *SaclSize)
      return STATUS_BUFFER_TOO_SMALL;

   memmove (Owner, pOwner, OwnerLength);
   memmove (Group, pGroup, GroupLength);
   memmove (Dacl, pDacl, DaclLength);
   memmove (Sacl, pSacl, SaclLength);

   memmove (AbsSD, RelSD, sizeof (SECURITY_DESCRIPTOR));

   AbsSD->Control &= ~SE_SELF_RELATIVE;
   AbsSD->Owner = Owner;
   AbsSD->Group = Group;
   AbsSD->Dacl = Dacl;
   AbsSD->Sacl = Sacl;

   *OwnerSize = OwnerLength;
   *GroupSize = GroupLength;
   *DaclSize = DaclLength;
   *SaclSize = SaclLength;

   return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD2(PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
                             PULONG BufferSize)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidRelativeSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
                                   IN ULONG SecurityDescriptorLength,
                                   IN SECURITY_INFORMATION RequiredInformation)
{
   if (SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR) ||
       SecurityDescriptorInput->Revision != SECURITY_DESCRIPTOR_REVISION1 ||
       !(SecurityDescriptorInput->Control & SE_SELF_RELATIVE))
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Owner != NULL)
   {
      PSID Owner = (PSID)((ULONG_PTR)SecurityDescriptorInput->Owner + (ULONG_PTR)SecurityDescriptorInput);
      if (!RtlValidSid(Owner))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & OWNER_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Group != NULL)
   {
      PSID Group = (PSID)((ULONG_PTR)SecurityDescriptorInput->Group + (ULONG_PTR)SecurityDescriptorInput);
      if (!RtlValidSid(Group))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & GROUP_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Control & SE_DACL_PRESENT)
   {
      if (SecurityDescriptorInput->Dacl != NULL &&
          !RtlValidAcl((PACL)((ULONG_PTR)SecurityDescriptorInput->Dacl + (ULONG_PTR)SecurityDescriptorInput)))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & DACL_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Control & SE_SACL_PRESENT)
   {
      if (SecurityDescriptorInput->Sacl != NULL &&
          !RtlValidAcl((PACL)((ULONG_PTR)SecurityDescriptorInput->Sacl + (ULONG_PTR)SecurityDescriptorInput)))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & SACL_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlGetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  PUCHAR RMControl)
{
  if (!(SecurityDescriptor->Control & SE_RM_CONTROL_VALID))
  {
    *RMControl = 0;
    return FALSE;
  }

  *RMControl = SecurityDescriptor->Sbz1;

  return TRUE;
}


/*
 * @implemented
 */
VOID STDCALL
RtlSetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  PUCHAR RMControl)
{
  if (RMControl == NULL)
  {
    SecurityDescriptor->Control &= ~SE_RM_CONTROL_VALID;
    SecurityDescriptor->Sbz1 = 0;
  }
  else
  {
    SecurityDescriptor->Control |= SE_RM_CONTROL_VALID;
    SecurityDescriptor->Sbz1 = *RMControl;
  }
}

/* EOF */
