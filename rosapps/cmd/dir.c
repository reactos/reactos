/* $Id: dir.c,v 1.10 2001/02/28 22:33:23 ekohl Exp $
 *
 *  DIR.C - dir internal command.
 *
 *
 *  History:
 *
 *    01/29/97 (Tim Norman)
 *        started.
 *
 *    06/13/97 (Tim Norman)
 *      Fixed code.
 *
 *    07/12/97 (Tim Norman)
 *        Fixed bug that caused the root directory to be unlistable
 *
 *    07/12/97 (Marc Desrochers)
 *        Changed to use maxx, maxy instead of findxy()
 *
 *    06/08/98 (Rob Lake)
 *        Added compatibility for /w in dir
 *
 *    06/09/98 (Rob Lake)
 *        Compatibility for dir/s started
 *        Tested that program finds directories off root fine
 *
 *    06/10/98 (Rob Lake)
 *        do_recurse saves the cwd and also stores it in Root
 *        build_tree adds the cwd to the beginning of its' entries
 *        Program runs fine, added print_tree -- works fine.. as EXE,
 *        program won't work properly as COM.
 *
 *    06/11/98 (Rob Lake)
 *        Found problem that caused COM not to work
 *
 *    06/12/98 (Rob Lake)
 *        debugged...
 *        added free mem routine
 *
 *    06/13/98 (Rob Lake)
 *        debugged the free mem routine
 *        debugged whole thing some more
 *        Notes:
 *        ReadDir stores Root name and _Read_Dir does the hard work
 *        PrintDir prints Root and _Print_Dir does the hard work
 *        KillDir kills Root _after_ _Kill_Dir does the hard work
 *        Integrated program into DIR.C(this file) and made some same
 *        changes throughout
 *
 *    06/14/98 (Rob Lake)
 *        Cleaned up code a bit, added comments
 *
 *    06/16/98 (Rob Lake)
 *        Added error checking to my previously added routines
 *
 *    06/17/98 (Rob Lake)
 *        Rewrote recursive functions, again! Most other recursive
 *        functions are now obsolete -- ReadDir, PrintDir, _Print_Dir,
 *        KillDir and _Kill_Dir.  do_recurse does what PrintDir did
 *        and _Read_Dir did what it did before along with what _Print_Dir
 *        did.  Makes /s a lot faster!
 *        Reports 2 more files/dirs that MS-DOS actually reports
 *        when used in root directory(is this because dir defaults
 *        to look for read only files?)
 *        Added support for /b, /a and /l
 *        Made error message similar to DOS error messages
 *        Added help screen
 *
 *    06/20/98 (Rob Lake)
 *        Added check for /-(switch) to turn off previously defined
 *        switches.
 *        Added ability to check for DIRCMD in environment and
 *        process it
 *
 *    06/21/98 (Rob Lake)
 *        Fixed up /B
 *        Now can dir *.ext/X, no spaces!
 *
 *    06/29/98 (Rob Lake)
 *        error message now found in command.h
 *
 *    07/08/1998 (John P. Price)
 *        removed extra returns; closer to MSDOS
 *        fixed wide display so that an extra return is not displayed
 *        when there is five filenames in the last line.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *
 *    04-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Converted source code to Win32, except recursive dir ("dir /s").
 *
 *    10-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed recursive dir ("dir /s").
 *
 *    14-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Converted to Win32 directory functions and
 *        fixed some output bugs. There are still some more ;)
 *
 *    10-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added "/N" and "/4" options, "/O" is a dummy.
 *        Added locale support.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection safe!
 *
 *    01-Mar-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced all runtime io functions by their Win32 counterparts.
 *  
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        dir /s now works in deeper trees
 */

#include "config.h"

#ifdef INCLUDE_CMD_DIR
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "cmd.h"



