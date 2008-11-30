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
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/directory.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ***************************************************************/

BOOL WINAPI
CopyProfileDirectoryA(LPCSTR lpSourcePath,
		      LPCSTR lpDestinationPath,
		      DWORD dwFlags)
{
  UNICODE_STRING SrcPath;
  UNICODE_STRING DstPath;
  NTSTATUS Status;
  BOOL bResult;

  Status = RtlCreateUnicodeStringFromAsciiz(&SrcPath,
                                            (LPSTR)lpSourcePath);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&DstPath,
                                            (LPSTR)lpDestinationPath);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&SrcPath);
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = CopyProfileDirectoryW(SrcPath.Buffer,
                                  DstPath.Buffer,
                                  dwFlags);

  RtlFreeUnicodeString(&DstPath);
  RtlFreeUnicodeString(&SrcPath);

  return bResult;
}


BOOL WINAPI
CopyProfileDirectoryW(LPCWSTR lpSourcePath,
		      LPCWSTR lpDestinationPath,
		      DWORD dwFlags)
{
  /* FIXME: dwFlags are ignored! */
  return CopyDirectory(lpDestinationPath, lpSourcePath);
}


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
	      if (!CreateDirectoryExW (szFullSrcName, szFullDstName, NULL))
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
	      DPRINT ("Copy file: %S -> %S\n", szFullSrcName, szFullDstName);
	      if (!CopyFileW (szFullSrcName, szFullDstName, FALSE))
		{
		  DPRINT1 ("Error: %lu\n", GetLastError());

		  FindClose (hFind);
		  return FALSE;
		}
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


BOOL
CreateDirectoryPath (LPCWSTR lpPathName,
		     LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  WCHAR szPath[MAX_PATH];
  LPWSTR Ptr;
  DWORD dwError;

  DPRINT ("CreateDirectoryPath() called\n");

  if (lpPathName == NULL || *lpPathName == 0)
    return TRUE;

  if (CreateDirectoryW (lpPathName,
			lpSecurityAttributes))
    return TRUE;

  dwError = GetLastError ();
  if (dwError == ERROR_ALREADY_EXISTS)
    return TRUE;

  wcscpy (szPath, lpPathName);

  if (wcslen(szPath) > 3 && szPath[1] == ':' && szPath[2] == '\\')
    {
      Ptr = &szPath[3];
    }
  else
    {
      Ptr = szPath;
    }

  while (Ptr != NULL)
    {
      Ptr = wcschr (Ptr, L'\\');
      if (Ptr != NULL)
        *Ptr = 0;

      DPRINT ("CreateDirectory(%S)\n", szPath);
      if (!CreateDirectoryW (szPath,
			     lpSecurityAttributes))
	{
	  dwError = GetLastError ();
	  if (dwError != ERROR_ALREADY_EXISTS)
	    return FALSE;
	}

      if (Ptr != NULL)
	{
	  *Ptr = L'\\';
	  Ptr++;
	}
    }

  DPRINT ("CreateDirectoryPath() done\n");

  return TRUE;
}


static BOOL
RecursiveRemoveDir (LPCWSTR lpPath)
{
  WCHAR szPath[MAX_PATH];
  WIN32_FIND_DATAW FindData;
  HANDLE hFind;
  BOOL bResult;

  wcscpy (szPath, lpPath);
  wcscat (szPath, L"\\*.*");
  DPRINT ("Search path: '%S'\n", szPath);

  hFind = FindFirstFileW (szPath,
			  &FindData);
  if (hFind == INVALID_HANDLE_VALUE)
    return FALSE;

  bResult = TRUE;
  while (TRUE)
    {
      if (wcscmp (FindData.cFileName, L".") &&
	  wcscmp (FindData.cFileName, L".."))
	{
	  wcscpy (szPath, lpPath);
	  wcscat (szPath, L"\\");
	  wcscat (szPath, FindData.cFileName);
	  DPRINT ("File name: '%S'\n", szPath);

	  if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
	      DPRINT ("Delete directory: '%S'\n", szPath);

	      if (!RecursiveRemoveDir (szPath))
		{
		  bResult = FALSE;
		  break;
		}

	      if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		{
		  SetFileAttributesW (szPath,
				      FindData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
		}

	      if (!RemoveDirectoryW (szPath))
		{
		  bResult = FALSE;
		  break;
		}
	    }
	  else
	    {
	      DPRINT ("Delete file: '%S'\n", szPath);

	      if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
		{
		  SetFileAttributesW (szPath,
				      FILE_ATTRIBUTE_NORMAL);
		}

	      if (!DeleteFileW (szPath))
		{
		  bResult = FALSE;
		  break;
		}
	    }
	}

      if (!FindNextFileW (hFind, &FindData))
	{
	  if (GetLastError () != ERROR_NO_MORE_FILES)
	    {
	      DPRINT1 ("Error: %lu\n", GetLastError());
	      bResult = FALSE;
	      break;
	    }

	  break;
	}
    }

  FindClose (hFind);

  return bResult;
}


BOOL
RemoveDirectoryPath (LPCWSTR lpPathName)
{
  if (!RecursiveRemoveDir (lpPathName))
    return FALSE;

  DPRINT ("Delete directory: '%S'\n", lpPathName);
  return RemoveDirectoryW (lpPathName);
}

/* EOF */
