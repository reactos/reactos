/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    Steven Edwards ( Steven_Ed4153@yahoo.com )
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Added a few new stubs 6/27/03
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorControl (
	PSECURITY_DESCRIPTOR		pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL	pControl,
	LPDWORD				lpdwRevision
	)
{
	NTSTATUS Status;

	Status = RtlGetControlSecurityDescriptor (pSecurityDescriptor,
	                                          pControl,
	                                          (PULONG)lpdwRevision);
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
GetSecurityDescriptorDacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPBOOL			lpbDaclPresent,
	PACL			*pDacl,
	LPBOOL			lpbDaclDefaulted
	)
{
	BOOLEAN DaclPresent;
	BOOLEAN DaclDefaulted;
	NTSTATUS Status;

	Status = RtlGetDaclSecurityDescriptor (pSecurityDescriptor,
	                                       &DaclPresent,
	                                       pDacl,
	                                       &DaclDefaulted);
	*lpbDaclPresent = (BOOL)DaclPresent;
	*lpbDaclDefaulted = (BOOL)DaclDefaulted;

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
GetSecurityDescriptorGroup (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			*pGroup,
	LPBOOL			lpbGroupDefaulted
	)
{
	BOOLEAN GroupDefaulted;
	NTSTATUS Status;

	Status = RtlGetGroupSecurityDescriptor (pSecurityDescriptor,
	                                        pGroup,
	                                        &GroupDefaulted);
	*lpbGroupDefaulted = (BOOL)GroupDefaulted;

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
GetSecurityDescriptorLength (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	return RtlLengthSecurityDescriptor(pSecurityDescriptor);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorOwner (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			*pOwner,
	LPBOOL			lpbOwnerDefaulted
)
{
	BOOLEAN OwnerDefaulted;
	NTSTATUS Status;

	Status = RtlGetOwnerSecurityDescriptor (pSecurityDescriptor,
	                                        pOwner,
	                                        &OwnerDefaulted);
	*lpbOwnerDefaulted = (BOOL)OwnerDefaulted;

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
GetSecurityDescriptorSacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPBOOL			lpbSaclPresent,
	PACL			*pSacl,
	LPBOOL			lpbSaclDefaulted
	)
{
	BOOLEAN SaclPresent;
	BOOLEAN SaclDefaulted;
	NTSTATUS Status;

	Status = RtlGetSaclSecurityDescriptor (pSecurityDescriptor,
	                                       &SaclPresent,
	                                       pSacl,
	                                       &SaclDefaulted);
	*lpbSaclPresent = (BOOL)SaclPresent;
	*lpbSaclDefaulted = (BOOL)SaclDefaulted;

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
InitializeSecurityDescriptor (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	DWORD			dwRevision
	)
{
	NTSTATUS Status;

	Status = RtlCreateSecurityDescriptor (pSecurityDescriptor,
	                                      dwRevision);
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
IsValidSecurityDescriptor (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	BOOLEAN Result;

	Result = RtlValidSecurityDescriptor (pSecurityDescriptor);
	if (Result == FALSE)
		SetLastError (RtlNtStatusToDosError (STATUS_INVALID_SECURITY_DESCR));

	return (BOOL)Result;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
MakeAbsoluteSD (
	PSECURITY_DESCRIPTOR	pSelfRelativeSecurityDescriptor,
	PSECURITY_DESCRIPTOR	pAbsoluteSecurityDescriptor,
	LPDWORD			lpdwAbsoluteSecurityDescriptorSize,
	PACL			pDacl,
	LPDWORD			lpdwDaclSize,
	PACL			pSacl,
	LPDWORD			lpdwSaclSize,
	PSID			pOwner,
	LPDWORD			lpdwOwnerSize,
	PSID			pPrimaryGroup,
	LPDWORD			lpdwPrimaryGroupSize
	)
{
	NTSTATUS Status;

	Status = RtlSelfRelativeToAbsoluteSD (pSelfRelativeSecurityDescriptor,
	                                      pAbsoluteSecurityDescriptor,
	                                      lpdwAbsoluteSecurityDescriptorSize,
	                                      pDacl,
	                                      lpdwDaclSize,
	                                      pSacl,
	                                      lpdwSaclSize,
	                                      pOwner,
	                                      lpdwOwnerSize,
	                                      pPrimaryGroup,
	                                      lpdwPrimaryGroupSize);
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
MakeSelfRelativeSD (
	PSECURITY_DESCRIPTOR	pAbsoluteSecurityDescriptor,
	PSECURITY_DESCRIPTOR	pSelfRelativeSecurityDescriptor,
	LPDWORD			lpdwBufferLength
	)
{
	NTSTATUS Status;

	Status = RtlAbsoluteToSelfRelativeSD (pAbsoluteSecurityDescriptor,
	                                      pSelfRelativeSecurityDescriptor,
	                                      (PULONG)lpdwBufferLength);
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
SetSecurityDescriptorDacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	BOOL			bDaclPresent,
	PACL			pDacl,
	BOOL			bDaclDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetDaclSecurityDescriptor (pSecurityDescriptor,
	                                       bDaclPresent,
	                                       pDacl,
	                                       bDaclDefaulted);
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
SetSecurityDescriptorGroup (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			pGroup,
	BOOL			bGroupDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetGroupSecurityDescriptor (pSecurityDescriptor,
	                                        pGroup,
	                                        bGroupDefaulted);
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
SetSecurityDescriptorOwner (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			pOwner,
	BOOL			bOwnerDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetGroupSecurityDescriptor (pSecurityDescriptor,
	                                        pOwner,
	                                        bOwnerDefaulted);
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
SetSecurityDescriptorSacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	BOOL			bSaclPresent,
	PACL			pSacl,
	BOOL			bSaclDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetSaclSecurityDescriptor (pSecurityDescriptor,
	                                       bSaclPresent,
	                                       pSacl,
	                                       bSaclDefaulted);
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
BOOL STDCALL
GetUserNameA(LPSTR lpBuffer, LPDWORD nSize)
{
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetUserNameW(LPWSTR lpBuffer, LPDWORD nSize)
{
  return(FALSE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetFileSecurityA (
    LPCSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetFileSecurityW (
    LPCWSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    )
{
  return(FALSE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetFileSecurityA (
    LPCSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
  return(FALSE);
}
/* EOF */