/* flag definitions */
enum
{
	DIR_RECURSE = 0x0001,
	DIR_PAGE    = 0x0002,
	DIR_WIDE    = 0x0004,        /* Rob Lake */
	DIR_BARE    = 0x0008,        /* Rob Lake */
	DIR_ALL     = 0x0010,        /* Rob Lake */
	DIR_LWR     = 0x0020,        /* Rob Lake */
	DIR_SORT    = 0x0040,        /* /O sort */
	DIR_NEW     = 0x0080,        /* /N new style */
	DIR_FOUR    = 0x0100         /* /4 four digit year */
};


/* Globally save the # of dirs, files and bytes,
 * probabaly later pass them to functions. Rob Lake  */
static ULONG recurse_dir_cnt;
static ULONG recurse_file_cnt;
static ULARGE_INTEGER recurse_bytes;


/*
 * help
 *
 * displays help screen for dir
 * Rob Lake
 */
static VOID Help (VOID)
{
	ConOutPuts (_T("Displays a list of files and subdirectories in a directory.\n"
       "\n"
       "DIR [drive:][path][filename] [/A] [/B] [/L] [/N] [/S] [/P] [/W] [/4]\n"
       "\n"
       "  [drive:][path][filename]\n"
       "              Specifies drive, directory, and/or files to list.\n"
       "\n"
       "  /A          Displays files with HIDDEN SYSTEM attributes\n"
       "              default is ARCHIVE and READ ONLY\n"
       "  /B          Uses bare format (no heading information or summary).\n"
       "  /L          Uses lowercase.\n"
       "  /N          New long list format where filenames are on the far right.\n"
       "  /S          Displays files in specified directory and all subdirectories\n"
       "  /P          Pauses after each screen full\n"
       "  /W          Prints in wide format\n"
       "  /4          Display four digit years.\n"
       "\n"
       "Switches may be present in the DIRCMD environment variable.  Use\n"
       "of the - (hyphen) can turn off defined swtiches.  Ex. /-W would\n"
       "turn off printing in wide format.\n"
      ));
}


/*
 * DirReadParam
 *
 * read the parameters from the command line
 */
static BOOL
DirReadParam (LPTSTR line, LPTSTR *param, LPDWORD lpFlags)
{
	INT slash = 0;

	if (!line)
		return TRUE;

	*param = NULL;

	/* scan the command line, processing switches */
	while (*line)
	{
		/* process switch */
		if (*line == _T('/') || slash)
		{
			if (!slash)
				line++;
			slash = 0;
			if (*line == _T('-'))
			{
				line++;
				if (_totupper (*line) == _T('S'))
					*lpFlags &= ~DIR_RECURSE;
				else if (_totupper (*line) == _T('P'))
					*lpFlags &= ~DIR_PAGE;
				else if (_totupper (*line) == _T('W'))
					*lpFlags &= ~DIR_WIDE;
				else if (_totupper (*line) == _T('B'))
					*lpFlags &= ~DIR_BARE;
				else if (_totupper (*line) == _T('A'))
					*lpFlags &= ~DIR_ALL;
				else if (_totupper (*line) == _T('L'))
					*lpFlags &= ~DIR_LWR;
				else if (_totupper (*line) == _T('N'))
					*lpFlags &= ~DIR_NEW;
				else if (_totupper (*line) == _T('O'))
					*lpFlags &= ~DIR_SORT;
				else if (_totupper (*line) == _T('4'))
					*lpFlags &= ~DIR_FOUR;
				else
				{
					error_invalid_switch ((TCHAR)_totupper (*line));
					return FALSE;
				}
				line++;
				continue;
			}
			else
			{
				if (_totupper (*line) == _T('S'))
					*lpFlags |= DIR_RECURSE;
				else if (_totupper (*line) == _T('P'))
					*lpFlags |= DIR_PAGE;
				else if (_totupper (*line) == _T('W'))
					*lpFlags |= DIR_WIDE;
				else if (_totupper (*line) == _T('B'))
					*lpFlags |= DIR_BARE;
				else if (_totupper (*line) == _T('A'))
					*lpFlags |= DIR_ALL;
				else if (_totupper (*line) == _T('L'))
					*lpFlags |= DIR_LWR;
				else if (_totupper (*line) == _T('N'))
					*lpFlags |= DIR_NEW;
				else if (_totupper (*line) == _T('O'))
					*lpFlags |= DIR_SORT;
				else if (_totupper (*line) == _T('4'))
					*lpFlags |= DIR_FOUR;
				else if (*line == _T('?'))
				{
					Help();
					return FALSE;
				}
				else
				{
					error_invalid_switch ((TCHAR)_totupper (*line));
					return FALSE;
				}
				line++;
				continue;
			}
		}

		/* process parameter */
		if (!_istspace (*line))
		{
			if (*param)
			{
				error_too_many_parameters (*param);
				return FALSE;
			}

			*param = line;

			/* skip to end of line or next whitespace or next / */
			while (*line && !_istspace (*line) && *line != _T('/'))
				line++;

			/* if end of line, return */
			if (!*line)
				return TRUE;

			/* if parameter, remember to process it later */
			if (*line == _T('/'))
				slash = 1;

			*line++ = 0;
			continue;
		}

		line++;
	}

	if (slash)
	{
		error_invalid_switch ((TCHAR)_totupper (*line));
		return FALSE;
	}

	return TRUE;
}


