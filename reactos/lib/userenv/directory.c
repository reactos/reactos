/* $Id: directory.c,v 1.2 2004/01/16 15:31:53 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/directory.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath)
{
  WCHAR szFileName[MAX_PATH];
  WCHAR szFullSrcName[MAX_PATH];
  WCHAR szFullDstName[MAX_PATH];
  WIN32_FIND_DATAW FindFileData;
  LPWSTR lpSrcPtr;
  LPWSTR lpDstPtr;
  HANDLE hFind;

  DPRINT ("CopyDirectory (%S, %S) called\n",
	  lpDestinationPath, lpSourcePath);

  wcscpy (szFileName, lpSourcePath);
  wcscat (szFileName, L"\\*.*");

  hFind = FindFirstFileW (szFileName,
			  &FindFileData);
  if (hFind == INVALID_HANDLE_VALUE)
    {
      DPRINT1 ("Error: %lu\n", GetLastError());
      return FALSE;
    }

  wcscpy (szFullSrcName, lpSourcePath);
  lpSrcPtr = AppendBackslash (szFullSrcName);

  wcscpy (szFullDstName, lpDestinationPath);
  lpDstPtr = AppendBackslash (szFullDstName);

  for (;;)
    {
      if (wcscmp (FindFileData.cFileName, L".") &&
	  wcscmp (FindFileData.cFileName, L".."))
	{
	  wcscpy (lpSrcPtr, FindFileData.cFileName);
	  wcscpy (lpDstPtr, FindFileData.cFileName);

	  if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
	      DPRINT ("Create directory: %S\n", szFullDstName);
	      if (!CreateDirectoryW (szFullDstName, NULL))
		{
		  if (GetLastError () != ERROR_ALREADY_EXISTS)
		    {
		      DPRINT1 ("Error: %lu\n", GetLastError());

		      FindClose (hFind);
		      return FALSE;
		    }
		}

	      if (!CopyDirectory (szFullDstName, szFullSrcName))
		{
		  DPRINT1 ("Error: %lu\n", GetLastError());

		  FindClose (hFind);
		  return FALSE;
		}
	    }
	  else
	    {
	      DPRINT1 ("Copy file: %S -> %S\n", szFullSrcName, szFullDstName);
	      if (!CopyFileW (szFullSrcName, szFullDstName, FALSE))
		{
		  DPRINT1 ("Error: %lu\n", GetLastError());

		  FindClose (hFind);
		  return FALSE;
		}
	    }

	  /* Copy file attributes */
	  if (FindFileData.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY)
	    {
	      SetFileAttributesW (szFullDstName,
				  FindFileData.dwFileAttributes);
	    }
	}

      if (!FindNextFileW (hFind, &FindFileData))
	{
	  if (GetLastError () != ERROR_NO_MORE_FILES)
	    {
	      DPRINT1 ("Error: %lu\n", GetLastError());
	    }

	  break;
	}
    }

  FindClose (hFind);

  DPRINT ("Copy Directory() done\n");

  return TRUE;
}

/* EOF */
