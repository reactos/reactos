/* $Id: token.c,v 1.15 2004/12/10 16:50:37 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Token functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include "advapi32.h"


/*
 * @implemented
 */
BOOL STDCALL
AdjustTokenGroups (HANDLE TokenHandle,
		   BOOL ResetToDefault,
		   PTOKEN_GROUPS NewState,
		   DWORD BufferLength,
		   PTOKEN_GROUPS PreviousState,
		   PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtAdjustGroupsToken (TokenHandle,
				ResetToDefault,
				NewState,
				BufferLength,
				PreviousState,
				(PULONG)ReturnLength);
  if (!NT_SUCCESS (Status))
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
AdjustTokenPrivileges (HANDLE TokenHandle,
		       BOOL DisableAllPrivileges,
		       PTOKEN_PRIVILEGES NewState,
		       DWORD BufferLength,
		       PTOKEN_PRIVILEGES PreviousState,
		       PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtAdjustPrivilegesToken (TokenHandle,
				    DisableAllPrivileges,
				    NewState,
				    BufferLength,
				    PreviousState,
				    (PULONG)ReturnLength);
  if (STATUS_NOT_ALL_ASSIGNED == Status)
    {
      SetLastError(ERROR_NOT_ALL_ASSIGNED);
      return TRUE;
    }
  if (! NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  SetLastError(ERROR_SUCCESS); /* AdjustTokenPrivileges is documented to do this */
  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetTokenInformation (HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength,
		     PDWORD ReturnLength)
{
  NTSTATUS Status;

  Status = NtQueryInformationToken (TokenHandle,
				    TokenInformationClass,
				    TokenInformation,
				    TokenInformationLength,
				    (PULONG)ReturnLength);
  if (!NT_SUCCESS (Status))
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
SetTokenInformation (HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength)
{
  NTSTATUS Status;

  Status = NtSetInformationToken (TokenHandle,
				  TokenInformationClass,
				  TokenInformation,
				  TokenInformationLength);
  if (!NT_SUCCESS (Status))
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
AccessCheck (PSECURITY_DESCRIPTOR pSecurityDescriptor,
	     HANDLE ClientToken,
	     DWORD DesiredAccess,
	     PGENERIC_MAPPING GenericMapping,
	     PPRIVILEGE_SET PrivilegeSet,
	     LPDWORD PrivilegeSetLength,
	     LPDWORD GrantedAccess,
	     LPBOOL AccessStatus)
{
  NTSTATUS Status;
  NTSTATUS AccessStat;

  Status = NtAccessCheck (pSecurityDescriptor,
			  ClientToken,
			  DesiredAccess,
			  GenericMapping,
			  PrivilegeSet,
			  (PULONG)PrivilegeSetLength,
			  (PACCESS_MASK)GrantedAccess,
			  &AccessStat);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (!NT_SUCCESS (AccessStat))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      *AccessStatus = FALSE;
      return TRUE;
    }

  *AccessStatus = TRUE;

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
OpenProcessToken (HANDLE ProcessHandle,
		  DWORD DesiredAccess,
		  PHANDLE TokenHandle)
{
  NTSTATUS Status;

  Status = NtOpenProcessToken (ProcessHandle,
			       DesiredAccess,
			       TokenHandle);
  if (!NT_SUCCESS (Status))
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
OpenThreadToken (HANDLE ThreadHandle,
		 DWORD DesiredAccess,
		 BOOL OpenAsSelf,
		 PHANDLE TokenHandle)
{
  NTSTATUS Status;

  Status = NtOpenThreadToken (ThreadHandle,
			      DesiredAccess,
			      OpenAsSelf,
			      TokenHandle);
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
SetThreadToken (PHANDLE ThreadHandle,
                HANDLE TokenHandle)
{
  NTSTATUS Status;
  HANDLE hThread;

  hThread = NtCurrentThread();
  if (ThreadHandle != NULL)
    hThread = ThreadHandle;

  Status = NtSetInformationThread (hThread,
				   ThreadImpersonationToken,
				   &TokenHandle,
				   sizeof(HANDLE));
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
DuplicateTokenEx (HANDLE ExistingTokenHandle,
                  DWORD  dwDesiredAccess,
                  LPSECURITY_ATTRIBUTES lpTokenAttributes,
                  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                  TOKEN_TYPE TokenType,
                  PHANDLE DuplicateTokenHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_QUALITY_OF_SERVICE Qos;
  HANDLE NewToken;
  NTSTATUS Status;

  Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  Qos.ImpersonationLevel = ImpersonationLevel;
  Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
  Qos.EffectiveOnly = FALSE;

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  if (lpTokenAttributes->bInheritHandle)
    {
      ObjectAttributes.Attributes |= OBJ_INHERIT;
    }
  ObjectAttributes.SecurityDescriptor = lpTokenAttributes->lpSecurityDescriptor;
  ObjectAttributes.SecurityQualityOfService = &Qos;

  Status = NtDuplicateToken (ExistingTokenHandle,
			     dwDesiredAccess,
			     &ObjectAttributes,
			     FALSE,
			     TokenType,
			     &NewToken);
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
BOOL STDCALL
DuplicateToken (HANDLE ExistingTokenHandle,
                SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                PHANDLE DuplicateTokenHandle)
{
  return DuplicateTokenEx (ExistingTokenHandle,
                           TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
                           NULL,
                           ImpersonationLevel,
                           TokenImpersonation,
                           DuplicateTokenHandle);
}

/* EOF */
