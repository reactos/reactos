/* $Id: sd.c,v 1.5 2001/12/05 01:40:25 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			    ULONG Revision)
{
  if (Revision != 1)
    return(STATUS_UNSUCCESSFUL);

  SecurityDescriptor->Revision = 1;
  SecurityDescriptor->Sbz1 = 0;
  SecurityDescriptor->Control = 0;
  SecurityDescriptor->Owner = NULL;
  SecurityDescriptor->Group = NULL;
  SecurityDescriptor->Sacl = NULL;
  SecurityDescriptor->Dacl = NULL;

  return(STATUS_SUCCESS);
}

ULONG STDCALL
RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PSID Owner;
   PSID Group;
   ULONG Length;
   PACL Dacl;
   PACL Sacl;
   
   Length = sizeof(SECURITY_DESCRIPTOR);
   
   if (SecurityDescriptor->Owner != NULL)
     {
	Owner = SecurityDescriptor->Owner;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Owner = (PSID)((ULONG)Owner + 
			    (ULONG)SecurityDescriptor);
	  }
	Length = Length + ((sizeof(SID) + (Owner->SubAuthorityCount - 1) * 
			   sizeof(ULONG) + 3) & 0xfc);
     }
   if (SecurityDescriptor->Group != NULL)
     {
	Group = SecurityDescriptor->Group;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Group = (PSID)((ULONG)Group + (ULONG)SecurityDescriptor);
	  }
	Length = Length + ((sizeof(SID) + (Group->SubAuthorityCount - 1) *
			   sizeof(ULONG) + 3) & 0xfc);
     }
      if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
       SecurityDescriptor->Dacl != NULL)
     {
	Dacl = SecurityDescriptor->Dacl;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Dacl = (PACL)((ULONG)Dacl + (PVOID)SecurityDescriptor);
	  }
	Length = Length + ((Dacl->AclSize + 3) & 0xfc);
     }
   if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
       SecurityDescriptor->Sacl != NULL)
     {
	Sacl = SecurityDescriptor->Sacl;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Sacl = (PACL)((ULONG)Sacl + (PVOID)SecurityDescriptor);
	  }
	Length = Length + ((Sacl->AclSize + 3) & 0xfc);
     }
   return(Length);
}


NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN DaclPresent,
			     PACL* Dacl,
			     PBOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!(SecurityDescriptor->Control & SE_DACL_PRESENT))
     {
	*DaclPresent = 0;
	return(STATUS_SUCCESS);
     }
   *DaclPresent = 1;
   if (SecurityDescriptor->Dacl == NULL)
     {
	*Dacl = NULL;
     }
   else
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Dacl = (PACL)((ULONG)SecurityDescriptor->Dacl +
			    (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Dacl = SecurityDescriptor->Dacl;
	  }
     }
   if (SecurityDescriptor->Control & SE_DACL_DEFAULTED)
     {
	*DaclDefaulted = 1;
     }
   else
     {
	*DaclDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN DaclPresent,
			     PACL Dacl,
			     BOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!DaclPresent)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_PRESENT);
	return(STATUS_SUCCESS);
     }
   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_PRESENT;
   SecurityDescriptor->Dacl = Dacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_DEFAULTED);
   if (DaclDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


BOOLEAN STDCALL
RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Owner,
			      BOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   SecurityDescriptor->Owner = Owner;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_OWNER_DEFAULTED);
   if (OwnerDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_OWNER_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Owner,
			      PBOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Owner != NULL)
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Owner = (PSID)((ULONG)SecurityDescriptor->Owner +
			     (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Owner = SecurityDescriptor->Owner;
	  }
     }
   else
     {
	*Owner = NULL;
     }
   if (SecurityDescriptor->Control & SE_OWNER_DEFAULTED)
     {
	*OwnerDefaulted = 1;
     }
   else
     {
	*OwnerDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Group,
			      BOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   SecurityDescriptor->Group = Group;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_GROUP_DEFAULTED);
   if (GroupDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_GROUP_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Group,
			      PBOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Group != NULL)
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Group = (PSID)((ULONG)SecurityDescriptor->Group +
			     (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Group = SecurityDescriptor->Group;
	  }
     }
   else
     {
	*Group = NULL;
     }
   if (SecurityDescriptor->Control & SE_GROUP_DEFAULTED)
     {
	*GroupDefaulted = 1;
     }
   else
     {
	*GroupDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN SaclPresent,
			     PACL *Sacl,
			     PBOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!(SecurityDescriptor->Control & SE_SACL_PRESENT))
     {
	*SaclPresent = 0;
	return(STATUS_SUCCESS);
     }
   *SaclPresent = 1;
   if (SecurityDescriptor->Sacl == NULL)
     {
	*Sacl = NULL;
     }
   else
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Sacl = (PACL)((ULONG)SecurityDescriptor->Sacl +
			    (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Sacl = SecurityDescriptor->Sacl;
	  }
     }
   if (SecurityDescriptor->Control & SE_SACL_DEFAULTED)
     {
	*SaclDefaulted = 1;
     }
   else
     {
	*SaclDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN SaclPresent,
			     PACL Sacl,
			     BOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!SaclPresent)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_PRESENT);
	return(STATUS_SUCCESS);
     }
   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_PRESENT;
   SecurityDescriptor->Sacl = Sacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_DEFAULTED);
   if (SaclDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
			    PSECURITY_DESCRIPTOR RelSD,
			    PULONG BufferLength)
{
  if (AbsSD->Control & SE_SELF_RELATIVE)
    return(STATUS_BAD_DESCRIPTOR_FORMAT);

//  return(RtlPMakeSelfRelativeSD (AbsSD, RelSD, BufferLength));

  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}



/* EOF */