/*
 * ExtendFilespec
 *
 * extend the filespec, possibly adding wildcards
 */
static VOID
ExtendFilespec (LPTSTR file)
{
	INT len = 0;

	if (!file)
		return;

	/* if no file spec, change to "*.*" */
	if (*file == _T('\0'))
	{
		_tcscpy (file, _T("*.*"));
		return;
	}

	/* if starts with . add * in front */
	if (*file == _T('.'))
	{
		memmove (&file[1], &file[0], (_tcslen (file) + 1) * sizeof(TCHAR));
		file[0] = _T('*');
	}

	/* if no . add .* */
	if (!_tcschr (file, _T('.')))
	{
		_tcscat (file, _T(".*"));
		return;
	}

	/* if last character is '.' add '*' */
	len = _tcslen (file);
	if (file[len - 1] == _T('.'))
	{
		_tcscat (file, _T("*"));
		return;
	}
}


/*
 * dir_parse_pathspec
 *
 * split the pathspec into drive, directory, and filespec
 */
static INT
DirParsePathspec (LPTSTR szPathspec, LPTSTR szPath, LPTSTR szFilespec)
{
	TCHAR  szOrigPath[MAX_PATH];
	LPTSTR start;
	LPTSTR tmp;
	INT    i;
	BOOL   bWildcards = FALSE;

	GetCurrentDirectory (MAX_PATH, szOrigPath);

	/* get the drive and change to it */
	if (szPathspec[1] == _T(':'))
	{
		TCHAR szRootPath[] = _T("A:");

		szRootPath[0] = szPathspec[0];
		start = szPathspec + 2;
		SetCurrentDirectory (szRootPath);
	}
	else
	{
		start = szPathspec;
	}


	/* check for wildcards */
	for (i = 0; szPathspec[i]; i++)
	{
		if (szPathspec[i] == _T('*') || szPathspec[i] == _T('?'))
			bWildcards = TRUE;
	}

	/* check if this spec is a directory */
	if (!bWildcards)
	{
		if (SetCurrentDirectory (szPathspec))
		{
			_tcscpy (szFilespec, _T("*.*"));

			if (!GetCurrentDirectory (MAX_PATH, szPath))
			{
				szFilespec[0] = _T('\0');
				SetCurrentDirectory (szOrigPath);
				error_out_of_memory();
				return 1;
			}

			SetCurrentDirectory (szOrigPath);
			return 0;
		}
	}

	/* find the file spec */
	tmp = _tcsrchr (start, _T('\\'));

	/* if no path is specified */
	if (!tmp)
	{
		_tcscpy (szFilespec, start);
		ExtendFilespec (szFilespec);
		if (!GetCurrentDirectory (MAX_PATH, szPath))
		{
			szFilespec[0] = _T('\0');
			SetCurrentDirectory (szOrigPath);
			error_out_of_memory();
			return 1;
		}

		SetCurrentDirectory (szOrigPath);
		return 0;
	}

	/* get the filename */
	_tcscpy (szFilespec, tmp+1);
	ExtendFilespec (szFilespec);

	*tmp = _T('\0');

	/* change to this directory and get its full name */
	if (!SetCurrentDirectory (start))
	{
		*tmp = _T('\\');
		szFilespec[0] = _T('\0');
		SetCurrentDirectory (szOrigPath);
		error_path_not_found ();
		return 1;
	}

	if (!GetCurrentDirectory (MAX_PATH, szPath))
	{
		*tmp = _T('\\');
		szFilespec[0] = _T('\0');
		SetCurrentDirectory (szOrigPath);
		error_out_of_memory ();
		return 1;
	}

	*tmp = _T('\\');

	SetCurrentDirectory (szOrigPath);

	return 0;
}


