/* $Id: environment.c,v 1.1 2004/03/19 12:40:49 ekohl Exp $
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

  RtlInitUnicodeString (&Name, lpName);
  RtlInitUnicodeString (&Value, lpValue);

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


BOOL WINAPI
CreateEnvironmentBlock (LPVOID *lpEnvironment,
			HANDLE hToken,
			BOOL bInherit)
{
  WCHAR Buffer[MAX_PATH];
  DWORD Length;
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
