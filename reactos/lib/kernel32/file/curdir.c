/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <windows.h>

/* GLOBALS *******************************************************************/

static unsigned char CurrentDirectory[255];

/* FUNCTIONS *****************************************************************/

DWORD GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer)
{
   nBufferLength = min(nBufferLength, lstrlenA(CurrentDirectory));
   lstrncpyA(lpBuffer,CurrentDirectory,nBufferLength);
   return(nBufferLength);
}

BOOL SetCurrentDirectoryA(LPCSTR lpPathName)
{
   lstrcpyA(CurrentDirectory,lpPathName);
   return(TRUE);
}
