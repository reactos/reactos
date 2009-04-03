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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/dllmain.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

#include "debug.h"

//#define LOG_DEBUG_MESSAGES

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

BOOL WINAPI
DllMain (HINSTANCE hInstance,
	 DWORD dwReason,
	 LPVOID lpReserved)
{

  return TRUE;
}


void
DebugPrint (char* fmt,...)
{
#ifdef LOG_DEBUG_MESSAGES
  char FileName[MAX_PATH];
  HANDLE hLogFile;
  DWORD dwBytesWritten;
#endif
  char buffer[512];
  va_list ap;

  va_start (ap, fmt);
  vsprintf (buffer, fmt, ap);
  va_end (ap);

  OutputDebugStringA (buffer);

#ifdef LOG_DEBUG_MESSAGES
  strcpy (FileName, "C:\\reactos\\samlib.log");
  hLogFile = CreateFileA (FileName,
			  GENERIC_WRITE,
			  0,
			  NULL,
			  OPEN_ALWAYS,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);
  if (hLogFile == INVALID_HANDLE_VALUE)
    return;

  if (SetFilePointer(hLogFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
    {
      CloseHandle (hLogFile);
      return;
    }

  WriteFile (hLogFile,
	     buffer,
	     strlen(buffer),
	     &dwBytesWritten,
	     NULL);

  CloseHandle (hLogFile);
#endif
}

/* EOF */
