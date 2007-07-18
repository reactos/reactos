/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Security descriptor functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Steven Edwards ( Steven_Ed4153@yahoo.com )
 *                  Andrew Greenwood ( silverblade_uk@hotmail.com )
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <advapi32.h>

#define NDEBUG
#include <debug.h>

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
DWORD
STDCALL
GetSecurityDescriptorRMControl (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	PUCHAR			RMControl)
{
  if (!RtlGetSecurityDescriptorRMControl(SecurityDescriptor,
					 RMControl))
    return ERROR_INVALID_DATA;

  return ERROR_SUCCESS;
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
BOOL
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
BOOL
STDCALL
MakeAbsoluteSD2(IN OUT PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
                OUT LPDWORD lpdwBufferSize)
{
    NTSTATUS Status;

    Status = RtlSelfRelativeToAbsoluteSD2(pSelfRelativeSecurityDescriptor,
                                          lpdwBufferSize);
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
SetSecurityDescriptorControl (
	PSECURITY_DESCRIPTOR		pSecurityDescriptor,
	SECURITY_DESCRIPTOR_CONTROL	ControlBitsOfInterest,
	SECURITY_DESCRIPTOR_CONTROL	ControlBitsToSet)
{
	NTSTATUS Status;

	Status = RtlSetControlSecurityDescriptor(pSecurityDescriptor,
	                                         ControlBitsOfInterest,
	                                         ControlBitsToSet);
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

	Status = RtlSetOwnerSecurityDescriptor (pSecurityDescriptor,
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
DWORD
STDCALL
SetSecurityDescriptorRMControl (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	PUCHAR			RMControl)
{
  RtlSetSecurityDescriptorRMControl(SecurityDescriptor,
				    RMControl);

  return ERROR_SUCCESS;
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
 * @implemented
 */
VOID
WINAPI
QuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}


/*
 * @implemented
 */
VOID
WINAPI
SetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                      OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        *DesiredAccess |= WRITE_OWNER;

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
        *DesiredAccess |= WRITE_DAC;

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ConvertToAutoInheritPrivateObjectSecurity(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                          IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
                                          OUT PSECURITY_DESCRIPTOR* NewSecurityDescriptor,
                                          IN GUID* ObjectType,
                                          IN BOOLEAN IsDirectoryObject,
                                          IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
BuildSecurityDescriptorW(IN PTRUSTEE_W pOwner  OPTIONAL,
                         IN PTRUSTEE_W pGroup  OPTIONAL,
                         IN ULONG cCountOfAccessEntries,
                         IN PEXPLICIT_ACCESS_W pListOfAccessEntries  OPTIONAL,
                         IN ULONG cCountOfAuditEntries,
                         IN PEXPLICIT_ACCESS_W pListOfAuditEntries  OPTIONAL,
                         IN PSECURITY_DESCRIPTOR pOldSD  OPTIONAL,
                         OUT PULONG pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR* pNewSD)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
BuildSecurityDescriptorA(IN PTRUSTEE_A pOwner  OPTIONAL,
                         IN PTRUSTEE_A pGroup  OPTIONAL,
                         IN ULONG cCountOfAccessEntries,
                         IN PEXPLICIT_ACCESS_A pListOfAccessEntries  OPTIONAL,
                         IN ULONG cCountOfAuditEntries,
                         IN PEXPLICIT_ACCESS_A pListOfAuditEntries  OPTIONAL,
                         IN PSECURITY_DESCRIPTOR pOldSD  OPTIONAL,
                         OUT PULONG pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR* pNewSD)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved)
{
    DPRINT1("%s(%S) not implemented!\n", __FUNCTION__, lpFileName);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI DecryptFileA(LPCSTR lpFileName, DWORD dwReserved)
{
    UNICODE_STRING FileName;
    NTSTATUS Status;
    BOOL ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    ret = DecryptFileW(FileName.Buffer, dwReserved);

    if (FileName.Buffer != NULL)
        RtlFreeUnicodeString(&FileName);
    return ret;
}

/*
 * @unimplemented
 */
BOOL WINAPI EncryptFileW(LPCWSTR lpFileName)
{
    DPRINT1("%s() not implemented!\n", __FUNCTION__);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI EncryptFileA(LPCSTR lpFileName)
{
    UNICODE_STRING FileName;
    NTSTATUS Status;
    BOOL ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    ret = EncryptFileW(FileName.Buffer);

    if (FileName.Buffer != NULL)
        RtlFreeUnicodeString(&FileName);
    return ret;
}

BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorW(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    DWORD dword, 
    SECURITY_INFORMATION SecurityInformation, 
    LPWSTR* lpwstr,
    PULONG pulong)
{
    DPRINT1("%s() not implemented!\n", __FUNCTION__);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorA(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    DWORD dword, 
    SECURITY_INFORMATION SecurityInformation, 
    LPSTR* lpstr,
    PULONG pulong)
{
    DPRINT1("%s() not implemented!\n", __FUNCTION__);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* EOF */
