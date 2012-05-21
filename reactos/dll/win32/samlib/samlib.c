/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/samlib.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(samlib);

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

/*
 * ERROR_USER_EXISTS
 */
BOOL WINAPI
SamCreateUser (PWSTR UserName,
	       PWSTR UserPassword,
	       PSID UserSid)
{
  DWORD dwDisposition;
  HKEY hUsersKey;
  HKEY hUserKey;

  TRACE("SamCreateUser() called\n");

  /* FIXME: Check whether the SID is a real user sid */

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_ALL_ACCESS,
		     &hUsersKey))
    {
      ERR("Failed to open Account key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  /* Create user name key */
  if (RegCreateKeyExW (hUsersKey,
		       UserName,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hUserKey,
		       &dwDisposition))
    {
      ERR("Failed to create/open the user key! (Error %lu)\n", GetLastError());
      RegCloseKey (hUsersKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  if (dwDisposition == REG_OPENED_EXISTING_KEY)
    {
      ERR("User already exists!\n");
      RegCloseKey (hUserKey);
      SetLastError (ERROR_USER_EXISTS);
      return FALSE;
    }


  /* Set 'Name' value */
  if (RegSetValueExW (hUserKey,
		      L"Name",
		      0,
		      REG_SZ,
		      (LPBYTE)UserName,
		      (wcslen (UserName) + 1) * sizeof (WCHAR)))
    {
      ERR("Failed to set the user name value! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Set 'Password' value */
  if (RegSetValueExW (hUserKey,
		      L"Password",
		      0,
		      REG_SZ,
		      (LPBYTE)UserPassword,
		      (wcslen (UserPassword) + 1) * sizeof (WCHAR)))
    {
      ERR("Failed to set the user name value! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Set 'Sid' value */
  if (RegSetValueExW (hUserKey,
		      L"Sid",
		      0,
		      REG_BINARY,
		      (LPBYTE)UserSid,
		      RtlLengthSid (UserSid)))
    {
      ERR("Failed to set the user SID value! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  TRACE("SamCreateUser() done\n");

  return TRUE;
}


/*
 * ERROR_WRONG_PASSWORD
 * ERROR_NO_SUCH_USER
 */
BOOL WINAPI
SamCheckUserPassword (PWSTR UserName,
		      PWSTR UserPassword)
{
  WCHAR szPassword[256];
  DWORD dwLength;
  HKEY hUsersKey;
  HKEY hUserKey;

  TRACE("SamCheckUserPassword() called\n");

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_READ,
		     &hUsersKey))
    {
      ERR("Failed to open Users key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hUsersKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
	  ERR("Invalid user name!\n");
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
	  ERR("Failed to open user key! (Error %lu)\n", GetLastError());
	}

      RegCloseKey (hUsersKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  /* Get the password */
  dwLength = 256 * sizeof(WCHAR);
  if (RegQueryValueExW (hUserKey,
			L"Password",
			NULL,
			NULL,
			(LPBYTE)szPassword,
			&dwLength))
    {
      ERR("Failed to read the password! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  /* Compare passwords */
  if ((wcslen (szPassword) != wcslen (UserPassword)) ||
      (wcscmp (szPassword, UserPassword) != 0))
    {
      ERR("Wrong password!\n");
      SetLastError (ERROR_WRONG_PASSWORD);
      return FALSE;
    }

  TRACE("SamCheckUserPassword() done\n");

  return TRUE;
}


BOOL WINAPI
SamGetUserSid (PWSTR UserName,
	       PSID *Sid)
{
  PSID lpSid;
  DWORD dwLength;
  HKEY hUsersKey;
  HKEY hUserKey;

  TRACE("SamGetUserSid() called\n");

  if (Sid != NULL)
    *Sid = NULL;

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_READ,
		     &hUsersKey))
    {
      ERR("Failed to open Users key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hUsersKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
	  ERR("Invalid user name!\n");
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
	  ERR("Failed to open user key! (Error %lu)\n", GetLastError());
	}

      RegCloseKey (hUsersKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  /* Get SID size */
  dwLength = 0;
  if (RegQueryValueExW (hUserKey,
			L"Sid",
			NULL,
			NULL,
			NULL,
			&dwLength))
    {
      ERR("Failed to read the SID size! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Allocate sid buffer */
  TRACE("Required SID buffer size: %lu\n", dwLength);
  lpSid = (PSID)RtlAllocateHeap (RtlGetProcessHeap (),
				 0,
				 dwLength);
  if (lpSid == NULL)
    {
      ERR("Failed to allocate SID buffer!\n");
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Read sid */
  if (RegQueryValueExW (hUserKey,
			L"Sid",
			NULL,
			NULL,
			(LPBYTE)lpSid,
			&dwLength))
    {
      ERR("Failed to read the SID! (Error %lu)\n", GetLastError());
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   lpSid);
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  *Sid = lpSid;

  TRACE("SamGetUserSid() done\n");

  return TRUE;
}

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
PSAMPR_SERVER_NAME_bind(PSAMPR_SERVER_NAME pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\samr",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
PSAMPR_SERVER_NAME_unbind(PSAMPR_SERVER_NAME pszSystemName,
                          handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NTSTATUS
NTAPI
SamCloseHandle(IN SAM_HANDLE SamHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SamConnect(IN OUT PUNICODE_STRING ServerName,
           OUT PSAM_HANDLE ServerHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