/*
 * incline
 *
 * increment our line if paginating, display message at end of screen
 */
static BOOL
IncLine (LPINT pLine, DWORD dwFlags)
{
	if (!(dwFlags & DIR_PAGE))
		return FALSE;

	(*pLine)++;

	if (*pLine >= (int)maxy - 2)
	{
		*pLine = 0;
		return (PagePrompt () == PROMPT_BREAK);
	}

	return FALSE;
}


/*
 * PrintDirectoryHeader
 *
 * print the header for the dir command
 */
static BOOL
PrintDirectoryHeader (LPTSTR szPath, LPINT pLine, DWORD dwFlags)
{
	TCHAR szRootName[] = _T("A:\\");
	TCHAR szVolName[80];
	DWORD dwSerialNr;

	if (dwFlags & DIR_BARE)
		return TRUE;

	/* get the media ID of the drive */
	szRootName[0] = szPath[0];
	if (!GetVolumeInformation (szRootName, szVolName, 80, &dwSerialNr,
	                           NULL, NULL, NULL, 0))
	{
		error_invalid_drive();
		return FALSE;
	}

	/* print drive info */
	ConOutPrintf (_T(" Volume in drive %c"), szRootName[0]);

	if (szVolName[0] != _T('\0'))
		ConOutPrintf (_T(" is %s\n"), szVolName);
	else
		ConOutPrintf (_T(" has no label\n"));

	if (IncLine (pLine, dwFlags))
		return FALSE;

	/* print the volume serial number if the return was successful */
	ConOutPrintf (_T(" Volume Serial Number is %04X-%04X\n"),
	              HIWORD(dwSerialNr),
	              LOWORD(dwSerialNr));
	if (IncLine (pLine, dwFlags))
		return FALSE;

	return TRUE;
}


/*
 * convert
 *
 * insert commas into a number
 */
static INT
ConvertULong (ULONG num, LPTSTR des, INT len)
{
	TCHAR temp[32];
	INT c = 0;
	INT n = 0;

	if (num == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num > 0)
		{
			if (((c + 1) % (nNumberGroups + 1)) == 0)
				temp[30 - c++] = cThousandSeparator;
			temp[30 - c++] = (TCHAR)(num % 10) + _T('0');
			num /= 10;
		}

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

	return n;
}


static INT
ConvertULargeInteger (ULARGE_INTEGER num, LPTSTR des, INT len)
{
	TCHAR temp[32];
	INT c = 0;
	INT n = 0;

	if (num.QuadPart == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num.QuadPart > 0)
		{
			if (((c + 1) % (nNumberGroups + 1)) == 0)
				temp[30 - c++] = cThousandSeparator;
			temp[30 - c++] = (TCHAR)(num.QuadPart % 10) + _T('0');
			num.QuadPart /= 10;
		}

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

	return n;
}


