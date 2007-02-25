/*
 *  ReactOS applications
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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        subsys/system/reactos/reactos.c
 * PROGRAMMERS: Eric Kohl
 */

#include <windows.h>
#include <tchar.h>

#include "resource.h"


/* GLOBALS ******************************************************************/

TCHAR szCaption[256];
TCHAR szText[256];

HINSTANCE hInstance;


/* FUNCTIONS ****************************************************************/

int WINAPI
WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  hInstance = hInst;

  if (!LoadString(hInstance,
                  IDS_CAPTION,
                  szCaption,
                  (sizeof szCaption / sizeof szCaption[0])))
    return 0;

  if (!LoadString(hInstance,
                  IDS_TEXT,
                  szText,
                  (sizeof szText / sizeof szText[0])))
    return 0;

  MessageBox(NULL,
	     szText,
	     szCaption,
	     MB_OK | MB_ICONINFORMATION);

  return 0;
}

/* EOF */
