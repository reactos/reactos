/* $Id: misc.c,v 1.11 2004/03/08 18:09:05 sedwards Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/misc.c
 * PURPOSE:         Miscellaneous security functions
 */

#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


/*
 * @implemented
 */
BOOL STDCALL
AreAllAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAllAccessesGranted(GrantedAccess,
					DesiredAccess));
}


/*
 * @implemented
 */
BOOL STDCALL
AreAnyAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAnyAccessesGranted(GrantedAccess,
					DesiredAccess));
}


/*
 * @implemented
 */
BOOL STDCALL
GetKernelObjectSecurity(HANDLE Handle,
			SECURITY_INFORMATION RequestedInformation,
			PSECURITY_DESCRIPTOR pSecurityDescriptor,
			DWORD nLength,
			LPDWORD lpnLengthNeeded)
{
  NTSTATUS Status;

  Status = NtQuerySecurityObject(Handle,
				 RequestedInformation,
				 pSecurityDescriptor,
				 nLength,
				 lpnLengthNeeded);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
SetKernelObjectSecurity(HANDLE Handle,
			SECURITY_INFORMATION SecurityInformation,
			PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  NTSTATUS Status;

  Status = NtSetSecurityObject(Handle,
			       SecurityInformation,
			       SecurityDescriptor);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
VOID STDCALL
MapGenericMask(PDWORD AccessMask,
	       PGENERIC_MAPPING GenericMapping)
{
  RtlMapGenericMask(AccessMask,
		    GenericMapping);
}


/*
 * @unimplemented
 */
BOOL STDCALL
ImpersonateLoggedOnUser(HANDLE hToken)
{
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  NTSTATUS Status;

  Status = RtlImpersonateSelf(ImpersonationLevel);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
RevertToSelf(VOID)
{
  NTSTATUS Status;
  HANDLE Token = NULL;

  Status = NtSetInformationThread(NtCurrentThread(),
				  ThreadImpersonationToken,
				  &Token,
				  sizeof(HANDLE));
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }
  return(TRUE);
}


/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 *
 * Get the current user name.
 *
 * PARAMS
 *  lpszName [O]   Destination for the user name.
 *  lpSize   [I/O] Size of lpszName.
 *
 * RETURNS
 *  Success: The length of the user name, including terminating NUL.
 *  Failure: ERROR_MORE_DATA if *lpSize is too small.
 *
 * @unimplemented
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
//  size_t len;
//  char name[] = { "Administrator" };

  /* We need to include the null character when determining the size of the buffer. */
//  len = strlen(name) + 1;
//  if (len > *lpSize)
//  {
//    SetLastError(ERROR_MORE_DATA);
//    *lpSize = len;
//    return 0;
//  }

//  *lpSize = len;
//  strcpy(lpszName, name);
  return TRUE;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * See GetUserNameA.
 *
 * @unimplemented
 */
BOOL WINAPI
GetUserNameW( LPWSTR lpszName, LPDWORD lpSize )
{
//    char name[] = { "Administrator" };

//    DWORD len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );

//    if (len > *lpSize)
//    {
//        SetLastError(ERROR_MORE_DATA);
//        *lpSize = len;
//        return FALSE;
//    }

//    *lpSize = len;
//    MultiByteToWideChar( CP_ACP, 0, name, -1, lpszName, len );
    return TRUE;
}

/* EOF */
