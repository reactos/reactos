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
/* $Id: install.c,v 1.3 2003/09/08 09:56:57 weiden Exp $
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

/* FUNCTIONS ****************************************************************/

DWORD STDCALL
InstallReactOS (HINSTANCE hInstance)
{
  OutputDebugStringA ("InstallReactOS() called\n");

  if (!InitializeSetupActionLog (FALSE))
    {
      OutputDebugStringA ("InitializeSetupActionLog() failed\n");
    }

  LogItem (SEVERITY_INFORMATION,
	   L"ReactOS Setup starting");

  LogItem (SEVERITY_FATAL_ERROR,
	   L"Buuuuuuaaaah!");

  LogItem (SEVERITY_INFORMATION,
	   L"ReactOS Setup finished");

  TerminateSetupActionLog ();

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
