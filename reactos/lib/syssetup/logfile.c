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
/* $Id: logfile.c,v 1.1 2003/05/02 18:07:55 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Log file functions
 * FILE:              lib/syssetup/logfile.c
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ntos.h>
#include <string.h>
#include <tchar.h>

#include <syssetup.h>


/* GLOBALS ******************************************************************/

HANDLE hLogFile = NULL;


/* FUNCTIONS ****************************************************************/

BOOL STDCALL
InitializeSetupActionLog (BOOL bDeleteOldLogFile)
{
  WCHAR szFileName[MAX_PATH];

  GetWindowsDirectoryW (szFileName,
			MAX_PATH);

  if (szFileName[wcslen (szFileName)] != L'\\')
    {
      wcsncat (szFileName,
	       L"\\",
	       MAX_PATH);
    }
  wcsncat (szFileName,
	   L"setuplog.txt",
	   MAX_PATH);

  if (bDeleteOldLogFile != FALSE)
    {
      SetFileAttributesW (szFileName,
			  FILE_ATTRIBUTE_NORMAL);
      DeleteFileW (szFileName);
    }

  hLogFile = CreateFileW (szFileName,
			  GENERIC_READ | GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL,
			  OPEN_ALWAYS,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);
  if (hLogFile == INVALID_HANDLE_VALUE)
    {
      hLogFile = NULL;
      return FALSE;
    }

  return TRUE;
}


VOID STDCALL
TerminateSetupActionLog (VOID)
{
  if (hLogFile != NULL)
    {
      CloseHandle (hLogFile);
      hLogFile = NULL;
    }
}


BOOL STDCALL
LogItem (DWORD dwSeverity,
	 LPWSTR lpMessageText)
{
  LPSTR lpNewLine = "\r\n";
  LPSTR lpSeverityString;
  LPSTR lpMessageString;
  DWORD dwMessageLength;
  DWORD dwMessageSize;
  DWORD dwWritten;

  /* Get the severity code string */
  switch (dwSeverity)
    {
      case SEVERITY_INFORMATION:
	lpSeverityString = "Information : ";
	break;

      case SEVERITY_WARNING:
	lpSeverityString = "Warning : ";
	break;

      case SEVERITY_ERROR:
	lpSeverityString = "Error : ";
	break;

      case SEVERITY_FATAL_ERROR:
	lpSeverityString = "Fatal error : ";
	break;

      default:
	lpSeverityString = "Unknown : ";
	break;
    }

  /* Get length of the converted ansi string */
  dwMessageLength = wcslen(lpMessageText) * sizeof(WCHAR);
  RtlUnicodeToMultiByteSize (&dwMessageSize,
			     lpMessageText,
			     dwMessageLength);

  /* Allocate message string buffer */
  lpMessageString = (LPSTR) HeapAlloc (GetProcessHeap (),
				       HEAP_ZERO_MEMORY,
				       dwMessageSize);
  if (lpMessageString == NULL)
    {
      return FALSE;
    }

  /* Convert unicode to ansi */
  RtlUnicodeToMultiByteN (lpMessageString,
			  dwMessageSize,
			  NULL,
			  lpMessageText,
			  dwMessageLength);

  /* Set file pointer to the end of the file */
  SetFilePointer (hLogFile,
		  0,
		  NULL,
		  FILE_END);

  /* Write severity code */
  WriteFile (hLogFile,
	     lpSeverityString,
	     strlen (lpSeverityString),
	     &dwWritten,
	     NULL);

  /* Write message string */
  WriteFile (hLogFile,
	     lpMessageString,
	     dwMessageSize,
	     &dwWritten,
	     NULL);

  /* Write newline */
  WriteFile (hLogFile,
	     lpNewLine,
	     2,
	     &dwWritten,
	     NULL);

  HeapFree (GetProcessHeap (),
	    0,
	    lpMessageString);

  return TRUE;
}

/* EOF */
