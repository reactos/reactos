/* $Id: sid.c,v 1.6 2000/10/08 19:12:01 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              ntoskrnl/se/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

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

NTSTATUS STDCALL
RtlConvertSidToUnicodeString(PUNICODE_STRING String,
			     PSID Sid,
			     BOOLEAN AllocateString)
{
   WCHAR Buffer[256];
   PWSTR Ptr;
   ULONG Length;
   ULONG i;

   if (!RtlValidSid(Sid))
     return STATUS_INVALID_SID;

   Ptr = Buffer;
   Ptr += swprintf (Ptr,
		    L"S-%u-",
		    Sid->Revision);

   if(!Sid->IdentifierAuthority.Value[0] &&
      !Sid->IdentifierAuthority.Value[1])
      {
	Ptr += swprintf(Ptr,
			L"%u",
			(ULONG)Sid->IdentifierAuthority.Value[2] << 24 |
			(ULONG)Sid->IdentifierAuthority.Value[3] << 16 |
			(ULONG)Sid->IdentifierAuthority.Value[4] << 8 |
			(ULONG)Sid->IdentifierAuthority.Value[5]);
     }
   else
     {
	Ptr += swprintf(Ptr,
			L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
			Sid->IdentifierAuthority.Value[0],
			Sid->IdentifierAuthority.Value[1],
			Sid->IdentifierAuthority.Value[2],
			Sid->IdentifierAuthority.Value[3],
			Sid->IdentifierAuthority.Value[4],
			Sid->IdentifierAuthority.Value[5]);
     }

   for (i = 0; i < Sid->SubAuthorityCount; i++)
     {
	Ptr += swprintf(Ptr,
			L"-%u",
			Sid->SubAuthority[i]);
     }

   Length = (Ptr - Buffer) * sizeof(WCHAR);

   if (AllocateString)
     {
	String->Buffer = ExAllocatePool(NonPagedPool,
					Length + sizeof(WCHAR));
	if (String->Buffer == NULL)
	  return STATUS_NO_MEMORY;

	String->MaximumLength = Length + sizeof(WCHAR);
     }
   else
     {
	if (Length > String->MaximumLength)
	  return STATUS_BUFFER_TOO_SMALL;
     }
   String->Length = Length;
   memmove(String->Buffer,
	   Buffer,
	   Length);
   if (Length < String->MaximumLength)
     String->Buffer[Length] = 0;

   return STATUS_SUCCESS;
}

/* EOF */
