/* $Id: misc.c,v 1.16 2004/05/26 09:50:10 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/misc.c
 * PURPOSE:         Miscellaneous security functions
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <Accctrl.h>

#define NDEBUG
#include <debug.h>


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


/******************************************************************************
 * GetFileSecurityA [ADVAPI32.@]
 *
 * Obtains Specified information about the security of a file or directory.
 *
 * PARAMS
 *  lpFileName           [I] Name of the file to get info for
 *  RequestedInformation [I] SE_ flags from "winnt.h"
 *  pSecurityDescriptor  [O] Destination for security information
 *  nLength              [I] Length of pSecurityDescriptor
 *  lpnLengthNeeded      [O] Destination for length of returned security information
 *
 * RETURNS
 *  Success: TRUE. pSecurityDescriptor contains the requested information.
 *  Failure: FALSE. lpnLengthNeeded contains the required space to return the info. 
 *
 * NOTES
 *  The information returned is constrained by the callers access rights and
 *  privileges.
 *
 * @unimplemented
 */
BOOL WINAPI
GetFileSecurityA (LPCSTR lpFileName,
		  SECURITY_INFORMATION RequestedInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor,
		  DWORD nLength,
		  LPDWORD lpnLengthNeeded)
{
  DPRINT("GetFileSecurityA: stub\n");
  return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
GetFileSecurityW (LPCWSTR lpFileName,
                  SECURITY_INFORMATION RequestedInformation,
                  PSECURITY_DESCRIPTOR pSecurityDescriptor,
                  DWORD nLength, LPDWORD lpnLengthNeeded)
{
  DPRINT("GetFileSecurityW: stub\n");
  return TRUE;
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


/******************************************************************************
 * SetFileSecurityW [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @unimplemented
 */
BOOL STDCALL
SetFileSecurityW (LPCWSTR lpFileName,
		  SECURITY_INFORMATION RequestedInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  DPRINT("SetFileSecurityA : stub\n");
  return TRUE;
}


/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @unimplemented
 */
BOOL STDCALL
SetFileSecurityA (LPCSTR lpFileName,
		  SECURITY_INFORMATION RequestedInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  DPRINT("SetFileSecurityA : stub\n");
  return TRUE;
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
      return FALSE;
    }
  return TRUE;
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
 * @implemented
 */
BOOL STDCALL
ImpersonateLoggedOnUser(HANDLE hToken)
{
  SECURITY_QUALITY_OF_SERVICE Qos;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE NewToken;
  TOKEN_TYPE Type;
  ULONG ReturnLength;
  BOOL Duplicated;
  NTSTATUS Status;

  /* Get the token type */
  Status = NtQueryInformationToken (hToken,
				    TokenType,
				    &Type,
				    sizeof(TOKEN_TYPE),
				    &ReturnLength);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (Type == TokenPrimary)
    {
      /* Create a duplicate impersonation token */
      Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
      Qos.ImpersonationLevel = SecurityImpersonation;
      Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
      Qos.EffectiveOnly = FALSE;

      ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
      ObjectAttributes.RootDirectory = NULL;
      ObjectAttributes.ObjectName = NULL;
      ObjectAttributes.Attributes = 0;
      ObjectAttributes.SecurityDescriptor = NULL;
      ObjectAttributes.SecurityQualityOfService = &Qos;

      Status = NtDuplicateToken (hToken,
				 TOKEN_IMPERSONATE | TOKEN_QUERY,
				 &ObjectAttributes,
				 FALSE,
				 TokenImpersonation,
				 &NewToken);
      if (!NT_SUCCESS(Status))
	{
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}

      Duplicated = TRUE;
    }
  else
    {
      /* User the original impersonation token */
      NewToken = hToken;
      Duplicated = FALSE;
    }

  /* Impersonate the the current thread */
  Status = NtSetInformationThread (NtCurrentThread (),
				   ThreadImpersonationToken,
				   NewToken,
				   sizeof(HANDLE));

  if (Duplicated == TRUE)
    {
      NtClose (NewToken);
    }

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
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  NTSTATUS Status;

  Status = RtlImpersonateSelf(ImpersonationLevel);
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
      return FALSE;
    }
  return TRUE;
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


/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL STDCALL
LookupAccountSidA (LPCSTR lpSystemName,
		   PSID lpSid,
		   LPSTR lpName,
		   LPDWORD cchName,
		   LPSTR lpReferencedDomainName,
		   LPDWORD cchReferencedDomainName,
		   PSID_NAME_USE peUse)
{
  DPRINT1("LookupAccountSidA is unimplemented, but returns success\n");
  return TRUE;
}


/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL STDCALL
LookupAccountSidW (LPCWSTR lpSystemName,
		   PSID lpSid,
		   LPWSTR lpName,
		   LPDWORD cchName,
		   LPWSTR lpReferencedDomainName,
		   LPDWORD cchReferencedDomainName,
		   PSID_NAME_USE peUse)
{
  DPRINT1("LookupAccountSidW is unimplemented, but returns success\n");
  return TRUE;
}


/**********************************************************************
 * LookupPrivilegeValueA				EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
LookupPrivilegeValueA (LPCSTR lpSystemName,
		       LPCSTR lpName,
		       PLUID lpLuid)
{
  UNICODE_STRING SystemName;
  UNICODE_STRING Name;
  BOOL Result;

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlCreateUnicodeStringFromAsciiz (&SystemName,
					(LPSTR)lpSystemName);
    }

  /* Check the privilege name is not NULL */
  if (lpName == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  RtlCreateUnicodeStringFromAsciiz (&Name,
				    (LPSTR)lpName);

  Result = LookupPrivilegeValueW ((lpSystemName != NULL) ? SystemName.Buffer : NULL,
				  Name.Buffer,
				  lpLuid);

  RtlFreeUnicodeString (&Name);

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlFreeUnicodeString (&SystemName);
    }

  return Result;
}


/**********************************************************************
 * LookupPrivilegeValueW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeValueW (LPCWSTR lpSystemName,
		       LPCWSTR lpName,
		       PLUID lpLuid)
{
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameA (LPCSTR lpSystemName,
			     LPCSTR lpName,
			     LPSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeDisplayNameW			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameW (LPCWSTR lpSystemName,
			     LPCWSTR lpName,
			     LPWSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeNameA				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameA (LPCSTR lpSystemName,
		      PLUID lpLuid,
		      LPSTR lpName,
		      LPDWORD cbName)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeNameW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameW (LPCWSTR lpSystemName,
		      PLUID lpLuid,
		      LPWSTR lpName,
		      LPDWORD cbName)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * GetNamedSecurityInfoW			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
GetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * GetNamedSecurityInfoA			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
GetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * SetNamedSecurityInfoW			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
SetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * SetNamedSecurityInfoA			EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
SetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
  return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
