/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: install.c,v 1.6 2004/01/16 21:33:23 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/install.c
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <stdio.h>

#include <syssetup.h>

#if 0
VOID Wizard (VOID);
#endif

/* userenv.dll */
BOOL WINAPI InitializeProfiles (VOID);
BOOL WINAPI CreateUserProfileW (PSID Sid, LPCWSTR lpUserName);


/* GLOBALS ******************************************************************/

PSID DomainSid = NULL;
PSID AdminSid = NULL;


/* FUNCTIONS ****************************************************************/

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


static VOID
CreateRandomSid (PSID *Sid)
{
  SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
  LARGE_INTEGER SystemTime;
  PULONG Seed;

  NtQuerySystemTime (&SystemTime);
  Seed = &SystemTime.u.LowPart;

  RtlAllocateAndInitializeSid (&SystemAuthority,
			       4,
			       SECURITY_NT_NON_UNIQUE_RID,
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       Sid);
}

static VOID
AppendRidToSid (PSID *Dst,
		PSID Src,
		ULONG NewRid)
{
  ULONG Rid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  UCHAR RidCount;
  ULONG i;

  RidCount = *RtlSubAuthorityCountSid (Src);

  for (i = 0; i < RidCount; i++)
    Rid[i] = *RtlSubAuthoritySid (Src, i);

  if (RidCount < 8)
    {
      Rid[RidCount] = NewRid;
      RidCount++;
    }

  RtlAllocateAndInitializeSid (RtlIdentifierAuthoritySid (Src),
			       RidCount,
			       Rid[0],
			       Rid[1],
			       Rid[2],
			       Rid[3],
			       Rid[4],
			       Rid[5],
			       Rid[6],
			       Rid[7],
			       Dst);
}

static BOOL
SaveDomainSid (PSID Sid)
{
  DWORD dwDisposition;
  HKEY hKey;

  if (RegCreateKeyExW (HKEY_LOCAL_MACHINE,
		       L"SAM\\Accounts",
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hKey,
		       &dwDisposition))
    {
#if 0
      DebugPrint ("Failed to create Accounts key! (Error %lu)\n", GetLastError());
#endif
      return FALSE;
    }

  if (RegSetValueExW (hKey,
		      L"DomainSID",
		      0,
		      REG_BINARY,
		      (LPBYTE)Sid,
		      RtlLengthSid (Sid)))
    {
#if 0
      DebugPrint ("Failed to set DomainSID value! (Error %lu)\n", GetLastError());
#endif
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  return TRUE;
}


DWORD STDCALL
InstallReactOS (HINSTANCE hInstance)
{
# if 0
  OutputDebugStringA ("InstallReactOS() called\n");

  if (!InitializeSetupActionLog (FALSE))
    {
      OutputDebugStringA ("InitializeSetupActionLog() failed\n");
    }

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup starting");

  LogItem (SYSSETUP_SEVERITY_FATAL_ERROR,
	   L"Buuuuuuaaaah!");

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup finished");

  TerminateSetupActionLog ();
#endif
#if 0
  UNICODE_STRING SidString;
#endif

  if (!InitializeProfiles ())
    {
#if 0
      OutputDebugStringA ("InitializeProfiles() failed\n");
#endif
      MessageBoxA (NULL,
		   "Profile initialization failed!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);
      return 0;
    }

  CreateRandomSid (&DomainSid);
  if (DomainSid == NULL)
    {
#if 0
      OutputDebugStringA ("Failed to create the Domain-SID!\n");
#endif
      MessageBoxA (NULL,
		   "Failed to create the Domain SID!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);

      return 0;
    }

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Domain-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  if (!SaveDomainSid (DomainSid))
    {
#if 0
      OutputDebugStringA ("Failed to create the Domain-SID!\n");
#endif
      MessageBoxA (NULL,
		   "Failed to save the Domain SID!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);

      RtlFreeSid (DomainSid);
      return 0;
    }

  AppendRidToSid (&AdminSid, DomainSid, DOMAIN_USER_RID_ADMIN);

#if 0
  RtlConvertSidToUnicodeString (&SidString, AdminSid, TRUE);
  DebugPrint ("Admin SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  if (!CreateUserProfileW (AdminSid, L"Administrator"))
    {
#if 0
      DebugPrint ("Failed to create the admin user profile!\n");
#endif
      MessageBoxA (NULL,
		   "Failed to create the admin user profile!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);

      RtlFreeSid (AdminSid);
      RtlFreeSid (DomainSid);
      return 0;
    }

  RtlFreeSid (AdminSid);
  RtlFreeSid (DomainSid);

#if 0
  OutputDebugStringA ("System setup successful\n");
#endif
  MessageBoxA (NULL,
	       "Profile initialization successful!\nRebooting now!",
	       "ReactOS Setup",
	       MB_OK);

#if 0
  Wizard ();
#endif

  return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL SetupChangeFontSize(HANDLE HWindow,
                                  LPCWSTR lpszFontSize)
{
  return(FALSE);
}
