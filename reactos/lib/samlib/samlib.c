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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: samlib.c,v 1.2 2004/02/04 17:57:56 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/samlib.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <string.h>

#include <samlib.h>


#define NDEBUG
#include "debug.h"


/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/


static BOOL
CreateBuiltinAliases (HKEY hAliasesKey)
{
  return TRUE;
}


static BOOL
CreateBuiltinGroups (HKEY hGroupsKey)
{
  return TRUE;
}


static BOOL
CreateBuiltinUsers (HKEY hUsersKey)
{
  return TRUE;
}


BOOL STDCALL
SamInitializeSAM (VOID)
{
  DWORD dwDisposition;
  HKEY hSamKey;
  HKEY hDomainsKey;
  HKEY hAccountKey;
  HKEY hBuiltinKey;
  HKEY hAliasesKey;
  HKEY hGroupsKey;
  HKEY hUsersKey;

  DPRINT1("SamInitializeSAM() called\n");

  if (RegCreateKeyExW (HKEY_LOCAL_MACHINE,
		       L"SAM\\SAM",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hSamKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Sam' key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  if (RegCreateKeyExW (hSamKey,
		       L"Domains",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hDomainsKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Domains' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hSamKey);
      return FALSE;
    }

  RegCloseKey (hSamKey);

  /* Create the 'Domains\\Account' key */
  if (RegCreateKeyExW (hDomainsKey,
		       L"Account",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hAccountKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Domains\\Account' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hDomainsKey);
      return FALSE;
    }


  /* Create the 'Account\Aliases' key */
  if (RegCreateKeyExW (hAccountKey,
		       L"Aliases",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hAliasesKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Account\\Aliases' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hAccountKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hAliasesKey);


  /* Create the 'Account\Groups' key */
  if (RegCreateKeyExW (hAccountKey,
		       L"Groups",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hGroupsKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Account\\Groups' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hAccountKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hGroupsKey);


  /* Create the 'Account\Users' key */
  if (RegCreateKeyExW (hAccountKey,
		       L"Users",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hUsersKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Account\\Users' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hAccountKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  RegCloseKey (hAccountKey);


  /* Create the 'Domains\\Builtin' */
  if (RegCreateKeyExW (hDomainsKey,
		       L"Builtin",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hBuiltinKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create Builtin key! (Error %lu)\n", GetLastError());
      RegCloseKey (hDomainsKey);
      return FALSE;
    }


  /* Create the 'Builtin\Aliases' key */
  if (RegCreateKeyExW (hBuiltinKey,
		       L"Aliases",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hAliasesKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Builtin\\Aliases' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* Create builtin aliases */
  if (!CreateBuiltinAliases (hAliasesKey))
    {
      DPRINT1 ("Failed to create builtin aliases!\n");
      RegCloseKey (hAliasesKey);
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hAliasesKey);


  /* Create the 'Builtin\Groups' key */
  if (RegCreateKeyExW (hBuiltinKey,
		       L"Groups",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hGroupsKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Builtin\\Groups' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* Create builtin groups */
  if (!CreateBuiltinGroups (hGroupsKey))
    {
      DPRINT1 ("Failed to create builtin groups!\n");
      RegCloseKey (hGroupsKey);
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hGroupsKey);


  /* Create the 'Builtin\Users' key */
  if (RegCreateKeyExW (hBuiltinKey,
		       L"Users",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hUsersKey,
		       &dwDisposition))
    {
      DPRINT1 ("Failed to create 'Builtin\\Users' key! (Error %lu)\n", GetLastError());
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* Create builtin users */
  if (!CreateBuiltinUsers (hUsersKey))
    {
      DPRINT1 ("Failed to create builtin users!\n");
      RegCloseKey (hUsersKey);
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  RegCloseKey (hBuiltinKey);

  RegCloseKey (hDomainsKey);

  DPRINT1 ("SamInitializeSAM() done\n");

  return TRUE;
}


BOOL STDCALL
SamGetDomainSid (PSID *Sid)
{
  DPRINT1 ("SamGetDomainSid() called\n");

  return FALSE;
}


BOOL STDCALL
SamSetDomainSid (PSID Sid)
{
  HKEY hAccountKey;

  DPRINT1 ("SamSetDomainSid() called\n");

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account",
		     0,
		     KEY_ALL_ACCESS,
		     &hAccountKey))
    {
      DPRINT1 ("Failed to open the Account key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  if (RegSetValueExW (hAccountKey,
		      L"Sid",
		      0,
		      REG_BINARY,
		      (LPBYTE)Sid,
		      RtlLengthSid (Sid)))
    {
      DPRINT1 ("Failed to set Domain-SID value! (Error %lu)\n", GetLastError());
      RegCloseKey (hAccountKey);
      return FALSE;
    }

  RegCloseKey (hAccountKey);

  DPRINT1 ("SamSetDomainSid() called\n");

  return TRUE;
}


/*
 * ERROR_USER_EXISTS
 */
BOOL STDCALL
SamCreateUser (PWSTR UserName,
	       PWSTR UserPassword,
	       PSID UserSid)
{
  DWORD dwDisposition;
  HKEY hUsersKey;
  HKEY hUserKey;

  DPRINT1 ("SamCreateUser() called\n");

  /* FIXME: Check whether the SID is a real user sid */

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_ALL_ACCESS,
		     &hUsersKey))
    {
      DPRINT1 ("Failed to open Account key! (Error %lu)\n", GetLastError());
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
      DPRINT1 ("Failed to create/open the user key! (Error %lu)\n", GetLastError());
      RegCloseKey (hUsersKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  if (dwDisposition == REG_OPENED_EXISTING_KEY)
    {
      DPRINT1 ("User alredy exists!\n");
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
      DPRINT1 ("Failed to set the user name value! (Error %lu)\n", GetLastError());
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
      DPRINT1 ("Failed to set the user name value! (Error %lu)\n", GetLastError());
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
      DPRINT1 ("Failed to set the user SID value! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  DPRINT1 ("SamCreateUser() done\n");

  return TRUE;
}


/*
 * ERROR_WRONG_PASSWORD
 * ERROR_NO_SUCH_USER
 */
BOOL STDCALL
SamCheckUserPassword (PWSTR UserName,
		      PWSTR UserPassword)
{
  WCHAR szPassword[256];
  DWORD dwLength;
  HKEY hUsersKey;
  HKEY hUserKey;

  DPRINT1 ("SamCheckUserPassword() called\n");

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_READ,
		     &hUsersKey))
    {
      DPRINT1 ("Failed to open Users key! (Error %lu)\n", GetLastError());
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
	  DPRINT1 ("Invalid user name!\n");
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
	  DPRINT1 ("Failed to open user key! (Error %lu)\n", GetLastError());
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
      DPRINT1 ("Failed to read the password! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  /* Compare passwords */
  if ((wcslen (szPassword) != wcslen (UserPassword)) ||
      (wcscmp (szPassword, UserPassword) != 0))
    {
      DPRINT1 ("Wrong password!\n");
      SetLastError (ERROR_WRONG_PASSWORD);
      return FALSE;
    }

  DPRINT1 ("SamCheckUserPassword() done\n");

  return TRUE;
}


BOOL STDCALL
SamGetUserSid (PWSTR UserName,
	       PSID *Sid)
{
  PSID lpSid;
  DWORD dwLength;
  HKEY hUsersKey;
  HKEY hUserKey;

  DPRINT1 ("SamGetUserSid() called\n");

  if (Sid != NULL)
    *Sid = NULL;

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_READ,
		     &hUsersKey))
    {
      DPRINT1 ("Failed to open Users key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hUserKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
	  DPRINT1 ("Invalid user name!\n");
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
	  DPRINT1 ("Failed to open user key! (Error %lu)\n", GetLastError());
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
      DPRINT1 ("Failed to read the SID size! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Allocate sid buffer */
  DPRINT1 ("Required SID buffer size: %lu\n", dwLength);
  lpSid = (PSID)RtlAllocateHeap (RtlGetProcessHeap (),
				 0,
				 dwLength);
  if (lpSid == NULL)
    {
      DPRINT1 ("Failed to allocate SID buffer!\n");
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
      DPRINT1 ("Failed to read the SID! (Error %lu)\n", GetLastError());
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   lpSid);
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  *Sid = lpSid;

  DPRINT1 ("SamGetUserSid() done\n");

  return TRUE;
}

/* EOF */