static VOID
PrintFileDateTime (LPSYSTEMTIME dt, DWORD dwFlags)
{
	WORD wYear = (dwFlags & DIR_FOUR) ? dt->wYear : dt->wYear%100;

	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			ConOutPrintf (_T("%.2d%c%.2d%c%d"),
					dt->wMonth, cDateSeparator, dt->wDay, cDateSeparator, wYear);
			break;

		case 1: /* ddmmyy */
			ConOutPrintf (_T("%.2d%c%.2d%c%d"),
					dt->wDay, cDateSeparator, dt->wMonth, cDateSeparator, wYear);
			break;

		case 2: /* yymmdd */
			ConOutPrintf (_T("%d%c%.2d%c%.2d"),
					wYear, cDateSeparator, dt->wMonth, cDateSeparator, dt->wDay);
			break;
	}

	switch (nTimeFormat)
	{
		case 0: /* 12 hour format */
		default:
			ConOutPrintf (_T("  %2d%c%.2u%c"),
					(dt->wHour == 0 ? 12 : (dt->wHour <= 12 ? dt->wHour : dt->wHour - 12)),
					cTimeSeparator,
					 dt->wMinute, (dt->wHour <= 11 ? 'a' : 'p'));
			break;

		case 1: /* 24 hour format */
			ConOutPrintf (_T("  %2d%c%.2u"),
					dt->wHour, cTimeSeparator, dt->wMinute);
			break;
	}
}


/*
 * print_summary: prints dir summary
 * Added by Rob Lake 06/17/98 to compact code
 * Just copied Tim's Code and patched it a bit
 *
 */
static INT
PrintSummary (LPTSTR szPath, ULONG ulFiles, ULONG ulDirs, ULARGE_INTEGER bytes,
	      LPINT pLine, DWORD dwFlags)
{
	TCHAR buffer[64];

	if (dwFlags & DIR_BARE)
		return 0;

	/* print number of files and bytes */
	ConvertULong (ulFiles, buffer, sizeof(buffer));
	ConOutPrintf (_T("          %6s File%c"),
	              buffer, ulFiles == 1 ? _T(' ') : _T('s'));

	ConvertULargeInteger (bytes, buffer, sizeof(buffer));
	ConOutPrintf (_T("  %15s byte%c\n"),
	              buffer, bytes.QuadPart == 1 ? _T(' ') : _T('s'));

	if (IncLine (pLine, dwFlags))
		return 1;

	/* print number of dirs and bytes free */
	ConvertULong (ulDirs, buffer, sizeof(buffer));
	ConOutPrintf (_T("          %6s Dir%c"),
	              buffer, ulDirs == 1 ? _T(' ') : _T('s'));


	if (!(dwFlags & DIR_RECURSE))
	{
		ULARGE_INTEGER uliFree;
		TCHAR szRoot[] = _T("A:\\");
		DWORD dwSecPerCl;
		DWORD dwBytPerSec;
		DWORD dwFreeCl;
		DWORD dwTotCl;

		szRoot[0] = szPath[0];
		GetDiskFreeSpace (szRoot, &dwSecPerCl, &dwBytPerSec, &dwFreeCl, &dwTotCl);
		                  uliFree.QuadPart = dwSecPerCl * dwBytPerSec * dwFreeCl;
		ConvertULargeInteger (uliFree, buffer, sizeof(buffer));
		ConOutPrintf (_T("   %15s bytes free\n"), buffer);
	}

	if (IncLine (pLine, dwFlags))
		return 1;

	return 0;
}


/*
 * dir_list
 *
 * list the files in the directory
 */
