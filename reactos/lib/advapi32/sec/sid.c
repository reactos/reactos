/* $Id: sid.c,v 1.10 2004/02/25 14:25:11 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sid.c
 * PURPOSE:         Security ID functions
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
STDCALL
EqualPrefixSid (
	PSID	pSid1,
	PSID	pSid2
	)
{
	return RtlEqualPrefixSid (pSid1, pSid2);
}

/*
 * @implemented
 */
BOOL
STDCALL
EqualSid (
	PSID	pSid1,
	PSID	pSid2
	)
{
	return RtlEqualSid (pSid1, pSid2);
}


/*
 * @implemented
 */
PVOID
STDCALL
FreeSid (
	PSID	pSid
	)
{
	return RtlFreeSid (pSid);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetLengthSid (
	PSID	pSid
	)
{
	return (DWORD)RtlLengthSid (pSid);
}


/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY
STDCALL
GetSidIdentifierAuthority (
	PSID	pSid
	)
{
	return RtlIdentifierAuthoritySid (pSid);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetSidLengthRequired (
	UCHAR	nSubAuthorityCount
	)
{
	return (DWORD)RtlLengthRequiredSid (nSubAuthorityCount);
}


/*
 * @implemented
 */
PDWORD
STDCALL
GetSidSubAuthority (
	PSID	pSid,
	DWORD	nSubAuthority
	)
{
	return (PDWORD)RtlSubAuthoritySid (pSid, nSubAuthority);
}


/*
 * @implemented
 */
PUCHAR
STDCALL
GetSidSubAuthorityCount (
	PSID	pSid
	)
{
	return RtlSubAuthorityCountSid (pSid);
}


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
BOOL STDCALL
IsValidSid(PSID pSid)
{
  return((BOOL)RtlValidSid(pSid));
}

/* EOF */
