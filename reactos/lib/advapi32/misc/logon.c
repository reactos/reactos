/* $Id: logon.c,v 1.2 2004/01/20 23:16:42 ekohl Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/logon.c
 * PURPOSE:     Logon functions
 * PROGRAMMER:  Eric Kohl
 */

#include <windows.h>

#define NTOS_MODE_USER
#include <ntos.h>

#include <string.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
CreateProcessAsUserA (HANDLE hToken,
		      LPCSTR lpApplicationName,
		      LPSTR lpCommandLine,
		      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		      BOOL bInheritHandles,
		      DWORD dwCreationFlags,
		      LPVOID lpEnvironment,
		      LPCSTR lpCurrentDirectory,
		      LPSTARTUPINFOA lpStartupInfo,
		      LPPROCESS_INFORMATION lpProcessInformation)
{
  NTSTATUS Status;

  /* Create the process with a suspended main thread */
  if (!CreateProcessA (lpApplicationName,
		       lpCommandLine,
		       lpProcessAttributes,
		       lpThreadAttributes,
		       bInheritHandles,
		       dwCreationFlags | CREATE_SUSPENDED,
		       lpEnvironment,
		       lpCurrentDirectory,
		       lpStartupInfo,
		       lpProcessInformation))
    {
      return FALSE;
    }

  /* Set the new process token */
  Status = NtSetInformationProcess (lpProcessInformation->hProcess,
				    ProcessAccessToken,
				    (PVOID)&hToken,
				    sizeof (HANDLE));
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  /* Resume the main thread */
  if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
      ResumeThread (lpProcessInformation->hThread);
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CreateProcessAsUserW (HANDLE hToken,
		      LPCWSTR lpApplicationName,
		      LPWSTR lpCommandLine,
		      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		      BOOL bInheritHandles,
		      DWORD dwCreationFlags,
		      LPVOID lpEnvironment,
		      LPCWSTR lpCurrentDirectory,
		      LPSTARTUPINFOW lpStartupInfo,
		      LPPROCESS_INFORMATION lpProcessInformation)
{
  NTSTATUS Status;

  /* Create the process with a suspended main thread */
  if (!CreateProcessW (lpApplicationName,
		       lpCommandLine,
		       lpProcessAttributes,
		       lpThreadAttributes,
		       bInheritHandles,
		       dwCreationFlags | CREATE_SUSPENDED,
		       lpEnvironment,
		       lpCurrentDirectory,
		       lpStartupInfo,
		       lpProcessInformation))
    {
      return FALSE;
    }

  /* Set the new process token */
  Status = NtSetInformationProcess (lpProcessInformation->hProcess,
				    ProcessAccessToken,
				    (PVOID)&hToken,
				    sizeof (HANDLE));
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  /* Resume the main thread */
  if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
      ResumeThread (lpProcessInformation->hThread);
    }

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreateProcessWithLogonW (LPCWSTR lpUsername,
			 LPCWSTR lpDomain,
			 LPCWSTR lpPassword,
			 DWORD dwLogonFlags,
			 LPCWSTR lpApplicationName,
			 LPWSTR lpCommandLine,
			 DWORD dwCreationFlags,
			 LPVOID lpEnvironment,
			 LPCWSTR lpCurrentDirectory,
			 LPSTARTUPINFOW lpStartupInfo,
			 LPPROCESS_INFORMATION lpProcessInformation)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserA (LPCSTR lpszUsername,
	    LPCSTR lpszDomain,
	    LPCSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserW (LPCWSTR lpszUsername,
	    LPCWSTR lpszDomain,
	    LPCWSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_QUALITY_OF_SERVICE Qos;
  TOKEN_USER TokenUser;
  TOKEN_OWNER TokenOwner;
  TOKEN_PRIMARY_GROUP TokenPrimaryGroup;
  TOKEN_GROUPS TokenGroups;
//  PTOKEN_GROUPS TokenGroups;
  TOKEN_PRIVILEGES TokenPrivileges;
//  PTOKEN_PRIVILEGES TokenPrivileges;
  TOKEN_DEFAULT_DACL TokenDefaultDacl;
  LARGE_INTEGER ExpirationTime;
  LUID AuthenticationId;
  TOKEN_SOURCE TokenSource;
  PSID UserSid;
  ACL Dacl;
  NTSTATUS Status;

  SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};

  Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  Qos.ImpersonationLevel = SecurityAnonymous;
  Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
  Qos.EffectiveOnly = FALSE;

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = &Qos;

//  AuthenticationId = SYSTEM_LUID;
  AuthenticationId.LowPart = 1000;
  AuthenticationId.HighPart = 0;
  ExpirationTime.QuadPart = -1;

  /* FIXME: Get the user SID from the registry */
  RtlAllocateAndInitializeSid (&SystemAuthority,
			       5,
			       SECURITY_NT_NON_UNIQUE_RID,
			       0x12345678,
			       0x12345678,
			       0x12345678,
			       DOMAIN_USER_RID_ADMIN,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       &UserSid);

  TokenUser.User.Sid = UserSid;
  TokenUser.User.Attributes = 0;

//  TokenGroups = NULL;
  TokenGroups.GroupCount = 1;
  TokenGroups.Groups[0].Sid = UserSid; /* FIXME */
  TokenGroups.Groups[0].Attributes = SE_GROUP_ENABLED;

//  TokenPrivileges = NULL;
  TokenPrivileges.PrivilegeCount = 0;

  TokenOwner.Owner = UserSid;

  TokenPrimaryGroup.PrimaryGroup = UserSid;

  Status = RtlCreateAcl (&Dacl, sizeof(ACL), ACL_REVISION);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }

  TokenDefaultDacl.DefaultDacl = &Dacl;

  memcpy (TokenSource.SourceName,
	  "**ANON**",
	  8);
  Status = NtAllocateLocallyUniqueId (&TokenSource.SourceIdentifier);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeSid (UserSid);
      return FALSE;
    }

  Status = NtCreateToken (phToken,
			  TOKEN_ALL_ACCESS,
			  &ObjectAttributes,
			  TokenPrimary,
			  &AuthenticationId,
			  &ExpirationTime,
			  &TokenUser,
//			  TokenGroups,
			  &TokenGroups,
//			  TokenPrivileges,
			  &TokenPrivileges,
			  &TokenOwner,
			  &TokenPrimaryGroup,
			  &TokenDefaultDacl,
			  &TokenSource);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeSid (UserSid);
      return FALSE;
    }

  RtlFreeSid (UserSid);

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserExA (LPCSTR lpszUsername,
	      LPCSTR lpszDomain,
	      LPCSTR lpszPassword,
	      DWORD dwLogonType,
	      DWORD dwLogonProvider,
	      PHANDLE phToken,
	      PSID *ppLogonSid,
	      PVOID *ppProfileBuffer,
	      LPDWORD pdwProfileLength,
	      PQUOTA_LIMITS pQuotaLimits)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserExW (LPCWSTR lpszUsername,
	      LPCWSTR lpszDomain,
	      LPCWSTR lpszPassword,
	      DWORD dwLogonType,
	      DWORD dwLogonProvider,
	      PHANDLE phToken,
	      PSID *ppLogonSid,
	      PVOID *ppProfileBuffer,
	      LPDWORD pdwProfileLength,
	      PQUOTA_LIMITS pQuotaLimits)
{
  return FALSE;
}

/* EOF */
