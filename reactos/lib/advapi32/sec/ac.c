/* $Id: ac.c,v 1.12 2004/12/13 19:06:28 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/ac.c
 * PURPOSE:         ACL/ACE functions
 */

#include "advapi32.h"

#define NDEBUG
#include <debug.h>

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
BOOL STDCALL
AddAccessAllowedAceEx(PACL pAcl,
		      DWORD dwAceRevision,
		      DWORD AceFlags,
		      DWORD AccessMask,
		      PSID pSid)
{
  NTSTATUS Status;

  Status = RtlAddAccessAllowedAceEx(pAcl,
                                    dwAceRevision,
                                    AceFlags,
                                    AccessMask,
                                    pSid);
  if (!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
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
BOOL STDCALL
AddAccessDeniedAceEx(PACL pAcl,
		     DWORD dwAceRevision,
		     DWORD AceFlags,
		     DWORD AccessMask,
		     PSID pSid)
{
  NTSTATUS Status;

  Status = RtlAddAccessDeniedAceEx(pAcl,
                                   dwAceRevision,
                                   AceFlags,
                                   AccessMask,
                                   pSid);
  if (!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
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
BOOL STDCALL
AddAuditAccessAceEx(PACL pAcl,
		    DWORD dwAceRevision,
		    DWORD AceFlags,
		    DWORD dwAccessMask,
		    PSID pSid,
		    BOOL bAuditSuccess,
		    BOOL bAuditFailure)
{
  NTSTATUS Status;

  Status = RtlAddAuditAccessAceEx(pAcl,
                                  dwAceRevision,
                                  AceFlags,
                                  dwAccessMask,
                                  pSid,
                                  bAuditSuccess,
                                  bAuditFailure);
  if (!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
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


/*
 * @unimplemented
 */
DWORD
STDCALL
GetInheritanceSourceW (
	LPWSTR			pObjectName,
	SE_OBJECT_TYPE		ObjectType,
	SECURITY_INFORMATION	SecurityInfo,
	BOOL			Container,
	GUID**			pObjectClassGuids  OPTIONAL,
	DWORD			GuidCount,
	PACL			pAcl,
	PFN_OBJECT_MGR_FUNCTS	pfnArray  OPTIONAL,
	PGENERIC_MAPPING	pGenericMapping,
	PINHERITED_FROMW	pInheritArray
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetInheritanceSourceA (
	LPSTR			pObjectName,
	SE_OBJECT_TYPE		ObjectType,
	SECURITY_INFORMATION	SecurityInfo,
	BOOL			Container,
	GUID**			pObjectClassGuids  OPTIONAL,
	DWORD			GuidCount,
	PACL			pAcl,
	PFN_OBJECT_MGR_FUNCTS	pfnArray  OPTIONAL,
	PGENERIC_MAPPING	pGenericMapping,
	PINHERITED_FROM		pInheritArray
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
FreeInheritedFromArray (
	PINHERITED_FROM		pInheritArray,
	USHORT			AceCnt,
	PFN_OBJECT_MGR_FUNCTS	pfnArray  OPTIONAL
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
