/*
 *  ATTRIB.C - attrib internal command.
 *
 *
 *  History:
 *
 *    04-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        started
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        implementation works, except recursion ("attrib /s").
 *
 *    05-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        major rewrite.
 *        fixed recursion ("attrib /s").
 *        started directory support ("attrib /s /d").
 *        updated help text.
 *
 *    14-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 *
 *    21-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added check for invalid filenames.
 *
 *    23-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added handling of multiple filenames.
 */

#include "config.h"

#ifdef INCLUDE_CMD_ATTRIB

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


static VOID
PrintAttribute (LPTSTR pszPath, LPTSTR pszFile, BOOL bRecurse)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	TCHAR  szFullName[MAX_PATH];
	LPTSTR pszFileName;

	/* prepare full file name buffer */
	_tcscpy (szFullName, pszPath);
	pszFileName = szFullName + _tcslen (szFullName);

	/* display all subdirectories */
	if (bRecurse)
	{
		/* append file name */
		_tcscpy (pszFileName, pszFile);

		hFind = FindFirstFile (szFullName, &findData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			ErrorMessage (GetLastError (), pszFile);
			return;
		}

		do
		{
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			if (!_tcscmp (findData.cFileName, _T(".")) ||
				!_tcscmp (findData.cFileName, _T("..")))
				continue;

			_tcscpy (pszFileName, findData.cFileName);
			_tcscat (pszFileName, _T("\\"));
			PrintAttribute (szFullName, pszFile, bRecurse);
		}
		while (FindNextFile (hFind, &findData));
		FindClose (hFind);
	}

	/* append file name */
	_tcscpy (pszFileName, pszFile);

	/* display current directory */
	hFind = FindFirstFile (szFullName, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		ErrorMessage (GetLastError (), pszFile);
		return;
	}

	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		_tcscpy (pszFileName, findData.cFileName);

		ConOutPrintf (_T("%c  %c%c%c     %s\n"),
		              (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? _T('A') : _T(' '),
		              (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ? _T('S') : _T(' '),
		              (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? _T('H') : _T(' '),
		              (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? _T('R') : _T(' '),
		              szFullName);
	}
	while (FindNextFile (hFind, &findData));
	FindClose (hFind);
}


static VOID
ChangeAttribute (LPTSTR pszPath, LPTSTR pszFile, DWORD dwMask,
				 DWORD dwAttrib, BOOL bRecurse, BOOL bDirectories)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	DWORD  dwAttribute;
	TCHAR  szFullName[MAX_PATH];
	LPTSTR pszFileName;

	/* prepare full file name buffer */
	_tcscpy (szFullName, pszPath);
	pszFileName = szFullName + _tcslen (szFullName);

	/* change all subdirectories */
	if (bRecurse)
	{
		/* append file name */
		_tcscpy (pszFileName, _T("*.*"));

		hFind = FindFirstFile (szFullName, &findData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			ErrorMessage (GetLastError (), pszFile);
			return;
		}

		do
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!_tcscmp (findData.cFileName, _T(".")) ||
					!_tcscmp (findData.cFileName, _T("..")))
					continue;

				_tcscpy (pszFileName, findData.cFileName);
				_tcscat (pszFileName, _T("\\"));

				ChangeAttribute (szFullName, pszFile, dwMask,
								 dwAttrib, bRecurse, bDirectories);
			}
		}
		while (FindNextFile (hFind, &findData));
		FindClose (hFind);
	}

	/* append file name */
	_tcscpy (pszFileName, pszFile);

	hFind = FindFirstFile (szFullName, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		ErrorMessage (GetLastError (), pszFile);
		return;
	}

	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		_tcscpy (pszFileName, findData.cFileName);

		dwAttribute = GetFileAttributes (szFullName);

		if (dwAttribute != 0xFFFFFFFF)
		{
			dwAttribute = (dwAttribute & ~dwMask) | dwAttrib;
			SetFileAttributes (szFullName, dwAttribute);
		}
	}
	while (FindNextFile (hFind, &findData));
	FindClose (hFind);
}


