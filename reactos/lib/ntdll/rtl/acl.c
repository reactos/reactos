/* $Id: acl.c,v 1.3 2000/06/29 23:35:28 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/acl.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN
STDCALL
RtlFirstFreeAce(PACL Acl, PACE* Ace)
{
   PACE Current;
   PVOID AclEnd;
   ULONG i;

   Current = (PACE)(Acl + 1);
   *Ace = NULL;
   i = 0;
   if (Acl->AceCount == 0)
     {
	*Ace = Current;
	return(TRUE);
     }
   AclEnd = Acl->AclSize + Acl;
   do
     {
	if ((PVOID)Current >= AclEnd)
	  {
	     return(FALSE);
	  }
	if (Current->Header.AceType == 4)
	  {
	     if (Acl->AclRevision < 3)
	       {
		  return(FALSE);
	       }
	  }
	Current = (PACE)((PVOID)Current + (ULONG)Current->Header.AceSize);
	i++;
     } while (i < Acl->AceCount);
   if ((PVOID)Current >= AclEnd)
     {
	return(FALSE);
     }
   *Ace = Current;
   return(TRUE);
}

NTSTATUS
STDCALL
RtlGetAce(PACL Acl, ULONG AceIndex, PACE *Ace)
{
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS
RtlpAddKnownAce(PACL Acl,
		ULONG Revision,
		ACCESS_MASK AccessMask,
		PSID Sid,
		ULONG Type)
{
   PACE Ace;
   
   if (!RtlValidSid(Sid))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Acl->AclRevision > 3 ||
       Revision > 3)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Revision < Acl->AclRevision)
     {
	Revision = Acl->AclRevision;
     }
   if (!RtlFirstFreeAce(Acl, &Ace))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Ace == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (((PVOID)Ace + RtlLengthSid(Sid) + sizeof(ACE)) >= 
       ((PVOID)Acl + Acl->AclSize))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   Ace->Header.AceFlags = 0;
   Ace->Header.AceType = Type;
   Ace->Header.AceSize = RtlLengthSid(Sid) + sizeof(ACE);
   Ace->Header.AccessMask = AccessMask;
   RtlCopySid(RtlLengthSid(Sid), (PSID)Ace + 1, Sid);
   Acl->AceCount++;
   Acl->AclRevision = Revision;
   return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
RtlAddAccessAllowedAce (
	PACL		Acl,
	ULONG		Revision,
	ACCESS_MASK	AccessMask,
	PSID		Sid
	)
{
   return(RtlpAddKnownAce(Acl, Revision, AccessMask, Sid, 0));
}

NTSTATUS
STDCALL
RtlAddAccessDeniedAce (
	PACL		Acl,
	ULONG		Revision,
	ACCESS_MASK	AccessMask,
	PSID		Sid
	)
{
   return(RtlpAddKnownAce(Acl, Revision, AccessMask, Sid, 1));
}

NTSTATUS
STDCALL
RtlAddAce (
	PACL	Acl,
	ULONG	AclRevision,
	ULONG	StartingIndex,
	PACE	AceList,
	ULONG	AceListLength
	)
{
   PACE Ace;
   ULONG i;
   PACE Current;
   ULONG j;
   
   if (Acl->AclRevision != 2 &&
       Acl->AclRevision != 3)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!RtlFirstFreeAce(Acl,&Ace))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Acl->AclRevision <= AclRevision)
     {
	AclRevision = Acl->AclRevision;
     }
   if (((PVOID)AceList + AceListLength) <= (PVOID)AceList)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   i = 0;
   Current = (PACE)(Acl + 1);
   while ((PVOID)Current < ((PVOID)AceList + AceListLength))
     {
	if (AceList->Header.AceType == 4 &&
	    AclRevision < 3)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	Current = (PACE)((PVOID)Current + Current->Header.AceSize);
     }
   if (Ace == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (((PVOID)Ace + AceListLength) >= ((PVOID)Acl + Acl->AclSize))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (StartingIndex != 0)
     {
	if (Acl->AceCount > 0)
	  {
	     Current = (PACE)(Acl + 1);
	     for (j = 0; j < StartingIndex; j++)
	       {
		  Current = (PACE)((PVOID)Current + Current->Header.AceSize);
	       }
	  }
     }
   /* RtlpAddData(AceList, AceListLength, Current, (PVOID)Ace - Current)); */
   memcpy(Current, AceList, AceListLength);
   Acl->AceCount = Acl->AceCount + i;
   Acl->AclRevision = AclRevision;
   return(TRUE);
}

NTSTATUS
STDCALL
RtlAddAuditAccessAce (
	PACL		Acl,
	ULONG		Revision,
	ACCESS_MASK	AccessMask,
	PSID		Sid,
	BOOLEAN		Success,
	BOOLEAN		Failure
	)
{
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDCALL
RtlDeleteAce(PACL Acl, ULONG AceIndex)
{
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDCALL
RtlCreateAcl(PACL Acl, ULONG AclSize, ULONG AclRevision)
{
   if (AclSize < 8)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (AclRevision != 2 ||
       AclRevision != 3)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (AclSize > 0xffff)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   AclSize = AclSize & ~(0x3);
   Acl->AclSize = AclSize;
   Acl->AclRevision = AclRevision;
   Acl->AceCount = 0;
   Acl->Sbz1 = 0;
   Acl->Sbz2 = 0;
   return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
RtlQueryInformationAcl (
	PACL			Acl,
	PVOID			Information,
	ULONG			InformationLength,
	ACL_INFORMATION_CLASS	InformationClass
	)
{
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDCALL
RtlSetInformationAcl (
	PACL			Acl,
	PVOID			Information,
	ULONG			InformationLength,
	ACL_INFORMATION_CLASS	InformationClass
	)
{
   UNIMPLEMENTED;
   return STATUS_UNSUCCESSFUL;
}

BOOLEAN
STDCALL
RtlValidAcl (PACL Acl)
{
   UNIMPLEMENTED;
   return FALSE;
}

/* EOF */
