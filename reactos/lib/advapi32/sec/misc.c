/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/misc.c
 * PURPOSE:         Miscellaneous security functions
 */

#include "advapi32.h"
#include <accctrl.h>
#include <malloc.h>
#include <ntsecapi.h>

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
 * @implemented
 */
BOOL WINAPI
GetFileSecurityA(LPCSTR lpFileName,
		 SECURITY_INFORMATION RequestedInformation,
		 PSECURITY_DESCRIPTOR pSecurityDescriptor,
		 DWORD nLength,
		 LPDWORD lpnLengthNeeded)
{
  UNICODE_STRING FileName;
  NTSTATUS Status;
  BOOL bResult;

  Status = RtlCreateUnicodeStringFromAsciiz(&FileName,
					    (LPSTR)lpFileName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  bResult = GetFileSecurityW(FileName.Buffer,
			     RequestedInformation,
			     pSecurityDescriptor,
			     nLength,
			     lpnLengthNeeded);

  RtlFreeUnicodeString(&FileName);

  return bResult;
}


/*
 * @implemented
 */
BOOL WINAPI
GetFileSecurityW(LPCWSTR lpFileName,
		 SECURITY_INFORMATION RequestedInformation,
		 PSECURITY_DESCRIPTOR pSecurityDescriptor,
		 DWORD nLength,
		 LPDWORD lpnLengthNeeded)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK StatusBlock;
  UNICODE_STRING FileName;
  ULONG AccessMask = 0;
  HANDLE FileHandle;
  NTSTATUS Status;

  DPRINT("GetFileSecurityW() called\n");

  if (RequestedInformation &
      (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
      AccessMask |= STANDARD_RIGHTS_READ;
    }

  if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
      AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

  if (!RtlDosPathNameToNtPathName_U((LPWSTR)lpFileName,
				    &FileName,
				    NULL,
				    NULL))
    {
      DPRINT("Invalid path\n");
      SetLastError(ERROR_INVALID_NAME);
      return FALSE;
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      AccessMask,
		      &ObjectAttributes,
		      &StatusBlock,
		      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  RtlFreeUnicodeString(&FileName);

  Status = NtQuerySecurityObject(FileHandle,
				 RequestedInformation,
				 pSecurityDescriptor,
				 nLength,
				 lpnLengthNeeded);
  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtQuerySecurityObject() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

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
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL STDCALL
SetFileSecurityA (LPCSTR lpFileName,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  UNICODE_STRING FileName;
  NTSTATUS Status;
  BOOL bResult;

  Status = RtlCreateUnicodeStringFromAsciiz(&FileName,
					    (LPSTR)lpFileName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  bResult = SetFileSecurityW(FileName.Buffer,
			     SecurityInformation,
			     pSecurityDescriptor);

  RtlFreeUnicodeString(&FileName);

  return bResult;
}


/******************************************************************************
 * SetFileSecurityW [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL STDCALL
SetFileSecurityW (LPCWSTR lpFileName,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK StatusBlock;
  UNICODE_STRING FileName;
  ULONG AccessMask = 0;
  HANDLE FileHandle;
  NTSTATUS Status;

  DPRINT("SetFileSecurityW() called\n");

  if (SecurityInformation &
      (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
    {
      AccessMask |= WRITE_OWNER;
    }

  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      AccessMask |= WRITE_DAC;
    }

  if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
      AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

  if (!RtlDosPathNameToNtPathName_U((LPWSTR)lpFileName,
				    &FileName,
				    NULL,
				    NULL))
    {
      DPRINT("Invalid path\n");
      SetLastError(ERROR_INVALID_NAME);
      return FALSE;
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      AccessMask,
		      &ObjectAttributes,
		      &StatusBlock,
		      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  RtlFreeUnicodeString(&FileName);

  Status = NtSetSecurityObject(FileHandle,
			       SecurityInformation,
			       pSecurityDescriptor);
  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetSecurityObject() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

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
				   &NewToken,
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
 *
 * @implemented
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
  UNICODE_STRING NameW;
  ANSI_STRING NameA;
  BOOL Ret;
  
  /* apparently Win doesn't check whether lpSize is valid at all! */

  NameW.Length = 0;
  NameW.MaximumLength = (*lpSize) * sizeof(WCHAR);
  NameW.Buffer = LocalAlloc(LMEM_FIXED, NameW.MaximumLength);
  if(NameW.Buffer == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  NameA.Length = 0;
  NameA.MaximumLength = ((*lpSize) < 0xFFFF ? (USHORT)(*lpSize) : 0xFFFF);
  NameA.Buffer = lpszName;
  
  Ret = GetUserNameW(NameW.Buffer,
                     lpSize);
  if(Ret)
  {
    RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);
    NameA.Buffer[NameA.Length] = '\0';
    
    *lpSize = NameA.Length + 1;
  }
  
  LocalFree(NameW.Buffer);
  
  return Ret;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * See GetUserNameA.
 *
 * @implemented
 */
BOOL WINAPI
GetUserNameW ( LPWSTR lpszName, LPDWORD lpSize )
{
  HANDLE hToken = INVALID_HANDLE_VALUE;
  DWORD tu_len = 0;
  char* tu_buf = NULL;
  TOKEN_USER* token_user = NULL;
  DWORD an_len = 0;
  SID_NAME_USE snu = SidTypeUser;
  WCHAR* domain_name = NULL;
  DWORD dn_len = 0;

  if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken ) )
  {
    DWORD dwLastError = GetLastError();
    if ( dwLastError != ERROR_NO_TOKEN
      && dwLastError != ERROR_NO_IMPERSONATION_TOKEN )
    {
      /* don't call SetLastError(),
         as OpenThreadToken() ought to have set one */
      return FALSE;
    }
    if ( !OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
    {
      /* don't call SetLastError(),
         as OpenProcessToken() ought to have set one */
      return FALSE;
    }
  }
  tu_buf = LocalAlloc ( LMEM_FIXED, 36 );
  if ( !tu_buf )
  {
    SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
    return FALSE;
  }
  if ( !GetTokenInformation ( hToken, TokenUser, tu_buf, 36, &tu_len ) || tu_len > 36 )
  {
    LocalFree ( tu_buf );
    tu_buf = LocalAlloc ( LMEM_FIXED, tu_len );
    if ( !tu_buf )
    {
      SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }
    if ( !GetTokenInformation ( hToken, TokenUser, tu_buf, tu_len, &tu_len ) )
    {
      /* don't call SetLastError(),
         as GetTokenInformation() ought to have set one */
      LocalFree ( tu_buf );
      CloseHandle ( hToken );
      return FALSE;
    }
  }
  token_user = (TOKEN_USER*)tu_buf;

  an_len = *lpSize;
  dn_len = 32;
  domain_name = LocalAlloc ( LMEM_FIXED, dn_len * sizeof(WCHAR) );
  if ( !domain_name )
  {
    LocalFree ( tu_buf );
    SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
    return FALSE;
  }
  if ( !LookupAccountSidW ( NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu )
    || dn_len > 32 )
  {
    if ( dn_len > 32 )
    {
      LocalFree ( domain_name );
      domain_name = LocalAlloc ( LMEM_FIXED, dn_len * sizeof(WCHAR) );
      if ( !domain_name )
      {
        LocalFree ( tu_buf );
        SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
      }
    }
    if ( !LookupAccountSidW ( NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu ) )
    {
      /* don't call SetLastError(),
         as LookupAccountSid() ought to have set one */
      LocalFree ( domain_name );
      CloseHandle ( hToken );
      return FALSE;
    }
  }

  LocalFree ( domain_name );
  LocalFree ( tu_buf );
  CloseHandle ( hToken );

  if ( an_len > *lpSize )
  {
    *lpSize = an_len;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
  }

  return TRUE;
}


/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 *
 * @implemented
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
  UNICODE_STRING NameW, ReferencedDomainNameW, SystemNameW;
  DWORD szName, szReferencedDomainName;
  BOOL Ret;
  
  /*
   * save the buffer sizes the caller passed to us, as they may get modified and
   * we require the original values when converting back to ansi
   */
  szName = *cchName;
  szReferencedDomainName = *cchReferencedDomainName;
  
  /*
   * allocate buffers for the unicode strings to receive
   */
  
  if(szName > 0)
  {
    NameW.Length = 0;
    NameW.MaximumLength = szName * sizeof(WCHAR);
    NameW.Buffer = (PWSTR)LocalAlloc(LMEM_FIXED, NameW.MaximumLength);
    if(NameW.Buffer == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  }
  else
    NameW.Buffer = NULL;
  
  if(szReferencedDomainName > 0)
  {
    ReferencedDomainNameW.Length = 0;
    ReferencedDomainNameW.MaximumLength = szReferencedDomainName * sizeof(WCHAR);
    ReferencedDomainNameW.Buffer = (PWSTR)LocalAlloc(LMEM_FIXED, ReferencedDomainNameW.MaximumLength);
    if(ReferencedDomainNameW.Buffer == NULL)
    {
      if(szName > 0)
      {
        LocalFree(NameW.Buffer);
      }
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  }
  else
    ReferencedDomainNameW.Buffer = NULL;
  
  /*
   * convert the system name to unicode - if present
   */
  
  if(lpSystemName != NULL)
  {
    ANSI_STRING SystemNameA;

    RtlInitAnsiString(&SystemNameA, lpSystemName);
    RtlAnsiStringToUnicodeString(&SystemNameW, &SystemNameA, TRUE);
  }
  else
    SystemNameW.Buffer = NULL;
  
  /*
   * it's time to call the unicode version
   */
  
  Ret = LookupAccountSidW(SystemNameW.Buffer,
                          lpSid,
                          NameW.Buffer,
                          cchName,
                          ReferencedDomainNameW.Buffer,
                          cchReferencedDomainName,
                          peUse);
  if(Ret)
  {
    /*
     * convert unicode strings back to ansi, don't forget that we can't convert
     * more than 0xFFFF (USHORT) characters! Also don't forget to explicitly
     * terminate the converted string, the Rtl functions don't do that!
     */
    if(lpName != NULL)
    {
      ANSI_STRING NameA;
      
      NameA.Length = 0;
      NameA.MaximumLength = ((szName <= 0xFFFF) ? (USHORT)szName : 0xFFFF);
      NameA.Buffer = lpName;
      
      RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);
      NameA.Buffer[NameA.Length] = '\0';
    }
    
    if(lpReferencedDomainName != NULL)
    {
      ANSI_STRING ReferencedDomainNameA;

      ReferencedDomainNameA.Length = 0;
      ReferencedDomainNameA.MaximumLength = ((szReferencedDomainName <= 0xFFFF) ?
                                             (USHORT)szReferencedDomainName : 0xFFFF);
      ReferencedDomainNameA.Buffer = lpReferencedDomainName;

      RtlUnicodeStringToAnsiString(&ReferencedDomainNameA, &ReferencedDomainNameW, FALSE);
      ReferencedDomainNameA.Buffer[ReferencedDomainNameA.Length] = '\0';
    }
  }
  
  /*
   * free previously allocated buffers
   */

  if(SystemNameW.Buffer != NULL)
  {
    RtlFreeUnicodeString(&SystemNameW);
  }
  if(NameW.Buffer != NULL)
  {
    LocalFree(NameW.Buffer);
  }
  if(ReferencedDomainNameW.Buffer != NULL)
  {
    LocalFree(ReferencedDomainNameW.Buffer);
  }
  
  return Ret;
}


/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
 *
 * @implemented
 */
BOOL WINAPI
LookupAccountSidW (
	LPCWSTR pSystemName,
	PSID pSid,
	LPWSTR pAccountName,
	LPDWORD pdwAccountName,
	LPWSTR pDomainName,
	LPDWORD pdwDomainName,
	PSID_NAME_USE peUse )
{
	LSA_UNICODE_STRING SystemName;
	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
	LSA_HANDLE PolicyHandle = INVALID_HANDLE_VALUE;
	NTSTATUS Status;
	PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain = NULL;
	PLSA_TRANSLATED_NAME TranslatedName = NULL;
	BOOL ret;

	RtlInitUnicodeString ( &SystemName, pSystemName );
	ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	Status = LsaOpenPolicy ( &SystemName, &ObjectAttributes, POLICY_LOOKUP_NAMES, &PolicyHandle );
	if ( !NT_SUCCESS(Status) )
	{
		SetLastError ( LsaNtStatusToWinError(Status) );
		return FALSE;
	}
	Status = LsaLookupSids ( PolicyHandle, 1, &pSid, &ReferencedDomain, &TranslatedName );

	LsaClose ( PolicyHandle );

	if ( !NT_SUCCESS(Status) || Status == STATUS_SOME_NOT_MAPPED )
	{
		SetLastError ( LsaNtStatusToWinError(Status) );
		ret = FALSE;
	}
	else
	{
		ret = TRUE;
		if ( TranslatedName )
		{
			DWORD dwSrcLen = TranslatedName->Name.Length / sizeof(WCHAR);
			if ( *pdwAccountName <= dwSrcLen )
			{
				*pdwAccountName = dwSrcLen + 1;
				ret = FALSE;
			}
			else
			{
				*pdwAccountName = dwSrcLen;
				wcscpy ( pAccountName, TranslatedName->Name.Buffer );
			}
			if ( peUse )
				*peUse = TranslatedName->Use;
		}

		if ( ReferencedDomain )
		{
			if ( ReferencedDomain->Entries > 0 )
			{
				DWORD dwSrcLen = ReferencedDomain->Domains[0].Name.Length / sizeof(WCHAR);
				if ( *pdwDomainName <= dwSrcLen )
				{
					*pdwDomainName = dwSrcLen + 1;
					ret = FALSE;
				}
				else
				{
					*pdwDomainName = dwSrcLen;
					wcscpy ( pDomainName, ReferencedDomain->Domains[0].Name.Buffer );
				}
			}
		}

		if ( !ret )
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
	}

	if ( ReferencedDomain )
		LsaFreeMemory ( ReferencedDomain );
	if ( TranslatedName )
		LsaFreeMemory ( TranslatedName );

	return ret;
}



/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL STDCALL
LookupAccountNameA (LPCSTR SystemName,
                    LPCSTR AccountName,
		    PSID Sid,
		    LPDWORD SidLength,
		    LPSTR ReferencedDomainName,
		    LPDWORD hReferencedDomainNameLength,
		    PSID_NAME_USE SidNameUse)
{
  DPRINT1("LookupAccountNameA is unimplemented\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/******************************************************************************
 * LookupAccountNameW [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL STDCALL
LookupAccountNameW (LPCWSTR SystemName,
                    LPCWSTR AccountName,
		    PSID Sid,
		    LPDWORD SidLength,
		    LPWSTR ReferencedDomainName,
		    LPDWORD hReferencedDomainNameLength,
		    PSID_NAME_USE SidNameUse)
{
  DPRINT1("LookupAccountNameW is unimplemented\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
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
LookupPrivilegeValueW (LPCWSTR SystemName,
		       LPCWSTR PrivName,
		       PLUID Luid)
{
  static const WCHAR * const DefaultPrivNames[] =
    {
      L"SeCreateTokenPrivilege",
      L"SeAssignPrimaryTokenPrivilege",
      L"SeLockMemoryPrivilege",
      L"SeIncreaseQuotaPrivilege",
      L"SeUnsolicitedInputPrivilege",
      L"SeMachineAccountPrivilege",
      L"SeTcbPrivilege",
      L"SeSecurityPrivilege",
      L"SeTakeOwnershipPrivilege",
      L"SeLoadDriverPrivilege",
      L"SeSystemProfilePrivilege",
      L"SeSystemtimePrivilege",
      L"SeProfileSingleProcessPrivilege",
      L"SeIncreaseBasePriorityPrivilege",
      L"SeCreatePagefilePrivilege",
      L"SeCreatePermanentPrivilege",
      L"SeBackupPrivilege",
      L"SeRestorePrivilege",
      L"SeShutdownPrivilege",
      L"SeDebugPrivilege",
      L"SeAuditPrivilege",
      L"SeSystemEnvironmentPrivilege",
      L"SeChangeNotifyPrivilege",
      L"SeRemoteShutdownPrivilege",
      L"SeUndockPrivilege",
      L"SeSyncAgentPrivilege",
      L"SeEnableDelegationPrivilege",
      L"SeManageVolumePrivilege",
      L"SeImpersonatePrivilege",
      L"SeCreateGlobalPrivilege"
    };
  unsigned Priv;

  if (NULL != SystemName && L'\0' != *SystemName)
    {
      DPRINT1("LookupPrivilegeValueW: not implemented for remote system\n");
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
      return FALSE;
    }

  for (Priv = 0; Priv < sizeof(DefaultPrivNames) / sizeof(DefaultPrivNames[0]); Priv++)
    {
      if (0 == wcscmp(PrivName, DefaultPrivNames[Priv]))
        {
          Luid->LowPart = Priv + 1;
          Luid->HighPart = 0;
          return TRUE;
        }
    }

  DPRINT1("LookupPrivilegeValueW: no such privilege %S\n", PrivName);
  SetLastError(ERROR_NO_SUCH_PRIVILEGE);
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
  DPRINT1("LookupPrivilegeDisplayNameA: stub\n");
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
  DPRINT1("LookupPrivilegeDisplayNameW: stub\n");
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
  DPRINT1("LookupPrivilegeNameA: stub\n");
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
  DPRINT1("LookupPrivilegeNameW: stub\n");
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
  DPRINT1("GetNamedSecurityInfoW: stub\n");
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
  DPRINT1("GetNamedSecurityInfoA: stub\n");
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
  DPRINT1("SetNamedSecurityInfoW: stub\n");
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
  DPRINT1("SetNamedSecurityInfoA: stub\n");
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * GetSecurityInfo				EXPORTED
 *
 * @unimplemented
 */
DWORD STDCALL
GetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID* ppsidOwner,
                PSID* ppsidGroup,
                PACL* ppDacl,
                PACL* ppSacl,
                PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
  DPRINT1("GetSecurityInfo: stub\n");
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 * SetSecurityInfo				EXPORTED
 *
 * @unimplemented
 */
DWORD
WINAPI
SetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID psidOwner,
                PSID psidGroup,
                PACL pDacl,
                PACL pSacl)
{
  DPRINT1("SetSecurityInfo: stub\n");
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD WINAPI GetSecurityInfoExA(
   HANDLE hObject, 
   SE_OBJECT_TYPE ObjectType, 
   SECURITY_INFORMATION SecurityInfo, 
   LPCSTR lpProvider,
   LPCSTR lpProperty, 
   PACTRL_ACCESSA *ppAccessList, 
   PACTRL_AUDITA *ppAuditList, 
   LPSTR *lppOwner, 
   LPSTR *lppGroup
   )
{
  DPRINT1("GetSecurityInfoExA stub!\n");
  return ERROR_BAD_PROVIDER; 
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD WINAPI GetSecurityInfoExW(
   HANDLE hObject, 
   SE_OBJECT_TYPE ObjectType, 
   SECURITY_INFORMATION SecurityInfo, 
   LPCWSTR lpProvider,
   LPCWSTR lpProperty, 
   PACTRL_ACCESSW *ppAccessList, 
   PACTRL_AUDITW *ppAuditList, 
   LPWSTR *lppOwner, 
   LPWSTR *lppGroup
   )
{
  DPRINT1("GetSecurityInfoExW stub!\n");
  return ERROR_BAD_PROVIDER; 
}


/**********************************************************************
 * ImpersonateNamedPipeClient			EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
ImpersonateNamedPipeClient(HANDLE hNamedPipe)
{
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  DPRINT("ImpersonateNamedPipeClient() called\n");

  Status = NtFsControlFile(hNamedPipe,
			   NULL,
			   NULL,
			   NULL,
			   &StatusBlock,
			   FSCTL_PIPE_IMPERSONATE,
			   NULL,
			   0,
			   NULL,
			   0);
  if (!NT_SUCCESS(Status))
  {
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
  }

  return TRUE;
}

/* EOF */
