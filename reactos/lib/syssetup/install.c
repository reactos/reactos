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
/* $Id: install.c,v 1.7 2004/01/23 10:34:23 ekohl Exp $
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

#include <samlib.h>
#include <syssetup.h>

// #define NO_GUI

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
  char Buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

#ifdef NO_GUI
  OutputDebugStringA(buffer);
#else
  strcat (buffer, "\nRebooting now!");
  MessageBoxA (NULL,
	       Buffer,
	       "ReactOS Setup",
	       MB_OK);
#endif
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
      DebugPrint ("InitializeProfiles() failed\n");
      return 0;
    }

  /* Create the semi-random Domain-SID */
  CreateRandomSid (&DomainSid);
  if (DomainSid == NULL)
    {
      DebugPrint ("Domain-SID creation failed!\n");
      return 0;
    }

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Domain-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Initialize the Security Account Manager (SAM) */
  if (!SamInitializeSAM ())
    {
      DebugPrint ("SamInitializeSAM() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Set the Domain SID (aka Computer SID) */
  if (!SamSetDomainSid (DomainSid))
    {
      DebugPrint ("SamSetDomainSid() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Append the Admin-RID */
  AppendRidToSid (&AdminSid, DomainSid, DOMAIN_USER_RID_ADMIN);

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Admin-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Create the Administrator account */
  if (!SamCreateUser (L"Administrator", L"", AdminSid))
    {
      DebugPrint ("SamCreateUser() failed!\n");
      RtlFreeSid (AdminSid);
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Create the Administrator profile */
  if (!CreateUserProfileW (AdminSid, L"Administrator"))
    {
      DebugPrint ("CreateUserProfileW() failed!\n");
      RtlFreeSid (AdminSid);
      RtlFreeSid (DomainSid);
      return 0;
    }

  RtlFreeSid (AdminSid);
  RtlFreeSid (DomainSid);

  DebugPrint ("System setup successful\n");

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
