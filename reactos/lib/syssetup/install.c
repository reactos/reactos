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
/* $Id: install.c,v 1.5 2004/01/14 22:15:09 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/install.c
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#include <syssetup.h>

#if 0
VOID Wizard (VOID);
#endif

/* userenv.dll */
BOOL WINAPI InitializeProfiles (VOID);

/* FUNCTIONS ****************************************************************/

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


  if (InitializeProfiles())
    {
      MessageBoxA (NULL,
		   "Profiles initialized!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);
    }
  else
    {
      MessageBoxA (NULL,
		   "Profile initialization failed!\nRebooting now!",
		   "ReactOS Setup",
		   MB_OK);
    }

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
