/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/token.c
 * PURPOSE:         Token functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include "advapi32.h"

#define NDEBUG
#include <debug.h>

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
SetThreadToken (IN PHANDLE ThreadHandle  OPTIONAL,
                IN HANDLE TokenHandle)
{
  NTSTATUS Status;
  HANDLE hThread;

  hThread = ((ThreadHandle != NULL) ? *ThreadHandle : NtCurrentThread());

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
DuplicateTokenEx (IN HANDLE ExistingTokenHandle,
                  IN DWORD dwDesiredAccess,
                  IN LPSECURITY_ATTRIBUTES lpTokenAttributes  OPTIONAL,
                  IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                  IN TOKEN_TYPE TokenType,
                  OUT PHANDLE DuplicateTokenHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE NewToken;
  NTSTATUS Status;
  SECURITY_QUALITY_OF_SERVICE Sqos;

  Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  Sqos.ImpersonationLevel = ImpersonationLevel;
  Sqos.ContextTrackingMode = 0;
  Sqos.EffectiveOnly = FALSE;

  if (lpTokenAttributes != NULL)
    {
      InitializeObjectAttributes(&ObjectAttributes,
                                 NULL,
                                 lpTokenAttributes->bInheritHandle ? OBJ_INHERIT : 0,
                                 NULL,
                                 lpTokenAttributes->lpSecurityDescriptor);
    }
  else
    {
      InitializeObjectAttributes(&ObjectAttributes,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    }

  ObjectAttributes.SecurityQualityOfService = &Sqos;

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
DuplicateToken (IN HANDLE ExistingTokenHandle,
                IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                OUT PHANDLE DuplicateTokenHandle)
{
  return DuplicateTokenEx (ExistingTokenHandle,
                           TOKEN_IMPERSONATE | TOKEN_QUERY,
                           NULL,
                           ImpersonationLevel,
                           TokenImpersonation,
                           DuplicateTokenHandle);
}


/*
 * @implemented
 */
BOOL STDCALL
CheckTokenMembership (HANDLE ExistingTokenHandle,
                      PSID SidToCheck,
                      PBOOL IsMember)
{
  HANDLE AccessToken = NULL;
  BOOL Result = FALSE;
  DWORD dwSize;
  DWORD i;
  PTOKEN_GROUPS lpGroups = NULL;
  TOKEN_TYPE TokenInformation;

  if (IsMember == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (ExistingTokenHandle == NULL)
  {
    /* Get impersonation token of the calling thread */
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &ExistingTokenHandle))
      return FALSE;

    if (!DuplicateToken(ExistingTokenHandle, SecurityAnonymous, &AccessToken))
    {
      CloseHandle(ExistingTokenHandle);
      goto ByeBye;
    }
    CloseHandle(ExistingTokenHandle);
  }
  else
  {
    if (!GetTokenInformation(ExistingTokenHandle, TokenType, &TokenInformation, sizeof(TokenInformation), &dwSize))
      goto ByeBye;
    if (TokenInformation != TokenImpersonation)
    {
      /* Duplicate token to have a impersonation token */
      if (!DuplicateToken(ExistingTokenHandle, SecurityAnonymous, &AccessToken))
        return FALSE;
    }
    else
      AccessToken = ExistingTokenHandle;
  }

  *IsMember = FALSE;
  /* Search in groups of the token */
  if (!GetTokenInformation(AccessToken, TokenGroups, NULL, 0, &dwSize))
    goto ByeBye;
  lpGroups = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), 0, dwSize);
  if (!lpGroups)
    goto ByeBye;
  if (!GetTokenInformation(AccessToken, TokenGroups, lpGroups, dwSize, &dwSize))
    goto ByeBye;
  for (i = 0; i < lpGroups->GroupCount; i++)
  {
    if (EqualSid(SidToCheck, &lpGroups->Groups[i].Sid))
    {
      Result = TRUE;
      *IsMember = TRUE;
      goto ByeBye;
    }
  }
  /* FIXME: Search in users of the token? */
  DPRINT1("CheckTokenMembership() partially implemented!\n");
  Result = TRUE;

ByeBye:
  if (lpGroups != NULL)
    HeapFree(GetProcessHeap(), 0, lpGroups);
  if (AccessToken != NULL && AccessToken != ExistingTokenHandle)
    CloseHandle(AccessToken);

  return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
IsTokenRestricted(HANDLE TokenHandle)
{
  ULONG RetLength;
  PTOKEN_GROUPS lpGroups;
  NTSTATUS Status;
  BOOL Ret = FALSE;
  
  /* determine the required buffer size and allocate enough memory to read the
     list of restricted SIDs */

  Status = NtQueryInformationToken(TokenHandle,
                                   TokenRestrictedSids,
                                   NULL,
                                   0,
                                   &RetLength);
  if (Status != STATUS_BUFFER_TOO_SMALL)
  {
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
  }
  
AllocAndReadRestrictedSids:
  lpGroups = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
                                      0,
                                      RetLength);
  if (lpGroups == NULL)
  {
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
  }
  
  /* actually read the list of the restricted SIDs */
  
  Status = NtQueryInformationToken(TokenHandle,
                                   TokenRestrictedSids,
                                   lpGroups,
                                   RetLength,
                                   &RetLength);
  if (NT_SUCCESS(Status))
  {
    Ret = (lpGroups->GroupCount != 0);
  }
  else if (Status == STATUS_BUFFER_TOO_SMALL)
  {
    /* looks like the token was modified in the meanwhile, let's just try again */

    HeapFree(GetProcessHeap(),
             0,
             lpGroups);

    goto AllocAndReadRestrictedSids;
  }
  else
  {
    SetLastError(RtlNtStatusToDosError(Status));
  }
  
  /* free allocated memory */

  HeapFree(GetProcessHeap(),
           0,
           lpGroups);

  return Ret;
}

/* EOF */
