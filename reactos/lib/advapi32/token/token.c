/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Token functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

/*
 * @implemented
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
			BufferLength, PreviousState, (PULONG)ReturnLength );
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
WINBOOL
STDCALL
SetThreadToken (
                PHANDLE ThreadHandle,
                HANDLE TokenHandle
                 )
{
	NTSTATUS errCode;
	HANDLE hThread  = NtCurrentThread();
	if ( ThreadHandle != NULL )
		hThread = ThreadHandle;
	errCode = NtSetInformationThread(hThread,ThreadImpersonationToken,TokenHandle,sizeof(HANDLE));
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}


/*
 * @implemented
 */
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
	NTSTATUS errCode;
	HANDLE NewToken;

	OBJECT_ATTRIBUTES ObjectAttributes;
	

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = NULL;
   	ObjectAttributes.Attributes = 0;
	if ( lpTokenAttributes->bInheritHandle )
		ObjectAttributes.Attributes |= OBJ_INHERIT;	

	ObjectAttributes.SecurityDescriptor = lpTokenAttributes->lpSecurityDescriptor;
	ObjectAttributes.SecurityQualityOfService = NULL;

	errCode = NtDuplicateToken(  ExistingTokenHandle, dwDesiredAccess, 
 		&ObjectAttributes, ImpersonationLevel,
		TokenType,  &NewToken     );

	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
DuplicateToken (
                HANDLE ExistingTokenHandle,
                SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                PHANDLE DuplicateTokenHandle
                 )
{
 	return DuplicateTokenEx (
                  ExistingTokenHandle,
                  TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_QUERY,
                  NULL,
                  ImpersonationLevel,
                  TokenImpersonation,
                  DuplicateTokenHandle
                   );
}

/* EOF */