static INT
DirList (LPTSTR szPath, LPTSTR szFilespec, LPINT pLine, DWORD dwFlags)
{
	TCHAR szFullPath[MAX_PATH];
	WIN32_FIND_DATA file;
	ULARGE_INTEGER bytecount;
	FILETIME   ft;
	SYSTEMTIME dt;
	HANDLE hFile;
	TCHAR  buffer[32];
	ULONG filecount = 0;
	ULONG dircount = 0;
	INT count;

	bytecount.QuadPart = 0;

	_tcscpy (szFullPath, szPath);
	if (szFullPath[_tcslen(szFullPath) - 1] != _T('\\'))
		_tcscat (szFullPath, _T("\\"));
	_tcscat (szFullPath, szFilespec);

	hFile = FindFirstFile (szFullPath, &file);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		/* Don't want to print anything if scanning recursively
		 * for a file. RL
		 */
		if ((dwFlags & DIR_RECURSE) == 0)
		{
			FindClose (hFile);
			error_file_not_found ();
			if (IncLine (pLine, dwFlags))
				return 0;
			return 1;
		}
		FindClose (hFile);
		return 0;
	}

	/* moved down here because if we are recursively searching and
	 * don't find any files, we don't want just to print
	 * Directory of C:\SOMEDIR
	 * with nothing else
	 * Rob Lake 06/13/98
	 */
	if ((dwFlags & DIR_BARE) == 0)
	{
		ConOutPrintf (_T(" Directory of %s\n"), szPath);
		if (IncLine (pLine, dwFlags))
			return 1;
		ConOutPrintf (_T("\n"));
		if (IncLine (pLine, dwFlags))
			return 1;
	}

	/* For counting columns of output */
	count = 0;

	do
	{
		/* next file, if user doesn't want all files */
		if (!(dwFlags & DIR_ALL) &&
			((file.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
			 (file.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)))
			continue;

		if (dwFlags & DIR_LWR)
		{
			_tcslwr (file.cAlternateFileName);
		}

		if (dwFlags & DIR_WIDE && (dwFlags & DIR_BARE) == 0)
		{
			ULARGE_INTEGER uliSize;

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (file.cAlternateFileName[0] == _T('\0'))
					_stprintf (buffer, _T("[%s]"), file.cFileName);
				else
					_stprintf (buffer, _T("[%s]"), file.cAlternateFileName);
				dircount++;
			}
			else
			{
				if (file.cAlternateFileName[0] == _T('\0'))
					_stprintf (buffer, _T("%s"), file.cFileName);
				else
					_stprintf (buffer, _T("%s"), file.cAlternateFileName);
				filecount++;
			}

			ConOutPrintf (_T("%-15s"), buffer);
			count++;
			if (count == 5)
			{
				/* output 5 columns */
				ConOutPrintf (_T("\n"));
				if (IncLine (pLine, dwFlags))
					return 1;
				count = 0;
			}

			uliSize.u.LowPart = file.nFileSizeLow;
			uliSize.u.HighPart = file.nFileSizeHigh;
			bytecount.QuadPart += uliSize.QuadPart;
		}
		else if (dwFlags & DIR_BARE)
		{
			ULARGE_INTEGER uliSize;

			if (_tcscmp (file.cFileName, _T(".")) == 0 ||
				_tcscmp (file.cFileName, _T("..")) == 0)
				continue;

			if (dwFlags & DIR_RECURSE)
			{
				TCHAR dir[MAX_PATH];

				_tcscpy (dir, szPath);
				_tcscat (dir, _T("\\"));
				if (dwFlags & DIR_LWR)
					_tcslwr (dir);
				ConOutPrintf (dir);
			}

			ConOutPrintf (_T("%-13s\n"), file.cFileName);
			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				dircount++;
			else
				filecount++;
			if (IncLine (pLine, dwFlags))
				return 1;

			uliSize.u.LowPart = file.nFileSizeLow;
			uliSize.u.HighPart = file.nFileSizeHigh;
			bytecount.QuadPart += uliSize.QuadPart;
		}
		else
		{
			if (dwFlags & DIR_NEW)
			{
				/* print file date and time */
				if (FileTimeToLocalFileTime (&file.ftLastWriteTime, &ft))
				{
					FileTimeToSystemTime (&ft, &dt);
					PrintFileDateTime (&dt, dwFlags);
				}

				/* print file size */
				if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					ConOutPrintf (_T("         <DIR>         "));
					dircount++;
				}
				else
				{
					ULARGE_INTEGER uliSize;

					uliSize.u.LowPart = file.nFileSizeLow;
					uliSize.u.HighPart = file.nFileSizeHigh;

					ConvertULargeInteger (uliSize, buffer, sizeof(buffer));
					ConOutPrintf (_T("   %20s"), buffer);

					bytecount.QuadPart += uliSize.QuadPart;
					filecount++;
				}

				/* print long filename */
				ConOutPrintf (_T(" %s\n"), file.cFileName);
			}
			else
			{
				if (file.cFileName[0] == _T('.'))
					ConOutPrintf (_T("%-13s "), file.cFileName);
				else if (file.cAlternateFileName[0] == _T('\0'))
				{
					TCHAR szShortName[13];
					LPTSTR ext;

					_tcsncpy (szShortName, file.cFileName, 13);
					ext = _tcschr (szShortName, _T('.'));
					if (!ext)
						ext = _T("");
					else
						*ext++ = _T('\0');
					ConOutPrintf (_T("%-8s %-3s  "), szShortName, ext);
				}
				else
				{
					LPTSTR ext;

					ext = _tcschr (file.cAlternateFileName, _T('.'));
					if (!ext)
						ext = _T("");
					else
						*ext++ = _T('\0');
					ConOutPrintf (_T("%-8s %-3s  "), file.cAlternateFileName, ext);
				}

				/* print file size */
				if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					ConOutPrintf ("%-14s", "<DIR>");
					dircount++;
				}
				else
				{
					ULARGE_INTEGER uliSize;

					uliSize.u.LowPart = file.nFileSizeLow;
					uliSize.u.HighPart = file.nFileSizeHigh;
					ConvertULargeInteger (uliSize, buffer, sizeof(buffer));
					ConOutPrintf (_T("   %10s "), buffer);
					bytecount.QuadPart += uliSize.QuadPart;
					filecount++;
				}

				/* print file date and time */
				if (FileTimeToLocalFileTime (&file.ftLastWriteTime, &ft))
				{
					FileTimeToSystemTime (&ft, &dt);
					PrintFileDateTime (&dt, dwFlags);
				}

				/* print long filename */
				ConOutPrintf (" %s\n", file.cFileName);
			}

			if (IncLine (pLine, dwFlags))
				return 1;
		}
	}
	while (FindNextFile (hFile, &file));
	FindClose (hFile);

	/* Rob Lake, need to make clean output */
	/* JPP 07/08/1998 added check for count != 0 */
	if ((dwFlags & DIR_WIDE) && (count != 0))
	{
		ConOutPrintf (_T("\n"));
		if (IncLine (pLine, dwFlags))
			return 1;
	}

	if (filecount || dircount)
	{
		recurse_dir_cnt += dircount;
		recurse_file_cnt += filecount;
		recurse_bytes.QuadPart += bytecount.QuadPart;

		/* print_summary */
		if (PrintSummary (szPath, filecount, dircount, bytecount, pLine, dwFlags))
			return 1;
	}
	else
	{
		error_file_not_found ();
		return 1;
	}

	return 0;
}


