/* $Id: ac.c,v 1.9 2004/02/25 14:25:11 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/ac.c
 * PURPOSE:         ACL/ACE functions
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


/* --- ACL --- */

/*
 * @implemented
 */
BOOL
STDCALL
GetAclInformation (
	PACL			pAcl,
	LPVOID			pAclInformation,
	DWORD			nAclInformationLength,
	ACL_INFORMATION_CLASS	dwAclInformationClass
	)
{
	NTSTATUS Status;

	Status = RtlQueryInformationAcl (pAcl,
	                                 pAclInformation,
	                                 nAclInformationLength,
	                                 dwAclInformationClass);
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
InitializeAcl (
	PACL	pAcl,
	DWORD	nAclLength,
	DWORD	dwAclRevision
	)
{
	NTSTATUS Status;

	Status = RtlCreateAcl (pAcl,
	                       nAclLength,
	                       dwAclRevision);
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
IsValidAcl (
	PACL	pAcl
	)
{
	return RtlValidAcl (pAcl);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetAclInformation (
	PACL			pAcl,
	LPVOID			pAclInformation,
	DWORD			nAclInformationLength,
	ACL_INFORMATION_CLASS	dwAclInformationClass
	)
{
	NTSTATUS Status;

	Status = RtlSetInformationAcl (pAcl,
	                               pAclInformation,
	                               nAclInformationLength,
	                               dwAclInformationClass);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/* --- ACE --- */

/*
 * @implemented
 */
BOOL
STDCALL
AddAccessAllowedAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AccessMask,
	PSID	pSid
	)
{
	NTSTATUS Status;

	Status = RtlAddAccessAllowedAce (pAcl,
	                                 dwAceRevision,
	                                 AccessMask,
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
AddAccessDeniedAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AccessMask,
	PSID	pSid
	)
{
	NTSTATUS Status;

	Status = RtlAddAccessDeniedAce (pAcl,
	                                dwAceRevision,
	                                AccessMask,
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
AddAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	dwStartingAceIndex,
	LPVOID	pAceList,
	DWORD	nAceListLength
	)
{
	NTSTATUS Status;

	Status = RtlAddAce (pAcl,
	                    dwAceRevision,
	                    dwStartingAceIndex,
	                    pAceList,
	                    nAceListLength);
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
AddAuditAccessAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	dwAccessMask,
	PSID	pSid,
	BOOL	bAuditSuccess,
	BOOL	bAuditFailure
	)
{
	NTSTATUS Status;

	Status = RtlAddAuditAccessAce (pAcl,
	                               dwAceRevision,
	                               dwAccessMask,
	                               pSid,
	                               bAuditSuccess,
	                               bAuditFailure);
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
DeleteAce (
	PACL	pAcl,
	DWORD	dwAceIndex
	)
{
	NTSTATUS Status;

	Status = RtlDeleteAce (pAcl,
	                       dwAceIndex);
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
FindFirstFreeAce (
	PACL	pAcl,
	LPVOID	* pAce
	)
{
	return RtlFirstFreeAce (pAcl,
	                        (PACE*)pAce);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetAce (
	PACL	pAcl,
	DWORD	dwAceIndex,
	LPVOID	* pAce
	)
{
	NTSTATUS Status;

	Status = RtlGetAce (pAcl,
	                    dwAceIndex,
	                    (PACE*)pAce);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}

/* EOF */
