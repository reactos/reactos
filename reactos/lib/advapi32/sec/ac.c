/* $Id: ac.c,v 1.1 1999/11/07 08:04:55 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/ac.c
 * PURPOSE:         ACL/ACE functions
 */
#include <windows.h>


/* --- ACL --- */

WINBOOL
STDCALL
GetAclInformation (
	PACL			pAcl,
	LPVOID			pAclInformation,
	DWORD			nAclInformationLength,
	ACL_INFORMATION_CLASS	dwAclInformationClass
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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


WINBOOL
STDCALL
InitializeAcl (
	PACL	pAcl,
	DWORD	nAclLength,
	DWORD	dwAclRevision
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsValidAcl (
	PACL	pAcl
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetAclInformation (
	PACL			pAcl,
	LPVOID			pAclInformation,
	DWORD			nAclInformationLength,
	ACL_INFORMATION_CLASS	dwAclInformationClass
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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



WINBOOL
STDCALL
AddAccessAllowedAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AccessMask,
	PSID	pSid
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
AddAccessDeniedAce (
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AccessMask,
	PSID	pSid
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


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
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



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
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
DeleteAce (
	PACL	pAcl,
	DWORD	dwAceIndex
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
FindFirstFreeAce (
	PACL	pAcl,
	LPVOID	* pAce
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetAce (
	PACL	pAcl,
	DWORD	dwAceIndex,
	LPVOID	* pAce
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
