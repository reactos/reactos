/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sid.c
 * PURPOSE:         Security ID functions
 */

#include "advapi32.h"
#include <debug.h>


/*
 * @implemented
 */
BOOL STDCALL
AllocateLocallyUniqueId(PLUID Luid)
{
  NTSTATUS Status;

  Status = NtAllocateLocallyUniqueId (Luid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
AllocateAndInitializeSid (PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
			  BYTE nSubAuthorityCount,
			  DWORD dwSubAuthority0,
			  DWORD dwSubAuthority1,
			  DWORD dwSubAuthority2,
			  DWORD dwSubAuthority3,
			  DWORD dwSubAuthority4,
			  DWORD dwSubAuthority5,
			  DWORD dwSubAuthority6,
			  DWORD dwSubAuthority7,
			  PSID *pSid)
{
  NTSTATUS Status;

  Status = RtlAllocateAndInitializeSid (pIdentifierAuthority,
					nSubAuthorityCount,
					dwSubAuthority0,
					dwSubAuthority1,
	                                      dwSubAuthority2,
	                                      dwSubAuthority3,
	                                      dwSubAuthority4,
	                                      dwSubAuthority5,
	                                      dwSubAuthority6,
	                                      dwSubAuthority7,
	                                      pSid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CopySid (DWORD nDestinationSidLength,
	 PSID pDestinationSid,
	 PSID pSourceSid)
{
  NTSTATUS Status;

  Status = RtlCopySid (nDestinationSidLength,
		       pDestinationSid,
		       pSourceSid);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
EqualPrefixSid (PSID pSid1,
		PSID pSid2)
{
  return RtlEqualPrefixSid (pSid1, pSid2);
}


/*
 * @implemented
 */
BOOL STDCALL
EqualSid (PSID pSid1,
	  PSID pSid2)
{
  return RtlEqualSid (pSid1, pSid2);
}


/*
 * @implemented
 *
 * RETURNS
 *  Docs says this function does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID STDCALL
FreeSid (PSID pSid)
{
   return RtlFreeSid (pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetLengthSid (PSID pSid)
{
  return (DWORD)RtlLengthSid (pSid);
}


/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY STDCALL
GetSidIdentifierAuthority (PSID pSid)
{
  return RtlIdentifierAuthoritySid (pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetSidLengthRequired (UCHAR nSubAuthorityCount)
{
  return (DWORD)RtlLengthRequiredSid (nSubAuthorityCount);
}


/*
 * @implemented
 */
PDWORD STDCALL
GetSidSubAuthority (PSID pSid,
		    DWORD nSubAuthority)
{
  return (PDWORD)RtlSubAuthoritySid (pSid, nSubAuthority);
}


/*
 * @implemented
 */
PUCHAR STDCALL
GetSidSubAuthorityCount (PSID pSid)
{
  return RtlSubAuthorityCountSid (pSid);
}


/*
 * @implemented
 */
BOOL STDCALL
InitializeSid (PSID Sid,
	       PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	       BYTE nSubAuthorityCount)
{
  NTSTATUS Status;

  Status = RtlInitializeSid (Sid,
			     pIdentifierAuthority,
			     nSubAuthorityCount);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
IsValidSid (PSID pSid)
{
  return (BOOL)RtlValidSid (pSid);
}

/*
 * @implemented
 */
BOOL STDCALL
ConvertSidToStringSidW(PSID Sid, LPWSTR *StringSid)
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  WCHAR FixedBuffer[64];

  if (! RtlValidSid(Sid))
    {
      SetLastError(ERROR_INVALID_SID);
      return FALSE;
    }

  UnicodeString.Length = 0;
  UnicodeString.MaximumLength = sizeof(FixedBuffer);
  UnicodeString.Buffer = FixedBuffer;
  Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, FALSE);
  if (STATUS_BUFFER_TOO_SMALL == Status)
    {
      Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, TRUE);
    }
  if (! NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  *StringSid = LocalAlloc(LMEM_FIXED, UnicodeString.Length + sizeof(WCHAR));
  if (NULL == *StringSid)
    {
      if (UnicodeString.Buffer != FixedBuffer)
        {
          RtlFreeUnicodeString(&UnicodeString);
        }
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  MoveMemory(*StringSid, UnicodeString.Buffer, UnicodeString.Length);
  ZeroMemory((PCHAR) *StringSid + UnicodeString.Length, sizeof(WCHAR));
  if (UnicodeString.Buffer != FixedBuffer)
    {
      RtlFreeUnicodeString(&UnicodeString);
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ConvertSidToStringSidA(PSID Sid, LPSTR *StringSid)
{
  LPWSTR StringSidW;
  int Len;

  if (! ConvertSidToStringSidW(Sid, &StringSidW))
    {
      return FALSE;
    }

  Len = WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, NULL, 0, NULL, NULL);
  if (Len <= 0)
    {
      LocalFree(StringSidW);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
  *StringSid = LocalAlloc(LMEM_FIXED, Len);
  if (NULL == *StringSid)
    {
      LocalFree(StringSidW);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (! WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, *StringSid, Len, NULL, NULL))
    {
      LocalFree(StringSid);
      LocalFree(StringSidW);
      return FALSE;
    }

  LocalFree(StringSidW);

  return TRUE;
}

/* EOF */