INT CommandAttrib (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    argc, i;
	TCHAR  szPath[MAX_PATH];
	TCHAR  szFileName [MAX_PATH];
	BOOL   bRecurse = FALSE;
	BOOL   bDirectories = FALSE;
	DWORD  dwAttrib = 0;
	DWORD  dwMask = 0;

	/* initialize strings */
	szPath[0] = _T('\0');
	szFileName[0] = _T('\0');

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or changes file attributes.\n\n"
					   "ATTRIB [+R | -R] [+A | -A] [+S | -S] [+H | -H] file ...\n"
					   "       [/S [/D]]\n\n"
					   "  +   Sets an attribute\n"
					   "  -   Clears an attribute\n"
					   "  R   Read-only file attribute\n"
					   "  A   Archive file attribute\n"
					   "  S   System file attribute\n"
					   "  H   Hidden file attribute\n"
					   "  /S  Processes matching files in the current directory\n"
					   "      and all subdirectories\n"
					   "  /D  Processes direcories as well\n\n"
					   "Type ATTRIB without a parameter to display the attributes of all files."));
		return 0;
	}

	/* build parameter array */
	arg = split (param, &argc, FALSE);

	/* check for options */
	for (i = 0; i < argc; i++)
	{
		if (_tcsicmp (arg[i], _T("/s")) == 0)
			bRecurse = TRUE;
		else if (_tcsicmp (arg[i], _T("/d")) == 0)
			bDirectories = TRUE;
	}

	/* create attributes and mask */
	for (i = 0; i < argc; i++)
	{
		if (*arg[i] == _T('+'))
		{
			if (_tcslen (arg[i]) != 2)
			{
				error_invalid_parameter_format (arg[i]);
				freep (arg);
				return -1;
			}

			switch ((TCHAR)_totupper (arg[i][1]))
			{
				case _T('A'):
					dwMask   |= FILE_ATTRIBUTE_ARCHIVE;
					dwAttrib |= FILE_ATTRIBUTE_ARCHIVE;
					break;

				case _T('H'):
					dwMask   |= FILE_ATTRIBUTE_HIDDEN;
					dwAttrib |= FILE_ATTRIBUTE_HIDDEN;
					break;

				case _T('R'):
					dwMask   |= FILE_ATTRIBUTE_READONLY;
					dwAttrib |= FILE_ATTRIBUTE_READONLY;
					break;

				case _T('S'):
					dwMask   |= FILE_ATTRIBUTE_SYSTEM;
					dwAttrib |= FILE_ATTRIBUTE_SYSTEM;
					break;

				default:
					error_invalid_parameter_format (arg[i]);
					freep (arg);
					return -1;
			}
		}
		else if (*arg[i] == _T('-'))
		{
			if (_tcslen (arg[i]) != 2)
			{
				error_invalid_parameter_format (arg[i]);
				freep (arg);
				return -1;
			}

			switch ((TCHAR)_totupper (arg[i][1]))
			{
				case _T('A'):
					dwMask   |= FILE_ATTRIBUTE_ARCHIVE;
					dwAttrib &= ~FILE_ATTRIBUTE_ARCHIVE;
					break;

				case _T('H'):
					dwMask   |= FILE_ATTRIBUTE_HIDDEN;
					dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
					break;

				case _T('R'):
					dwMask   |= FILE_ATTRIBUTE_READONLY;
					dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
					break;

				case _T('S'):
					dwMask   |= FILE_ATTRIBUTE_SYSTEM;
					dwAttrib &= ~FILE_ATTRIBUTE_SYSTEM;
					break;

				default:
					error_invalid_parameter_format (arg[i]);
					freep (arg);
					return -1;
			}
		}
	}

	if (argc == 0)
	{
		DWORD len;

		len = GetCurrentDirectory (MAX_PATH, szPath);
		if (szPath[len-1] != _T('\\'))
		{
			szPath[len] = _T('\\');
			szPath[len + 1] = 0;
		}
		_tcscpy (szFileName, _T("*.*"));
		PrintAttribute (szPath, szFileName, bRecurse);
		freep (arg);
		return 0;
	}

	/* get full file name */
	for (i = 0; i < argc; i++)
	{
		if ((*arg[i] != _T('+')) && (*arg[i] != _T('-')) &&	(*arg[i] != _T('/')))
		{
			LPTSTR p;
			GetFullPathName (arg[i], MAX_PATH, szPath, NULL);
			p = _tcsrchr (szPath, _T('\\')) + 1;
			_tcscpy (szFileName, p);
			*p = _T('\0');

			if (dwMask == 0)
				PrintAttribute (szPath, szFileName, bRecurse);
			else
				ChangeAttribute (szPath, szFileName, dwMask,
								 dwAttrib, bRecurse, bDirectories);
		}
	}

	freep (arg);
	return 0;
}

#endif /* INCLUDE_CMD_ATTRIB */
