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
/* $Id: samlib.c,v 1.1 2004/01/23 10:33:21 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/samlib.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include <samlib.h>


/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

void
DebugPrint(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  OutputDebugStringA(buffer);
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
//#if 0
      DebugPrint ("Failed to create 'Sam' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Domains' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Domains\\Account' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Account\\Aliases' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Account\\Groups' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Account\\Users' key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create Builtin key! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to create 'Builtin\\Aliases' key! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* FIXME: Create builtin aliases */

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
//#if 0
      DebugPrint ("Failed to create 'Builtin\\Groups' key! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* FIXME: Create builtin groups */

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
//#if 0
      DebugPrint ("Failed to create 'Builtin\\Users' key! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hBuiltinKey);
      RegCloseKey (hDomainsKey);
      return FALSE;
    }

  /* FIXME: Create builtin users */

  RegCloseKey (hUsersKey);

  RegCloseKey (hBuiltinKey);

  RegCloseKey (hDomainsKey);

  return TRUE;
}


BOOL STDCALL
SamGetDomainSid (PSID *Sid)
{
  return FALSE;
}


BOOL STDCALL
SamSetDomainSid (PSID Sid)
{
  HKEY hAccountKey;

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account",
		     0,
		     KEY_ALL_ACCESS,
		     &hAccountKey))
    {
//#if 0
      DebugPrint ("Failed to open the Account key! (Error %lu)\n", GetLastError());
//#endif
      return FALSE;
    }

  if (RegSetValueExW (hAccountKey,
		      L"Sid",
		      0,
		      REG_BINARY,
		      (LPBYTE)Sid,
		      RtlLengthSid (Sid)))
    {
//#if 0
      DebugPrint ("Failed to set Domain-SID value! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hAccountKey);
      return FALSE;
    }

  RegCloseKey (hAccountKey);

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
  HKEY hAccountKey;
  HKEY hUserKey;

  DebugPrint ("SamCreateUser() called\n");

  /* FIXME: Check whether the SID is a real user sid */

  /* Open the Account key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account",
		     0,
		     KEY_ALL_ACCESS,
		     &hAccountKey))
    {
//#if 0
      DebugPrint ("Failed to open Account key! (Error %lu)\n", GetLastError());
//#endif
      return FALSE;
    }

  /* Create user name key */
  if (RegCreateKeyExW (hAccountKey,
		       UserName,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hUserKey,
		       &dwDisposition))
    {
//#if 0
      DebugPrint ("Failed to create/open the user key! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hAccountKey);
      return FALSE;
    }

  RegCloseKey (hAccountKey);

  if (dwDisposition == REG_OPENED_EXISTING_KEY)
    {
//#if 0
      DebugPrint ("User alredy exists!\n");
//#endif
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
//#if 0
      DebugPrint ("Failed to set the user name value! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to set the user name value! (Error %lu)\n", GetLastError());
//#endif
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
//#if 0
      DebugPrint ("Failed to set the user SID value! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

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
  HKEY hAccountKey;
  HKEY hUserKey;

  /* Open the Account key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account",
		     0,
		     KEY_READ,
		     &hAccountKey))
    {
//#if 0
      DebugPrint ("Failed to open Account key! (Error %lu)\n", GetLastError());
//#endif
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hAccountKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
//#if 0
	  DebugPrint ("Invalid user name!\n");
//#endif
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
//#if 0
	  DebugPrint ("Failed to open user key! (Error %lu)\n", GetLastError());
//#endif
	}

      RegCloseKey (hAccountKey);
      return FALSE;
    }

  RegCloseKey (hAccountKey);

  /* Get profiles path */
  dwLength = 256 * sizeof(WCHAR);
  if (RegQueryValueExW (hUserKey,
			L"Password",
			NULL,
			NULL,
			(LPBYTE)szPassword,
			&dwLength))
    {
//#if 0
      DebugPrint ("Failed to read the password! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  /* Compare passwords */
  if ((wcslen (szPassword) != wcslen (UserPassword)) ||
      (wcscmp (szPassword, UserPassword) != 0))
    {
      SetLastError (ERROR_WRONG_PASSWORD);
      return FALSE;
    }

  return TRUE;
}


BOOL STDCALL
SamGetUserSid (PWSTR UserName,
	       PSID *Sid)
{
  PSID lpSid;
  DWORD dwLength;
  HKEY hAccountKey;
  HKEY hUserKey;

  if (Sid != NULL)
    *Sid = NULL;

  /* Open the Account key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account",
		     0,
		     KEY_READ,
		     &hAccountKey))
    {
//#if 0
      DebugPrint ("Failed to open Account key! (Error %lu)\n", GetLastError());
//#endif
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hAccountKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
//#if 0
	  DebugPrint ("Invalid user name!\n");
//#endif
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
//#if 0
	  DebugPrint ("Failed to open user key! (Error %lu)\n", GetLastError());
//#endif
	}

      RegCloseKey (hAccountKey);
      return FALSE;
    }

  RegCloseKey (hAccountKey);

  /* Get SID size */
  dwLength = 0;
  if (RegQueryValueExW (hUserKey,
			L"Sid",
			NULL,
			NULL,
			NULL,
			&dwLength))
    {
//#if 0
      DebugPrint ("Failed to read the SID size! (Error %lu)\n", GetLastError());
//#endif
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* FIXME: Allocate sid buffer */
//#if 0
  DebugPrint ("Required SID buffer size: %lu\n", dwLength);
//#endif

  lpSid = (PSID)RtlAllocateHeap (RtlGetProcessHeap (),
				 0,
				 dwLength);
  if (lpSid == NULL)
    {
//#if 0
      DebugPrint ("Failed to allocate SID buffer!\n");
//#endif
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
//#if 0
      DebugPrint ("Failed to read the SID! (Error %lu)\n", GetLastError());
//#endif
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   lpSid);
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  *Sid = lpSid;

  return TRUE;
}

/* EOF */
