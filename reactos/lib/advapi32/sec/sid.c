/* $Id: sid.c,v 1.4 2001/11/22 02:37:32 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sid.c
 * PURPOSE:         Security ID functions
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>


BOOL STDCALL
AllocateLocallyUniqueId(PLUID Luid)
{
   NTSTATUS Status;

   Status = NtAllocateLocallyUniqueId(Luid);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   return(TRUE);
}

BOOL STDCALL
AllocateAndInitializeSid (
	PSID_IDENTIFIER_AUTHORITY	pIdentifierAuthority,
	BYTE				nSubAuthorityCount,
	DWORD				dwSubAuthority0,
	DWORD				dwSubAuthority1,
	DWORD				dwSubAuthority2,
	DWORD				dwSubAuthority3,
	DWORD				dwSubAuthority4,
	DWORD				dwSubAuthority5,
	DWORD				dwSubAuthority6,
	DWORD				dwSubAuthority7,
	PSID				*pSid
)
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
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}

BOOL
STDCALL
CopySid (
	DWORD	nDestinationSidLength,
	PSID	pDestinationSid,
	PSID	pSourceSid
)
{
	NTSTATUS Status;

	Status = RtlCopySid (nDestinationSidLength,
	                     pDestinationSid,
	                     pSourceSid);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}

WINBOOL
STDCALL
EqualPrefixSid (
	PSID	pSid1,
	PSID	pSid2
	)
{
	return RtlEqualPrefixSid (pSid1, pSid2);
}

WINBOOL
STDCALL
EqualSid (
	PSID	pSid1,
	PSID	pSid2
	)
{
	return RtlEqualSid (pSid1, pSid2);
}

PVOID
STDCALL
FreeSid (
	PSID	pSid
	)
{
	return RtlFreeSid (pSid);
}

DWORD
STDCALL
GetLengthSid (
	PSID	pSid
	)
{
	return (DWORD)RtlLengthSid (pSid);
}

PSID_IDENTIFIER_AUTHORITY
STDCALL
GetSidIdentifierAuthority (
	PSID	pSid
	)
{
	return RtlIdentifierAuthoritySid (pSid);
}

DWORD
STDCALL
GetSidLengthRequired (
	UCHAR	nSubAuthorityCount
	)
{
	return (DWORD)RtlLengthRequiredSid (nSubAuthorityCount);
}

PDWORD
STDCALL
GetSidSubAuthority (
	PSID	pSid,
	DWORD	nSubAuthority
	)
{
	return (PDWORD)RtlSubAuthoritySid (pSid, nSubAuthority);
}

PUCHAR
STDCALL
GetSidSubAuthorityCount (
	PSID	pSid
	)
{
	return RtlSubAuthorityCountSid (pSid);
}

WINBOOL
STDCALL
InitializeSid (
	PSID				Sid,
	PSID_IDENTIFIER_AUTHORITY	pIdentifierAuthority,
	BYTE				nSubAuthorityCount
	)
{
	NTSTATUS Status;

	Status = RtlInitializeSid (Sid,
	                           pIdentifierAuthority,
	                           nSubAuthorityCount);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


WINBOOL STDCALL
IsValidSid(PSID pSid)
{
  return((WINBOOL)RtlValidSid(pSid));
}


WINBOOL STDCALL
LookupAccountNameA(LPCSTR lpSystemName,
		   LPCSTR lpAccountName,
		   PSID Sid,
		   LPDWORD cbSid,
		   LPSTR DomainName,
		   LPDWORD cbDomainName,
		   PSID_NAME_USE peUse)
{
  return(FALSE);
}


WINBOOL STDCALL
LookupAccountNameW(LPCWSTR lpSystemName,
		   LPCWSTR lpAccountName,
		   PSID Sid,
		   LPDWORD cbSid,
		   LPWSTR DomainName,
		   LPDWORD cbDomainName,
		   PSID_NAME_USE peUse)
{
  return(FALSE);
}


WINBOOL STDCALL
LookupAccountSidA(LPCSTR lpSystemName,
		  PSID Sid,
		  LPSTR Name,
		  LPDWORD cbName,
		  LPSTR ReferencedDomainName,
		  LPDWORD cbReferencedDomainName,
		  PSID_NAME_USE peUse)
{
  return(FALSE);
}


WINBOOL STDCALL
LookupAccountSidW(LPCWSTR lpSystemName,
		  PSID Sid,
		  LPWSTR Name,
		  LPDWORD cbName,
		  LPWSTR ReferencedDomainName,
		  LPDWORD cbReferencedDomainName,
		  PSID_NAME_USE peUse)
{
  return(FALSE);
}

/* EOF */
