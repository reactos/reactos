/* $Id: acl.c,v 1.6 2002/08/13 20:41:22 ekohl Exp $
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
  AclEnd = Acl->AclSize + (PVOID)Acl;
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
    }
  while (i < Acl->AceCount);

  if ((PVOID)Current < AclEnd)
    {
      *Ace = Current;
    }

  return(TRUE);
}


NTSTATUS STDCALL
RtlGetAce(PACL Acl,
	  ULONG AceIndex,
	  PACE *Ace)
{
  ULONG i;

  *Ace = (PACE)(Acl + 1);

  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  if (AceIndex >= Acl->AceCount)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  for (i = 0; i < AceIndex; i++)
    {
      if ((PVOID)*Ace >= (PVOID)Acl + Acl->AclSize)
	{
	  return(STATUS_INVALID_PARAMETER);
	}
      *Ace = (PACE)((PVOID)(*Ace) + (ULONG)(*Ace)->Header.AceSize);
    }

  if ((PVOID)*Ace >= (PVOID)Acl + Acl->AclSize)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  return(STATUS_SUCCESS);
}


static NTSTATUS
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
  if (Acl->AclRevision > 3 ||
      Revision > 3)
    {
      return(STATUS_UNKNOWN_REVISION);
    }
  if (Revision < Acl->AclRevision)
    {
      Revision = Acl->AclRevision;
    }
  if (!RtlFirstFreeAce(Acl, &Ace))
    {
      return(STATUS_INVALID_ACL);
    }
  if (Ace == NULL)
    {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
    }
  if (((PVOID)Ace + RtlLengthSid(Sid) + sizeof(ACE)) >=
      ((PVOID)Acl + Acl->AclSize))
    {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
    }
  Ace->Header.AceFlags = 0;
  Ace->Header.AceType = Type;
  Ace->Header.AceSize = RtlLengthSid(Sid) + sizeof(ACE);
  Ace->Header.AccessMask = AccessMask;
  RtlCopySid(RtlLengthSid(Sid), (PSID)(Ace + 1), Sid);
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
RtlAddAccessDeniedAce(PACL Acl,
		      ULONG Revision,
		      ACCESS_MASK AccessMask,
		      PSID Sid)
{
  return(RtlpAddKnownAce(Acl, Revision, AccessMask, Sid, 1));
}


static VOID
RtlpAddData(PVOID AceList,
	    ULONG AceListLength,
	    PVOID Ace,
	    ULONG Offset)
{
  if (Offset > 0)
    {
      memcpy((PUCHAR)Ace + AceListLength,
	     Ace,
	     Offset);
    }

  if (AceListLength != 0)
    {
      memcpy(Ace,
	     AceList,
	     AceListLength);
    }
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
      return(STATUS_INVALID_PARAMETER);
    }

  if (!RtlFirstFreeAce(Acl,&Ace))
    {
      return(STATUS_INVALID_PARAMETER);
    }

  if (Acl->AclRevision <= AclRevision)
    {
      AclRevision = Acl->AclRevision;
    }

  if (((PVOID)AceList + AceListLength) <= (PVOID)AceList)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  i = 0;
  Current = (PACE)(Acl + 1);
  while ((PVOID)Current < ((PVOID)AceList + AceListLength))
    {
      if (AceList->Header.AceType == 4 &&
	  AclRevision < 3)
	{
	  return(STATUS_INVALID_PARAMETER);
	}
      Current = (PACE)((PVOID)Current + Current->Header.AceSize);
    }

  if (Ace == NULL)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }

  if (((PVOID)Ace + AceListLength) >= ((PVOID)Acl + Acl->AclSize))
    {
      return(STATUS_BUFFER_TOO_SMALL);
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

  RtlpAddData(AceList,
	      AceListLength,
	      Current,
	      (ULONG)Ace - (ULONG)Current);
  Acl->AceCount = Acl->AceCount + i;
  Acl->AclRevision = AclRevision;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlAddAuditAccessAce(PACL Acl,
		     ULONG Revision,
		     ACCESS_MASK AccessMask,
		     PSID Sid,
		     BOOLEAN Success,
		     BOOLEAN Failure)
{
  PACE Ace;
  ULONG Flags = 0;

  if (Success != FALSE)
    {
      Flags |= SUCCESSFUL_ACCESS_ACE_FLAG;
    }

  if (Failure != FALSE)
    {
      Flags |= FAILED_ACCESS_ACE_FLAG;
    }

  if (!RtlValidSid(Sid))
    {
      return(STATUS_INVALID_SID);
    }

  if (Acl->AclRevision > 3 ||
      Revision > 3)
    {
      return(STATUS_REVISION_MISMATCH);
    }

  if (Revision < Acl->AclRevision)
    {
      Revision = Acl->AclRevision;
    }

  if (!RtlFirstFreeAce(Acl, &Ace))
    {
      return(STATUS_INVALID_ACL);
    }

  if (Ace == NULL)
    {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
    }

  if (((PVOID)Ace + RtlLengthSid(Sid) + sizeof(ACE)) >= ((PVOID)Acl + Acl->AclSize))
    {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
    }

  Ace->Header.AceFlags = Flags;
  Ace->Header.AceType = 2;
  Ace->Header.AceSize = RtlLengthSid(Sid) + sizeof(ACE);
  Ace->Header.AccessMask = AccessMask;
  RtlCopySid(RtlLengthSid(Sid),
	     (PSID)(Ace + 1),
	     Sid);
  Acl->AceCount++;
  Acl->AclRevision = Revision;

  return(STATUS_SUCCESS);
}


static VOID
RtlpDeleteData(PVOID Ace,
	       ULONG AceSize,
	       ULONG Offset)
{
  if (AceSize < Offset)
    {
      memcpy(Ace,
	     (PUCHAR)Ace + AceSize,
	     Offset - AceSize);
    }

  if (Offset - AceSize < Offset)
    {
      memset((PUCHAR)Ace + Offset - AceSize,
	     0,
	     AceSize);
    }
}


NTSTATUS STDCALL
RtlDeleteAce(PACL Acl,
	     ULONG AceIndex)
{
  PACE Ace;
  PACE Current;

  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  if (Acl->AceCount <= AceIndex)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  if (!RtlFirstFreeAce(Acl, &Ace))
    {
      return(STATUS_INVALID_PARAMETER);
    }

  Current = (PACE)(Acl + 1);

  while(AceIndex--)
    {
      Current = (PACE)((PVOID)Current + Current->Header.AceSize);
    }

  RtlpDeleteData(Current,
		 Current->Header.AceSize,
		 Ace - Current);
  Acl->AceCount++;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlCreateAcl(PACL Acl,
	     ULONG AclSize,
	     ULONG AclRevision)
{
  if (AclSize < 8)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }

  if (AclRevision != 2 &&
      AclRevision != 3)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  if (AclSize > 0xffff)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  AclSize = AclSize & ~(0x3);
  Acl->AclSize = AclSize;
  Acl->AclRevision = AclRevision;
  Acl->AceCount = 0;
  Acl->Sbz1 = 0;
  Acl->Sbz2 = 0;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlQueryInformationAcl(PACL Acl,
		       PVOID Information,
		       ULONG InformationLength,
		       ACL_INFORMATION_CLASS InformationClass)
{
  PACE Ace;

  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  switch (InformationClass)
    {
      case AclRevisionInformation:
	{
	  PACL_REVISION_INFORMATION Info = (PACL_REVISION_INFORMATION)Information;

	  if (InformationLength < sizeof(ACL_REVISION_INFORMATION))
	    {
	      return(STATUS_BUFFER_TOO_SMALL);
	    }
	  Info->AclRevision = Acl->AclRevision;
	}
	break;

      case AclSizeInformation:
	{
	  PACL_SIZE_INFORMATION Info = (PACL_SIZE_INFORMATION)Information;

	  if (InformationLength < sizeof(ACL_SIZE_INFORMATION))
	    {
	      return(STATUS_BUFFER_TOO_SMALL);
	    }

	  if (!RtlFirstFreeAce(Acl, &Ace))
	    {
	      return(STATUS_INVALID_PARAMETER);
	    }

	  Info->AceCount = Acl->AceCount;
	  if (Ace != NULL)
	    {
	      Info->AclBytesInUse = (PVOID)Ace - (PVOID)Acl;
	      Info->AclBytesFree  = Acl->AclSize - Info->AclBytesInUse;
	    }
	  else
	    {
	      Info->AclBytesInUse = Acl->AclSize;
	      Info->AclBytesFree  = 0;
	    }
	}
	break;

      default:
	return(STATUS_INVALID_INFO_CLASS);
    }

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSetInformationAcl(PACL Acl,
		     PVOID Information,
		     ULONG InformationLength,
		     ACL_INFORMATION_CLASS InformationClass)
{
  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  switch (InformationClass)
    {
      case AclRevisionInformation:
	{
	  PACL_REVISION_INFORMATION Info = (PACL_REVISION_INFORMATION)Information;

	  if (InformationLength < sizeof(ACL_REVISION_INFORMATION))
	    {
	      return(STATUS_BUFFER_TOO_SMALL);
	    }

	  if (Acl->AclRevision >= Info->AclRevision)
	    {
	      return(STATUS_INVALID_PARAMETER);
	    }

	  Acl->AclRevision = Info->AclRevision;
	}
	break;

      default:
	return(STATUS_INVALID_INFO_CLASS);
    }

  return(STATUS_SUCCESS);
}


BOOLEAN STDCALL
RtlValidAcl(PACL Acl)
{
  PACE Ace;
  USHORT Size;

  Size = (Acl->AclSize + 3) & ~3;

  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3)
    {
      return(FALSE);
    }

  if (Size != Acl->AclSize)
    {
      return(FALSE);
    }

  return(RtlFirstFreeAce(Acl, &Ace));
}

/* EOF */
