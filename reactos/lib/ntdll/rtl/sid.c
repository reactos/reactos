/* $Id: sid.c,v 1.1 2000/03/12 01:17:59 ekohl Exp $
 *
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
#include <string.h>

//#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN STDCALL RtlValidSid (PSID Sid)
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

ULONG STDCALL RtlLengthRequiredSid (UCHAR SubAuthorityCount)
{
   return(sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG));
}

NTSTATUS STDCALL RtlInitializeSid (PSID Sid,
			  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
			  UCHAR SubAuthorityCount)
{
   Sid->Revision = 1;
   Sid->SubAuthorityCount = SubAuthorityCount;
   memcpy(&Sid->IdentifierAuthority, IdentifierAuthority, 
	  sizeof(SID_IDENTIFIER_AUTHORITY));
   return(STATUS_SUCCESS);
}

PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority)
{
   return(&Sid->SubAuthority[SubAuthority]);
}

PUCHAR STDCALL RtlSubAuthorityCountSid (PSID Sid)
{
   return(&Sid->SubAuthorityCount);
}

BOOLEAN STDCALL RtlEqualSid (PSID Sid1, PSID Sid2)
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

ULONG STDCALL RtlLengthSid (PSID Sid)
{
   return(sizeof(SID) + (Sid->SubAuthorityCount-1)*4);
}

NTSTATUS STDCALL RtlCopySid (ULONG BufferLength, PSID Dest, PSID Src)
{
   if (BufferLength < RtlLengthSid(Src))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   memmove(Dest, Src, RtlLengthSid(Src));
   return(STATUS_SUCCESS);
}

PSID_IDENTIFIER_AUTHORITY
STDCALL
RtlIdentifierAuthoritySid (
	PSID	Sid
	)
{
	return (&Sid->IdentifierAuthority);
}

NTSTATUS
STDCALL
RtlAllocateAndInitializeSid (
	PSID_IDENTIFIER_AUTHORITY	IdentifierAuthority,
	UCHAR				SubAuthorityCount,
	ULONG				SubAuthority0,
	ULONG				SubAuthority1,
	ULONG				SubAuthority2,
	ULONG				SubAuthority3,
	ULONG				SubAuthority4,
	ULONG				SubAuthority5,
	ULONG				SubAuthority6,
	ULONG				SubAuthority7,
	PSID				*Sid
	)
{
	PSID pSid;

	if (SubAuthorityCount > 8)
		return STATUS_INVALID_SID;

	if (Sid == NULL)
		return STATUS_INVALID_PARAMETER;

	pSid = (PSID)RtlAllocateHeap (RtlGetProcessHeap (),
	                              0,
	                              SubAuthorityCount * sizeof(DWORD) + 8);
	if (pSid == NULL)
		return STATUS_NO_MEMORY;

	pSid->Revision = 1;
	pSid->SubAuthorityCount = SubAuthorityCount;
	memcpy (&pSid->IdentifierAuthority,
	        IdentifierAuthority,
	        sizeof(SID_IDENTIFIER_AUTHORITY));

	switch (SubAuthorityCount)
	{
		case 8:
			pSid->SubAuthority[7] = SubAuthority7;
		case 7:
			pSid->SubAuthority[6] = SubAuthority6;
		case 6:
			pSid->SubAuthority[5] = SubAuthority5;
		case 5:
			pSid->SubAuthority[4] = SubAuthority4;
		case 4:
			pSid->SubAuthority[3] = SubAuthority3;
		case 3:
			pSid->SubAuthority[2] = SubAuthority2;
		case 2:
			pSid->SubAuthority[1] = SubAuthority1;
		case 1:
			pSid->SubAuthority[0] = SubAuthority0;
			break;
	}

	*Sid = pSid;

	return STATUS_SUCCESS;
}


PSID
STDCALL
RtlFreeSid (
	PSID	Sid
	)
{
	RtlFreeHeap (RtlGetProcessHeap (), 0, Sid);
	return Sid;
}


BOOLEAN
STDCALL
RtlEqualPrefixSid (
	PSID	Sid1,
	PSID	Sid2
	)
{
	return (Sid1->SubAuthorityCount == Sid2->SubAuthorityCount &&
	        !memcmp (Sid1, Sid2,
	                 (Sid1->SubAuthorityCount - 1) * sizeof(DWORD) + 8));
}



/* EOF */