/*
 * _Read_Dir: Actual function that does recursive listing
 */
static INT
DirRead (LPTSTR szPath, LPTSTR szFilespec, LPINT pLine, DWORD dwFlags)
{
	TCHAR szFullPath[MAX_PATH];
	WIN32_FIND_DATA file;
	HANDLE hFile;

	_tcscpy (szFullPath, szPath);
	if (szFullPath[_tcslen (szFullPath) - 1] != _T('\\'))
		_tcscat (szFullPath, _T("\\"));
	_tcscat (szFullPath, szFilespec);

	hFile = FindFirstFile (szFullPath, &file);
	if (hFile == INVALID_HANDLE_VALUE)
		return 1;

	do
	{
		/* don't list "." and ".." */
		if (_tcscmp (file.cFileName, _T(".")) == 0 ||
			_tcscmp (file.cFileName, _T("..")) == 0)
			continue;

		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_tcscpy (szFullPath, szPath);
			if (szFullPath[_tcslen (szFullPath) - 1] != _T('\\'))
				_tcscat (szFullPath, _T("\\"));
			_tcscat (szFullPath, file.cFileName);
			
			if (DirList (szFullPath, szFilespec, pLine, dwFlags))
			{
				FindClose (hFile);
				return 1;
			}

			if ((dwFlags & DIR_BARE) == 0)
			{
				ConOutPrintf ("\n");
				if (IncLine (pLine, dwFlags) != 0)
					return 1;
				ConOutPrintf ("\n");
				if (IncLine (pLine, dwFlags) != 0)
					return 1;
			}

			if (DirRead (szFullPath, szFilespec, pLine, dwFlags) == 1)
			{
				FindClose (hFile);
				return 1;
			}
		}
	}
	while (FindNextFile (hFile, &file));

	if (!FindClose (hFile))
		return 1;

	return 0;
}


