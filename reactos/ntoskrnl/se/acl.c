/* $Id: acl.c,v 1.16 2004/02/02 12:05:41 ekohl Exp $
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

BOOLEAN INIT_FUNCTION
SepInitDACLs(VOID)
{
  ULONG AclLength2;
  ULONG AclLength3;
  ULONG AclLength4;

  AclLength2 = sizeof(ACL) +
	       2 * (RtlLengthRequiredSid(1) + sizeof(ACE));
  AclLength3 = sizeof(ACL) +
	       3 * (RtlLengthRequiredSid(1) + sizeof(ACE));
  AclLength4 = sizeof(ACL) +
	       4 * (RtlLengthRequiredSid(1) + sizeof(ACE));

  /* create PublicDefaultDacl */
  SePublicDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength2,
					      TAG_ACL);
  if (SePublicDefaultDacl == NULL)
    return(FALSE);

  RtlCreateAcl(SePublicDefaultDacl,
	       AclLength2,
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
  SePublicDefaultUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
							  AclLength4,
							  TAG_ACL);
  if (SePublicDefaultUnrestrictedDacl == NULL)
    return(FALSE);

  RtlCreateAcl(SePublicDefaultUnrestrictedDacl,
	       AclLength4,
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


  return(TRUE);
}


BOOLEAN STDCALL
RtlFirstFreeAce(PACL Acl,
		PACE* Ace)
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

  AclEnd = Acl->AclSize + (char*)Acl;
  do
    {
      if ((PVOID)Current >= AclEnd)
	{
	  return(FALSE);
	}

      if (Current->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE &&
	  Acl->AclRevision < ACL_REVISION3)
	{
	  return(FALSE);
	}
      Current = (PACE)((char*)Current + (ULONG)Current->Header.AceSize);
      i++;
    }
  while (i < Acl->AceCount);

  if ((PVOID)Current < AclEnd)
    {
      *Ace = Current;
    }

  return(TRUE);
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
      return(STATUS_INVALID_SID);
    }
  if (Acl->AclRevision > MAX_ACL_REVISION ||
      Revision > MAX_ACL_REVISION)
    {
      return(STATUS_UNKNOWN_REVISION);
    }
  if (Revision < Acl->AclRevision)
    {
      Revision = Acl->AclRevision;
    }
  if (!RtlFirstFreeAce(Acl, &Ace))
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  if (Ace == NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  if (((char*)Ace + RtlLengthSid(Sid) + sizeof(ACE)) >= 
      ((char*)Acl + Acl->AclSize))
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  Ace->Header.AceFlags = 0;
  Ace->Header.AceType = Type;
  Ace->Header.AceSize = RtlLengthSid(Sid) + sizeof(ACE);
  Ace->AccessMask = AccessMask;
  RtlCopySid(RtlLengthSid(Sid), (PSID)(Ace + 1), Sid);
  Acl->AceCount++;
  Acl->AclRevision = Revision;
  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAddAccessAllowedAce (PACL Acl,
			ULONG Revision,
			ACCESS_MASK AccessMask,
			PSID Sid)
{
  return RtlpAddKnownAce (Acl,
			  Revision,
			  AccessMask,
			  Sid,
			  ACCESS_ALLOWED_ACE_TYPE);
}


/*
 * @implemented
 */
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

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION)
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
   if (((char*)AceList + AceListLength) <= (char*)AceList)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   i = 0;
   Current = (PACE)(Acl + 1);
   while ((char*)Current < ((char*)AceList + AceListLength))
     {
	if (AceList->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE &&
	    AclRevision < ACL_REVISION3)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	Current = (PACE)((char*)Current + Current->Header.AceSize);
     }
   if (Ace == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (((char*)Ace + AceListLength) >= ((char*)Acl + Acl->AclSize))
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
		  Current = (PACE)((char*)Current + Current->Header.AceSize);
	       }
	  }
     }
   /* RtlpAddData(AceList, AceListLength, Current, (PVOID)Ace - Current)); */
   memcpy(Current, AceList, AceListLength);
   Acl->AceCount = Acl->AceCount + i;
   Acl->AclRevision = AclRevision;
   return(TRUE);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCreateAcl(PACL Acl,
	     ULONG AclSize,
	     ULONG AclRevision)
{
  if (AclSize < 8)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  if (Acl->AclRevision < MIN_ACL_REVISION ||
      Acl->AclRevision > MAX_ACL_REVISION)
    {
      return(STATUS_UNKNOWN_REVISION);
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


BOOLEAN STDCALL
RtlValidAcl(PACL Acl)
{
  PACE Ace;
  USHORT Size;

  if (Acl->AclRevision < MIN_ACL_REVISION ||
      Acl->AclRevision > MAX_ACL_REVISION)
    {
      return(FALSE);
    }

  Size = (Acl->AclSize + 3) & ~3;
  if (Size != Acl->AclSize)
    {
      return(FALSE);
    }

  return(RtlFirstFreeAce(Acl, &Ace));
}

/* EOF */
