/* $Id: ac.c,v 1.7 2003/07/10 15:05:55 chorns Exp $
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
WINBOOL
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


#if 0
DWORD
WINAPI
GetAuditedPermissionsFromAclA (
	IN	PACL		pacl,
	IN	PTRUSTEE_A	pTrustee,
	OUT	PACCESS_MASK	pSuccessfulAuditedRights,
	OUT	PACCESS_MASK	pFailedAuditRights
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
GetAuditedPermissionsFromAclW (
	IN	PACL		pacl,
	IN	PTRUSTEE_W	pTrustee,
	OUT	PACCESS_MASK	pSuccessfulAuditedRights,
	OUT	PACCESS_MASK	pFailedAuditRights
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
GetEffectiveRightsFromAclA (
	IN	PACL		pacl,
	IN	PTRUSTEE_A	pTrustee,
	OUT	PACCESS_MASK	pAccessRights
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
GetEffectiveRightsFromAclW (
	IN	PACL		pacl,
	IN	PTRUSTEE_W	pTrustee,
	OUT	PACCESS_MASK	pAccessRights
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
GetExplicitEntriesFromAclA (
	IN	PACL			pacl,
	OUT	PULONG			pcCountOfExplicitEntries,
	OUT	PEXPLICIT_ACCESS_A	* pListOfExplicitEntries
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
GetExplicitEntriesFromAclW (
	IN	PACL			pacl,
	OUT	PULONG			pcCountOfExplicitEntries,
	OUT	PEXPLICIT_ACCESS_W	* pListOfExplicitEntries
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}
#endif


/*
 * @implemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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


#if 0
DWORD
WINAPI
SetEntriesInAclA (
	IN	ULONG			cCountOfExplicitEntries,
	IN	PEXPLICIT_ACCESS_A	pListOfExplicitEntries,
	IN	PACL			OldAcl,
	OUT	PACL			* NewAcl
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
WINAPI
SetEntriesInAclW (
	IN	ULONG			cCountOfExplicitEntries,
	IN	PEXPLICIT_ACCESS_W	pListOfExplicitEntries,
	IN	PACL			OldAcl,
	OUT	PACL			* NewAcl
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}
#endif



/* --- ACE --- */

/*
 * @implemented
 */
WINBOOL
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
WINBOOL
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
WINBOOL
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
WINBOOL
STDCALL
AddAuditAccessAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	dwAccessMask,
	PSID	pSid,
	WINBOOL	bAuditSuccess,
	WINBOOL	bAuditFailure
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
WINBOOL
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
WINBOOL
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
WINBOOL
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