/*
 * do_recurse: Sets up for recursive directory listing
 */
static INT
DirRecurse (LPTSTR szPath, LPTSTR szSpec, LPINT pLine, DWORD dwFlags)
{
	if (!PrintDirectoryHeader (szPath, pLine, dwFlags))
		return 1;

	if (DirList (szPath, szSpec, pLine, dwFlags))
		return 1;

	if ((dwFlags & DIR_BARE) == 0)
	{
		ConOutPrintf (_T("\n"));
		if (IncLine (pLine, dwFlags))
			return 1;
	}

	if (DirRead (szPath, szSpec, pLine, dwFlags))
		return 1;

	if ((dwFlags & DIR_BARE) == 0)
		ConOutPrintf (_T("Total files listed:\n"));

	dwFlags &= ~DIR_RECURSE;

	if (PrintSummary (szPath, recurse_file_cnt,
					  recurse_dir_cnt, recurse_bytes, pLine, dwFlags))
		return 1;

	if ((dwFlags & DIR_BARE) == 0)
	{
		ConOutPrintf (_T("\n"));
		if (IncLine (pLine, dwFlags))
			return 1;
	}

	return 0;
}


/*
 * dir
 *
 * internal dir command
 */
INT CommandDir (LPTSTR first, LPTSTR rest)
{
	DWORD  dwFlags = DIR_NEW | DIR_FOUR;
	TCHAR  dircmd[256];
	TCHAR  szPath[MAX_PATH];
	TCHAR  szFilespec[MAX_PATH];
	LPTSTR param;
	INT    nLine = 0;


	recurse_dir_cnt = 0L;
	recurse_file_cnt = 0L;
	recurse_bytes.QuadPart = 0;

	/* read the parameters from the DIRCMD environment variable */
	if (GetEnvironmentVariable (_T("DIRCMD"), dircmd, 256))
	{
		if (!DirReadParam (dircmd, &param, &dwFlags))
			return 1;
	}

	/* read the parameters */
	if (!DirReadParam (rest, &param, &dwFlags))
		return 1;

	/* default to current directory */
	if (!param)
		param = ".";

	/* parse the directory info */
	if (DirParsePathspec (param, szPath, szFilespec))
		return 1;

	if (dwFlags & DIR_RECURSE)
	{
		if (IncLine (&nLine, dwFlags))
			return 0;
		if (DirRecurse (szPath, szFilespec, &nLine, dwFlags))
			return 1;
		return 0;
	}

	/* print the header */
	if (!PrintDirectoryHeader (szPath, &nLine, dwFlags))
		return 1;

	if (DirList (szPath, szFilespec, &nLine, dwFlags))
		return 1;

	return 0;
}

#endif

/* EOF */