/* $Id: environment.c,v 1.2 2004/03/24 16:00:01 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/environment.c
 * PURPOSE:         User environment functions
 * PROGRAMMER:      Eric Kohl
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <userenv.h>

#include "internal.h"


static BOOL
SetUserEnvironmentVariable (LPVOID *Environment,
			    LPWSTR lpName,
			    LPWSTR lpValue)
{
  UNICODE_STRING Name;
  UNICODE_STRING Value;
  NTSTATUS Status;

  RtlInitUnicodeString (&Name,
			lpName);
  RtlInitUnicodeString (&Value,
			lpValue);

  Status = RtlSetEnvironmentVariable ((PWSTR*)Environment,
				      &Name,
				      &Value);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("RtlSetEnvironmentVariable() failed (Status %lx)\n", Status);
      return FALSE;
    }

  return TRUE;
}


static BOOL
AppendUserEnvironmentVariable (LPVOID *Environment,
			       LPWSTR lpName,
			       LPWSTR lpValue)
{
  UNICODE_STRING Name;
  UNICODE_STRING Value;
  NTSTATUS Status;

  RtlInitUnicodeString (&Name,
			lpName);

  Value.Length = 0;
  Value.MaximumLength = 1024 * sizeof(WCHAR);
  Value.Buffer = LocalAlloc (LPTR,
			     1024 * sizeof(WCHAR));
  if (Value.Buffer == NULL)
    {
      return FALSE;
    }
  Value.Buffer[0] = UNICODE_NULL;

  Status = RtlQueryEnvironmentVariable_U ((PWSTR)*Environment,
					  &Name,
					  &Value);
  if (NT_SUCCESS(Status))
    {
      RtlAppendUnicodeToString (&Value,
				L";");
    }

  RtlAppendUnicodeToString (&Value,
			    lpValue);

  Status = RtlSetEnvironmentVariable ((PWSTR*)Environment,
				      &Name,
				      &Value);
  LocalFree (Value.Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("RtlSetEnvironmentVariable() failed (Status %lx)\n", Status);
      return FALSE;
    }

  return TRUE;
}


static HKEY
GetCurrentUserKey (HANDLE hToken)
{
  UNICODE_STRING SidString;
  HKEY hKey;

  if (!GetUserSidFromToken (hToken,
			    &SidString))
    {
      DPRINT1 ("GetUserSidFromToken() failed\n");
      return NULL;
    }

  if (RegOpenKeyExW (HKEY_USERS,
		     SidString.Buffer,
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1 ("RegOpenKeyExW() failed (Error %ld)\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return NULL;
    }

  RtlFreeUnicodeString (&SidString);

  return hKey;
}


static BOOL
SetUserEnvironment (LPVOID *lpEnvironment,
		    HKEY hKey,
		    LPWSTR lpSubKeyName)
{
  HKEY hEnvKey;
  DWORD dwValues;
  DWORD dwMaxValueNameLength;
  DWORD dwMaxValueDataLength;
  DWORD dwValueNameLength;
  DWORD dwValueDataLength;
  DWORD dwType;
  DWORD i;
  LPWSTR lpValueName;
  LPWSTR lpValueData;

  if (RegOpenKeyExW (hKey,
		     lpSubKeyName,
		     0,
		     KEY_ALL_ACCESS,
		     &hEnvKey))
    {
      DPRINT1 ("RegOpenKeyExW() failed (Error %ld)\n", GetLastError());
      return FALSE;
    }

  if (RegQueryInfoKey (hEnvKey,
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       &dwValues,
		       &dwMaxValueNameLength,
		       &dwMaxValueDataLength,
		       NULL,
		       NULL))
    {
      DPRINT1 ("RegQueryInforKey() failed (Error %ld)\n", GetLastError());
      RegCloseKey (hEnvKey);
      return FALSE;
    }

  if (dwValues == 0)
    {
      RegCloseKey (hEnvKey);
      return TRUE;
    }

  /* Allocate buffers */
  lpValueName = LocalAlloc (LPTR,
			    dwMaxValueNameLength * sizeof(WCHAR));
  if (lpValueName == NULL)
    {
      RegCloseKey (hEnvKey);
      return FALSE;
    }

  lpValueData = LocalAlloc (LPTR,
			    dwMaxValueDataLength);
  if (lpValueData == NULL)
    {
      LocalFree (lpValueName);
      RegCloseKey (hEnvKey);
      return FALSE;
    }

  /* Enumerate values */
  for (i = 0; i < dwValues; i++)
    {
      dwValueNameLength = dwMaxValueNameLength;
      dwValueDataLength = dwMaxValueDataLength;
      RegEnumValueW (hEnvKey,
		     i,
		     lpValueName,
		     &dwValueNameLength,
		     NULL,
		     &dwType,
		     (LPBYTE)lpValueData,
		     &dwValueDataLength);

      if (!_wcsicmp (lpValueName, L"path"))
	{
	  /* Append 'Path' environment variable */
	  AppendUserEnvironmentVariable (lpEnvironment,
					 lpValueName,
					 lpValueData);
	}
       else
	{
	  /* Set environment variable */
	  SetUserEnvironmentVariable (lpEnvironment,
				      lpValueName,
				      lpValueData);
	}
    }

  LocalFree (lpValueData);
  LocalFree (lpValueName);
  RegCloseKey (hEnvKey);

  return TRUE;
}


BOOL WINAPI
CreateEnvironmentBlock (LPVOID *lpEnvironment,
			HANDLE hToken,
			BOOL bInherit)
{
  WCHAR Buffer[MAX_PATH];
  DWORD Length;
  HKEY hKeyUser;
  NTSTATUS Status;

  DPRINT1 ("CreateEnvironmentBlock() called\n");

  if (lpEnvironment == NULL)
    return FALSE;

  Status = RtlCreateEnvironment ((BOOLEAN)bInherit,
				 (PWSTR*)lpEnvironment);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("RtlCreateEnvironment() failed (Status %lx)\n", Status);
      return FALSE;
    }

  /* Set 'COMPUTERNAME' variable */
  Length = MAX_PATH;
  if (GetComputerNameW (Buffer,
			&Length))
    {
      SetUserEnvironmentVariable (lpEnvironment,
				  L"COMPUTERNAME",
				  Buffer);
    }

  if (hToken == NULL)
    return TRUE;

  hKeyUser = GetCurrentUserKey (hToken);
  if (hKeyUser == NULL)
    {
      DPRINT1 ("GetCurrentUserKey() failed\n");
      RtlDestroyEnvironment (*lpEnvironment);
      return FALSE;
    }

  /* Set 'USERPROFILE' variable */
  Length = MAX_PATH;
  if (GetUserProfileDirectoryW (hToken,
				Buffer,
				&Length))
    {
      SetUserEnvironmentVariable (lpEnvironment,
				  L"USERPROFILE",
				  Buffer);
    }

  /* FIXME: Set 'USERDOMAIN' variable */

  /* FIXME: Set 'USERNAME' variable */

  /* Set user environment variables */
  SetUserEnvironment (lpEnvironment,
		      hKeyUser,
		      L"Environment");

  RegCloseKey (hKeyUser);

  return TRUE;
}


BOOL WINAPI
DestroyEnvironmentBlock (LPVOID lpEnvironment)
{
  DPRINT ("DestroyEnvironmentBlock() called\n");

  if (lpEnvironment == NULL)
    return FALSE;

  RtlDestroyEnvironment (lpEnvironment);

  return TRUE;
}

/* EOF */
