/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
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
			BufferLength, PreviousState, ReturnLength );
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
	errCode = NtAdjustPrivilegesToken(TokenHandle,ResetToDefault,NewState,
			BufferLength, PreviousState, ReturnLength );
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
			TokenInformationLength, ReturnLength);
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
	errCode = NtSetnformationToken(TokenHandle,TokenInformationClass,TokenInformation,
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
	     PrivilegeSetLength,
	     GrantedAccess,
	     AccessStatus);
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


