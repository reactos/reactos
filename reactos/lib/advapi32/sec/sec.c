/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>


BOOL
STDCALL
GetSecurityDescriptorControl (
	PSECURITY_DESCRIPTOR		pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL	pControl,
	LPDWORD				lpdwRevision
	)
{
#if 0
	NTSTATUS Status;

	Status = RtlGetControlSecurityDescriptor (pSecurityDescriptor,
	                                          pControl,
	                                          lpdwRevision);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
#endif

	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


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


DWORD
STDCALL
GetSecurityDescriptorLength (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	return RtlLengthSecurityDescriptor(pSecurityDescriptor);
}


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


/* GetSecurityDescriptorSacl */


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

BOOL
STDCALL
IsValidSecurityDescriptor (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	BOOL Result;

	Result = RtlValidSecurityDescriptor (pSecurityDescriptor);
	if (Result == FALSE)
		SetLastError (RtlNtStatusToDosError (STATUS_INVALID_SECURITY_DESCR));

	return Result;
}


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


/* SetSecurityDescriptorSacl */


/* EOF */
