/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN RtlValidSid(PSID Sid)
{
   if ((Sid->Revision & 0xf) != 1)
     {
	return(FALSE);
     }
   if (Sid->SubAuthorityCount > 15)
     {
	return(FALSE);
     }
   return(TRUE);
}

ULONG RtlLengthRequiredSid(UCHAR SubAuthorityCount)
{
   return(sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG));
}

NTSTATUS RtlInitializeSid(PSID Sid,
			  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
			  UCHAR SubAuthorityCount)
{
   Sid->Revision = 1;
   Sid->SubAuthorityCount = SubAuthorityCount;
   memcpy(&Sid->IdentifierAuthority, IdentifierAuthority, 
	  sizeof(SID_IDENTIFIER_AUTHORITY));
   return(STATUS_SUCCESS);
}

PULONG RtlSubAuthoritySid(PSID Sid, ULONG SubAuthority)
{
   return(&Sid->SubAuthority[SubAuthority]);
}

PUCHAR RtlSubAuthorityCountSid(PSID Sid)
{
   return(&Sid->SubAuthorityCount);
}

BOOLEAN RtlEqualSid(PSID Sid1, PSID Sid2)
{
   if (Sid1->Revision != Sid2->Revision)
     {
	return(FALSE);
     }
   if ((*RtlSubAuthorityCountSid(Sid1)) !=
       (*RtlSubAuthorityCountSid(Sid2)))
     {
	return(FALSE);
     }
   if (memcmp(Sid1, Sid2, RtlLengthSid(Sid1) != 0))
     {
	return(FALSE);
     }
   return(TRUE);
}

ULONG RtlLengthSid(PSID Sid)
{
   return(sizeof(SID) + (Sid->SubAuthorityCount-1)*4);
}


NTSTATUS RtlCopySid(ULONG BufferLength, PSID Src, PSID Dest)
{
   if (BufferLength < RtlLengthSid(Src))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   memmove(Dest, Src, RtlLengthSid(Src));
   return(STATUS_SUCCESS);
}
