/* $Id: acl.c,v 1.5 2002/02/20 20:15:38 ekohl Exp $
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
#include <internal/se.h>

#include <internal/debug.h>

#define TAG_ACL    TAG('A', 'C', 'L', 'T')


/* GLOBALS ******************************************************************/

PACL EXPORTED SePublicDefaultDacl = NULL;
PACL EXPORTED SeSystemDefaultDacl = NULL;

PACL SePublicDefaultUnrestrictedDacl = NULL;
PACL SePublicOpenDacl = NULL;
PACL SePublicOpenUnrestrictedDacl = NULL;
PACL SeUnrestrictedDacl = NULL;


/* FUNCTIONS ****************************************************************/

BOOLEAN
SepInitDACLs(VOID)
{
  ULONG AclLength2;
  ULONG AclLength3;
  ULONG AclLength4;

  AclLength2 = sizeof(ACL) +
	       2 * (RtlLengthRequiredSid(1) + sizeof(ACE_HEADER));
  AclLength3 = sizeof(ACL) +
	       3 * (RtlLengthRequiredSid(1) + sizeof(ACE_HEADER));
  AclLength4 = sizeof(ACL) +
	       4 * (RtlLengthRequiredSid(1) + sizeof(ACE_HEADER));

  /* create PublicDefaultDacl */
  SePublicDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength2,
					      TAG_ACL);
  if (SePublicDefaultDacl == NULL)
    return(FALSE);

  RtlCreateAcl(SePublicDefaultDacl,
	       AclLength2,
	       2);

  RtlAddAccessAllowedAce(SePublicDefaultDacl,
			 2,
			 GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicDefaultDacl,
			 2,
			 GENERIC_ALL,
			 SeLocalSystemSid);


  /* create PublicDefaultUnrestrictedDacl */
  SePublicDefaultUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
							  AclLength4,
							  TAG_ACL);
  if (SePublicDefaultUnrestrictedDacl == NULL)
    return(FALSE);

  RtlCreateAcl(SePublicDefaultUnrestrictedDacl,
	       AclLength4,
	       2);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 4,
			 GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 4,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 4,
			 GENERIC_ALL,
			 SeAliasAdminsSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 4,
			 GENERIC_READ | GENERIC_EXECUTE | STANDARD_RIGHTS_READ,
			 SeRestrictedCodeSid);

  /* create PublicOpenDacl */
  SePublicOpenDacl = ExAllocatePoolWithTag(NonPagedPool,
					   AclLength3,
					   TAG_ACL);
  if (SePublicOpenDacl == NULL)
    return(FALSE);

  RtlCreateAcl(SePublicOpenDacl,
	       AclLength3,
	       3);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 2,
			 GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 2,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 2,
			 GENERIC_ALL,
			 SeAliasAdminsSid);


  return(TRUE);
}


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

NTSTATUS RtlpAddKnownAce(PACL Acl,
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

NTSTATUS STDCALL
RtlAddAccessAllowedAce(PACL Acl,
		       ULONG Revision,
		       ACCESS_MASK AccessMask,
		       PSID Sid)
{
   return(RtlpAddKnownAce(Acl, Revision, AccessMask, Sid, 0));
}

NTSTATUS STDCALL
RtlAddAce(PACL Acl,
	  ULONG AclRevision,
	  ULONG StartingIndex,
	  PACE AceList,
	  ULONG AceListLength)
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


NTSTATUS STDCALL
RtlCreateAcl(PACL Acl,
	     ULONG AclSize,
	     ULONG AclRevision)
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

/* EOF */
