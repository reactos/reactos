/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>

WINBOOL
STDCALL
AdjustTokenGroups (
		   HANDLE TokenHandle,
		   WINBOOL ResetToDefault,
		   PTOKEN_GROUPS NewState,
		   DWORD BufferLength,
		   PTOKEN_GROUPS PreviousState,
		   PDWORD ReturnLength
		    )
{
	NTSTATUS errCode;
	errCode = NtAdjustGroupsToken(TokenHandle,ResetToDefault,NewState,
			BufferLength, PreviousState, (PULONG)ReturnLength );
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
}

WINBOOL
STDCALL
AdjustTokenPrivileges (
		       HANDLE TokenHandle,
		       WINBOOL DisableAllPrivileges,
		       PTOKEN_PRIVILEGES NewState,
		       DWORD BufferLength,
		       PTOKEN_PRIVILEGES PreviousState,
		       PDWORD ReturnLength
			)
{	NTSTATUS errCode;
	errCode = NtAdjustPrivilegesToken(TokenHandle,DisableAllPrivileges,NewState,
			BufferLength, PreviousState, (PULONG)ReturnLength );
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
}


WINBOOL
STDCALL
GetTokenInformation (
		     HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength,
		     PDWORD ReturnLength
		      )
{
	NTSTATUS errCode;
	errCode = NtQueryInformationToken(TokenHandle,TokenInformationClass,TokenInformation,
			TokenInformationLength, (PULONG)ReturnLength);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

WINBOOL
STDCALL
SetTokenInformation (
		     HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength
		      )
{
	NTSTATUS errCode;
	errCode = NtSetInformationToken(TokenHandle,TokenInformationClass,TokenInformation,
			TokenInformationLength);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

WINBOOL
STDCALL
AccessCheck (
	     PSECURITY_DESCRIPTOR pSecurityDescriptor,
	     HANDLE ClientToken,
	     DWORD DesiredAccess,
	     PGENERIC_MAPPING GenericMapping,
	     PPRIVILEGE_SET PrivilegeSet,
	     LPDWORD PrivilegeSetLength,
	     LPDWORD GrantedAccess,
	     LPBOOL AccessStatus
	      )
{
	NTSTATUS errCode;
	errCode = NtAccessCheck( pSecurityDescriptor,
	     ClientToken,
	     DesiredAccess,
	     GenericMapping,
             PrivilegeSet,
	     (PULONG)PrivilegeSetLength,
	     (PULONG)GrantedAccess,
	     (PBOOLEAN)AccessStatus);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}



WINBOOL
STDCALL
OpenProcessToken (
		  HANDLE ProcessHandle,
		  DWORD DesiredAccess,
		  PHANDLE TokenHandle
		   )
{
	NTSTATUS errCode;
	errCode = NtOpenProcessToken(ProcessHandle,DesiredAccess,TokenHandle);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}



WINBOOL
STDCALL
OpenThreadToken (
		 HANDLE ThreadHandle,
		 DWORD DesiredAccess,
		 WINBOOL OpenAsSelf,
		 PHANDLE TokenHandle
		  )
{
	NTSTATUS errCode;
	errCode = NtOpenThreadToken(ThreadHandle,DesiredAccess,OpenAsSelf,TokenHandle);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}


WINBOOL
STDCALL
SetThreadToken (
                PHANDLE ThreadHandle,
                HANDLE TokenHandle
                 )
{
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
}


WINBOOL
STDCALL
DuplicateToken (
                HANDLE ExistingTokenHandle,
                SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                PHANDLE DuplicateTokenHandle
                 )
{
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
}



WINBOOL
STDCALL
DuplicateTokenEx (
                  HANDLE ExistingTokenHandle,
                  DWORD  dwDesiredAccess,
                  LPSECURITY_ATTRIBUTES lpTokenAttributes,
                  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                  TOKEN_TYPE TokenType,
                  PHANDLE DuplicateTokenHandle
                   )
{
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
}


/* EOF */
