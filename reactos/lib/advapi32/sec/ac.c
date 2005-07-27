/* $Id$
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
 * @unimplemented
 */
BOOL
STDCALL
AddAccessAllowedObjectAce(
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AceFlags,
	DWORD	AccessMask,
	GUID*	ObjectTypeGuid,
	GUID*	InheritedObjectTypeGuid,
	PSID	pSid)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
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
 * @unimplemented
 */
BOOL
STDCALL
AddAccessDeniedObjectAce(
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AccessMask,
	PSID	pSid)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
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
 * @unimplemented
 */
BOOL
STDCALL
AddAuditAccessObjectAce(
	PACL	pAcl,
	DWORD	dwAceRevision,
	DWORD	AceFlags,
	DWORD	AccessMask,
	GUID*	ObjectTypeGuid,
	GUID*	InheritedObjectTypeGuid,
	PSID	pSid,
	BOOL	bAuditSuccess,
	BOOL	bAuditFailure)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
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
 * @implemented
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
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccGetInheritanceSource(pObjectName,
                                            ObjectType,
                                            SecurityInfo,
                                            Container,
                                            pObjectClassGuids,
                                            GuidCount,
                                            pAcl,
                                            pfnArray,
                                            pGenericMapping,
                                            pInheritArray);
    }

    return ErrorCode;
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
    /* That's all this function does, at least up to w2k3... Even MS was too
       lazy to implement it... */
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
DWORD
STDCALL
FreeInheritedFromArray (
	PINHERITED_FROMW	pInheritArray,
	USHORT			AceCnt,
	PFN_OBJECT_MGR_FUNCTS	pfnArray  OPTIONAL
	)
{
    DWORD ErrorCode;
    
    /* pfnArray is not yet used */
    UNREFERENCED_PARAMETER(pfnArray);

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccFreeIndexArray(pInheritArray,
                                      AceCnt,
                                      NULL);
    }

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
STDCALL
SetEntriesInAclW(
	ULONG			cCountOfExplicitEntries,
	PEXPLICIT_ACCESS_W	pListOfExplicitEntries,
	PACL			OldAcl,
	PACL*			NewAcl)
{
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccRewriteSetEntriesInAcl(cCountOfExplicitEntries,
                                              pListOfExplicitEntries,
                                              OldAcl,
                                              NewAcl);
    }

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
STDCALL
SetEntriesInAclA(
	ULONG			cCountOfExplicitEntries,
	PEXPLICIT_ACCESS_A	pListOfExplicitEntries,
	PACL			OldAcl,
	PACL*			NewAcl)
{
    PEXPLICIT_ACCESS_W ListOfExplicitEntriesW;
    ULONG i;
    DWORD ErrorCode;

    if (cCountOfExplicitEntries != 0)
    {
        ListOfExplicitEntriesW = HeapAlloc(GetProcessHeap(),
                                           0,
                                           cCountOfExplicitEntries * sizeof(EXPLICIT_ACCESS_W));
        if (ListOfExplicitEntriesW != NULL)
        {
            /* directly copy the array, this works as the size of the EXPLICIT_ACCESS_A
               structure matches the size of the EXPLICIT_ACCESS_W version */
            ASSERT(sizeof(EXPLICIT_ACCESS_A) == sizeof(EXPLICIT_ACCESS_W));

            RtlCopyMemory(ListOfExplicitEntriesW,
                          pListOfExplicitEntries,
                          cCountOfExplicitEntries * sizeof(EXPLICIT_ACCESS_W));

            /* convert the trustee names if required */
            for (i = 0; i != cCountOfExplicitEntries; i++)
            {
                if (pListOfExplicitEntries[i].Trustee.TrusteeForm == TRUSTEE_IS_NAME)
                {
                    UINT BufCount = strlen(pListOfExplicitEntries[i].Trustee.ptstrName) + 1;
                    ListOfExplicitEntriesW[i].Trustee.ptstrName =
                        (LPWSTR)HeapAlloc(GetProcessHeap(),
                                          0,
                                          BufCount * sizeof(WCHAR));

                    if (ListOfExplicitEntriesW[i].Trustee.ptstrName == NULL ||
                        MultiByteToWideChar(CP_ACP,
                                            0,
                                            pListOfExplicitEntries[i].Trustee.ptstrName,
                                            -1,
                                            ListOfExplicitEntriesW[i].Trustee.ptstrName,
                                            BufCount) == 0)
                    {
                        /* failed to allocate enough momory for the strings or failed to
                           convert the ansi string to unicode, then fail and free all
                           allocated memory */

                        ErrorCode = GetLastError();

                        while (i != 0)
                        {
                            if (ListOfExplicitEntriesW[i].Trustee.TrusteeForm == TRUSTEE_IS_NAME &&
                                ListOfExplicitEntriesW[i].Trustee.ptstrName != NULL)
                            {
                                HeapFree(GetProcessHeap(),
                                         0,
                                         ListOfExplicitEntriesW[i].Trustee.ptstrName);
                            }

                            i--;
                        }

                        /* free the allocated array */
                        HeapFree(GetProcessHeap(),
                                 0,
                                 ListOfExplicitEntriesW);

                        return ErrorCode;
                    }
                }
            }
        }
        else
        {
            return GetLastError();
        }
    }
    else
        ListOfExplicitEntriesW = NULL;

    ErrorCode = SetEntriesInAclW(cCountOfExplicitEntries,
                                 ListOfExplicitEntriesW,
                                 OldAcl,
                                 NewAcl);

    /* free the strings */
    if (ListOfExplicitEntriesW != NULL)
    {
        /* free the converted strings */
        for (i = 0; i != cCountOfExplicitEntries; i++)
        {
            if (ListOfExplicitEntriesW[i].Trustee.TrusteeForm == TRUSTEE_IS_NAME)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         ListOfExplicitEntriesW[i].Trustee.ptstrName);
            }
        }

        /* free the allocated array */
        HeapFree(GetProcessHeap(),
                 0,
                 ListOfExplicitEntriesW);
    }

    return ErrorCode;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetExplicitEntriesFromAclW(
	PACL			pacl,
	PULONG			pcCountOfExplicitEntries,
	PEXPLICIT_ACCESS_W*	pListOfExplicitEntries
	)
{
    DWORD ErrorCode;

    ErrorCode = CheckNtMartaPresent();
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* call the MARTA provider */
        ErrorCode = AccRewriteGetExplicitEntriesFromAcl(pacl,
                                                        pcCountOfExplicitEntries,
                                                        pListOfExplicitEntries);
    }

    return ErrorCode;
}


/* EOF */
