/* $Id: sid.c,v 1.5 2002/09/07 15:12:41 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              lib/ntdll/rtl/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN STDCALL
RtlValidSid(IN PSID Sid)
{
   PISID iSid = (PISID)Sid;

   if ((iSid->Revision & 0xf) != 1)
     {
	return(FALSE);
     }
   if (iSid->SubAuthorityCount > 15)
     {
	return(FALSE);
     }
   return(TRUE);
}


ULONG STDCALL
RtlLengthRequiredSid(IN UCHAR SubAuthorityCount)
{
  return(sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG));
}


NTSTATUS STDCALL
RtlInitializeSid(IN PSID Sid,
		 IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
		 IN UCHAR SubAuthorityCount)
{
   PISID iSid = (PISID)Sid;

   iSid->Revision = 1;
   iSid->SubAuthorityCount = SubAuthorityCount;
   memcpy(&iSid->IdentifierAuthority, IdentifierAuthority, 
	  sizeof(SID_IDENTIFIER_AUTHORITY));
   return(STATUS_SUCCESS);
}


PULONG STDCALL
RtlSubAuthoritySid(IN PSID Sid,
		   IN ULONG SubAuthority)
{
   PISID iSid = (PISID)Sid;

   return(&iSid->SubAuthority[SubAuthority]);
}


PUCHAR STDCALL
RtlSubAuthorityCountSid(IN PSID Sid)
{
   PISID iSid = (PISID)Sid;

   return(&iSid->SubAuthorityCount);
}


BOOLEAN STDCALL
RtlEqualSid(IN PSID Sid1,
	    IN PSID Sid2)
{
   PISID iSid1 = (PISID)Sid1;
   PISID iSid2 = (PISID)Sid2;

   if (iSid1->Revision != iSid2->Revision)
     {
	return(FALSE);
     }
   if ((*RtlSubAuthorityCountSid(Sid1)) !=
       (*RtlSubAuthorityCountSid(Sid2)))
     {
	return(FALSE);
     }
   if (memcmp(Sid1, Sid2, RtlLengthSid(Sid1)) != 0)
    {
      return(FALSE);
    }
  return(TRUE);
}


ULONG STDCALL
RtlLengthSid(IN PSID Sid)
{
   PISID iSid = (PISID)Sid;

   return(sizeof(SID) + (iSid->SubAuthorityCount-1)*4);
}


NTSTATUS STDCALL
RtlCopySid(ULONG BufferLength,
	   PSID Dest,
	   PSID Src)
{
  if (BufferLength < RtlLengthSid(Src))
    {
      return(STATUS_UNSUCCESSFUL);
    }
  memmove(Dest,
	  Src,
	  RtlLengthSid(Src));
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlCopySidAndAttributesArray(ULONG Count,
			     PSID_AND_ATTRIBUTES Src,
			     ULONG SidAreaSize,
			     PSID_AND_ATTRIBUTES Dest,
			     PVOID SidArea,
			     PVOID* RemainingSidArea,
			     PULONG RemainingSidAreaSize)
{
  ULONG SidLength;
  ULONG Length;
  ULONG i;

  Length = SidAreaSize;

  for (i=0; i<Count; i++)
    {
      if (RtlLengthSid(Src[i].Sid) > Length)
	{
	  return(STATUS_BUFFER_TOO_SMALL);
	}
      SidLength = RtlLengthSid(Src[i].Sid);
      Length = Length - SidLength;
      Dest[i].Sid = SidArea;
      Dest[i].Attributes = Src[i].Attributes;
      RtlCopySid(SidLength,
		 SidArea,
		 Src[i].Sid);
      SidArea = SidArea + SidLength;
    }
  *RemainingSidArea = SidArea;
  *RemainingSidAreaSize = Length;
  return(STATUS_SUCCESS);
}


PSID_IDENTIFIER_AUTHORITY STDCALL
RtlIdentifierAuthoritySid(IN PSID Sid)
{
  PISID iSid = (PISID)Sid;

  return (&iSid->IdentifierAuthority);
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
	PISID pSid;

	if (SubAuthorityCount > 8)
		return STATUS_INVALID_SID;

	if (Sid == NULL)
		return STATUS_INVALID_PARAMETER;

	pSid = (PISID)RtlAllocateHeap (RtlGetProcessHeap (),
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

	*Sid = (PSID)pSid;

	return STATUS_SUCCESS;
}


PSID STDCALL
RtlFreeSid(IN PSID Sid)
{
  RtlFreeHeap(RtlGetProcessHeap(),
	      0,
	      Sid);
  return(Sid);
}


BOOLEAN STDCALL
RtlEqualPrefixSid(IN PSID Sid1,
		  IN PSID Sid2)
{
  PISID iSid1 = (PISID)Sid1;
  PISID iSid2 = (PISID)Sid2;

	return (iSid1->SubAuthorityCount == iSid2->SubAuthorityCount &&
	        !memcmp (iSid1, iSid2,
	                 (iSid1->SubAuthorityCount - 1) * sizeof(DWORD) + 8));
}


NTSTATUS STDCALL
RtlConvertSidToUnicodeString(PUNICODE_STRING String,
			     PSID Sid,
			     BOOLEAN AllocateBuffer)
{
  PISID iSid = (PISID)Sid;
	WCHAR Buffer[256];
	PWSTR wcs;
	ULONG Length;
	BYTE  i;

	if (RtlValidSid (Sid) == FALSE)
		return STATUS_INVALID_SID;

	wcs = Buffer;
	wcs += swprintf (wcs, L"S-%u-", iSid->Revision);
	if (!iSid->IdentifierAuthority.Value[0] &&
	    !iSid->IdentifierAuthority.Value[1])
	{
		wcs += swprintf (wcs,
		                 L"%u",
		                 (DWORD)iSid->IdentifierAuthority.Value[2] << 24 |
		                 (DWORD)iSid->IdentifierAuthority.Value[3] << 16 |
		                 (DWORD)iSid->IdentifierAuthority.Value[4] << 8 |
		                 (DWORD)iSid->IdentifierAuthority.Value[5]);
	}
	else
	{
		wcs += swprintf (wcs,
		                 L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
		                 iSid->IdentifierAuthority.Value[0],
		                 iSid->IdentifierAuthority.Value[1],
		                 iSid->IdentifierAuthority.Value[2],
		                 iSid->IdentifierAuthority.Value[3],
		                 iSid->IdentifierAuthority.Value[4],
		                 iSid->IdentifierAuthority.Value[5]);
	}

	for (i = 0; i < iSid->SubAuthorityCount; i++)
	{
		wcs += swprintf (wcs,
		                 L"-%u",
		                 iSid->SubAuthority[0]);
	}

	Length = (wcs - Buffer) * sizeof(WCHAR);
	if(AllocateBuffer)
	{
		String->Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
		                                  0,
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
	memmove (String->Buffer,
	         Buffer,
	         Length);
	if (Length < String->MaximumLength)
		String->Buffer[Length] = 0;

	return STATUS_SUCCESS;
}

/* EOF */
