/*
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
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_DIR
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <direct.h>

#include "cmd.h"


/* useful macro */
#define MEM_ERR error_out_of_memory(); return 1;


/* flag definitions */
/* Changed hex to decimal, hex wouldn't work
 * if > 8, Rob Lake 06/17/98.
 */
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
long recurse_dir_cnt;
long recurse_file_cnt;
ULARGE_INTEGER recurse_bytes;


/*
 * help
 *
 * displays help screen for dir
 * Rob Lake
 */
static VOID Help (VOID)
{
#if 1
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
#endif
#if 0
	InitializePageOut ();
	LinePageOut (_T("Displays a list of files and subdirectories in a directory."));
	LinePageOut (_T(""));
	LinePageOut (_T("DIR [drive:][path][filename] [/A] [/B] [/L] [/N] [/S] [/P] [/W] [/4]"));
	LinePageOut (_T(""));
	LinePageOut (_T("  [drive:][path][filename]"));
	LinePageOut (_T("              Specifies drive, directory, and/or files to list."));
	LinePageOut (_T(""));
	LinePageOut (_T("  /A          Displays files with HIDDEN SYSTEM attributes"));
	LinePageOut (_T("              default is ARCHIVE and READ ONLY"));
	LinePageOut (_T("  /B          Uses bare format (no heading information or summary)."));
	LinePageOut (_T("  /L          Uses lowercase."));
	LinePageOut (_T("  /N          New long list format where filenames are on the far right."));
	LinePageOut (_T("  /S          Displays files in specified directory and all subdirectories"));
	LinePageOut (_T("  /P          Pauses after each screen full"));
	LinePageOut (_T("  /W          Prints in wide format"));
	LinePageOut (_T("  /4          Display four digit years."));
	LinePageOut (_T(""));
	LinePageOut (_T("Switches may be present in the DIRCMD environment variable.  Use"));
	LinePageOut (_T("of the - (hyphen) can turn off defined swtiches.  Ex. /-W would"));
	LinePageOut (_T("turn off printing in wide format."));
	TerminatePageOut ();
#endif
}


/*
 * dir_read_param
 *
 * read the parameters from the command line
 */
static BOOL
DirReadParam (char *line, char **param, LPDWORD lpFlags)
{
	int slash = 0;

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
				else if (_toupper (*line) == _T('P'))
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
 * extend_file
 *
 * extend the filespec, possibly adding wildcards
 */
void extend_file (char **file)
{
	LPTSTR tmp;

	if (!*file)
		return;

	/* if no file spec, change to "*.*" */
	if (!**file)
	{
		free (*file);
		*file = _tcsdup (_T("*.*"));
		return;
	}

	/* if starts with . add * in front */
	if (**file == _T('.'))
	{
		tmp = malloc ((_tcslen (*file) + 2) * sizeof(TCHAR));
		if (tmp)
		{
			*tmp = _T('*');
			_tcscpy (&tmp[1], *file);
		}
		free (*file);
		*file = tmp;
		return;
	}

	/* if no . add .* */
	if (!_tcschr (*file, _T('.')))
	{
		tmp = malloc ((_tcslen (*file) + 3) * sizeof(TCHAR));
		if (tmp)
		{
			_tcscpy (tmp, *file);
			_tcscat (tmp, _T(".*"));
		}
		free (*file);
		*file = tmp;
		return;
	}
}


/*
 * dir_parse_pathspec
 *
 * split the pathspec into drive, directory, and filespec
 */
static int
DirParsePathspec (char *pathspec, int *drive, char **dir, char **file)
{
	TCHAR  orig_dir[MAX_PATH];
	LPTSTR start;
	LPTSTR tmp;
	INT    i;
	INT    wildcards = 0;


	/* get the drive and change to it */
	if (pathspec[1] == _T(':'))
	{
		*drive = _totupper (pathspec[0]) - _T('@');
		start = pathspec + 2;
		_chdrive (*drive);
	}
	else
	{
		*drive = _getdrive ();
		start = pathspec;
	}

	GetCurrentDirectory (MAX_PATH, orig_dir);

	/* check for wildcards */
	for (i = 0; pathspec[i]; i++)
		if (pathspec[i] == _T('*') || pathspec[i] == _T('?'))
			wildcards = 1;

	/* check if this spec is a directory */
	if (!wildcards)
	{
		if (chdir(pathspec) == 0)
		{
			*file = _tcsdup (_T("*.*"));
			if (!*file)
			{
				MEM_ERR
			}

			tmp = getcwd (NULL, MAX_PATH);
			if (!tmp)
			{
				free (*file);
				chdir (orig_dir);
				MEM_ERR
			}

			*dir = _tcsdup (&tmp[2]);
			free (tmp);
			if (!*dir)
			{
				free (*file);
				chdir (orig_dir);
				MEM_ERR
			}

			chdir (orig_dir);
			return 0;
		}
	}

	/* find the file spec */
	tmp = _tcsrchr (start, _T('\\'));

	/* if no path is specified */
	if (!tmp)
	{
		*file = _tcsdup (start);
		extend_file (file);
		if (!*file)
		{
			MEM_ERR
		}

		tmp = getcwd (NULL, _MAX_PATH);
		if (!tmp)
		{
			free (*file);
			chdir (orig_dir);
			MEM_ERR
		}
		*dir = _tcsdup (&tmp[2]);
		free(tmp);
		if (!*dir)
		{
			free (*file);
			chdir (orig_dir);
			MEM_ERR
		}

		return 0;
	}

	/* get the filename */
	*file = _tcsdup (tmp + 1);
	extend_file (file);
	if (!*file)
	{
		MEM_ERR
	}

	*tmp = 0;

	/* change to this directory and get its full name */
	if (chdir (start) < 0)
	{
		error_path_not_found ();
		*tmp = _T('\\');
		free (*file);
		chdir (orig_dir);
		return 1;
	}

	tmp = getcwd (NULL, _MAX_PATH);
	if (!tmp)
	{
		free (*file);
		MEM_ERR
	}
	*dir = _tcsdup (&tmp[2]);
	free(tmp);
	if (!*dir)
	{
		free(*file);
		MEM_ERR
	}

	*tmp = _T('\\');

	chdir(orig_dir);
	return 0;
}


/*
 * pause
 *
 * pause until a key is pressed
 */
static INT
Pause (VOID)
{
	cmd_pause ("", "");

	return 0;
}


/*
 * incline
 *
 * increment our line if paginating, display message at end of screen
 */
static INT
incline (int *line, DWORD dwFlags)
{
	if (!(dwFlags & DIR_PAGE))
		return 0;

	(*line)++;

	if (*line >= (int)maxy - 2)
	{
		*line = 0;
		return Pause ();
	}

	return 0;
}


/*
 * PrintDirectoryHeader
 *
 * print the header for the dir command
 */
static BOOL
PrintDirectoryHeader (int drive, int *line, DWORD dwFlags)
{
	TCHAR szRootName[] = _T("A:\\");
	TCHAR szVolName[80];
	DWORD dwSerialNr;

	if (dwFlags & DIR_BARE)
		return TRUE;

	/* get the media ID of the drive */
	szRootName[0] = drive + _T('@');
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

	if (incline (line, dwFlags) != 0)
		return FALSE;

	/* print the volume serial number if the return was successful */
	ConOutPrintf (_T(" Volume Serial Number is %04X-%04X\n"),
				  HIWORD(dwSerialNr), LOWORD(dwSerialNr));
	if (incline (line, dwFlags) != 0)
		return FALSE;

	return TRUE;
}


/*
 * convert
 *
 * insert commas into a number
 */
static INT
ConvertLong (LONG num, LPTSTR des, INT len)
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
			ConOutPrintf ("%.2d%c%.2d%c%d", dt->wMonth, cDateSeparator, dt->wDay, cDateSeparator, wYear);
			break;

		case 1: /* ddmmyy */
			ConOutPrintf ("%.2d%c%.2d%c%d",
					dt->wDay, cDateSeparator, dt->wMonth, cDateSeparator, wYear);
			break;

		case 2: /* yymmdd */
			ConOutPrintf ("%d%c%.2d%c%.2d",
					wYear, cDateSeparator, dt->wMonth, cDateSeparator, dt->wDay);
			break;
	}

	switch (nTimeFormat)
	{
		case 0: /* 12 hour format */
		default:
			ConOutPrintf ("  %2d%c%.2u%c",
					(dt->wHour == 0 ? 12 : (dt->wHour <= 12 ? dt->wHour : dt->wHour - 12)),
					cTimeSeparator,
					 dt->wMinute, (dt->wHour <= 11 ? 'a' : 'p'));
			break;

		case 1: /* 24 hour format */
			ConOutPrintf ("  %2d%c%.2u",
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
PrintSummary (int drive, long files, long dirs, ULARGE_INTEGER bytes,
              DWORD flags, int *line)
{
	TCHAR buffer[64];

	if (flags & DIR_BARE)
		return 0;

	/* print number of files and bytes */
	ConvertLong (files, buffer, sizeof(buffer));
	ConOutPrintf (_T("          %6s File%c"),
				  buffer, files == 1 ? _T(' ') : _T('s'));

	ConvertULargeInteger (bytes, buffer, sizeof(buffer));
	ConOutPrintf (_T("  %15s byte%c\n"),
				  buffer, bytes.QuadPart == 1 ? _T(' ') : _T('s'));

	if (incline (line, flags) != 0)
		return 1;

	/* print number of dirs and bytes free */
	ConvertLong (dirs, buffer, sizeof(buffer));
	ConOutPrintf (_T("          %6s Dir%c"),
				  buffer, files == 1 ? _T(' ') : _T('s'));


	if (!(flags & DIR_RECURSE))
	{
		ULARGE_INTEGER uliFree;
		TCHAR szRoot[] = _T("A:\\");
		DWORD dwSecPerCl;
		DWORD dwBytPerSec;
		DWORD dwFreeCl;
		DWORD dwTotCl;

		szRoot[0] = drive + _T('@');
		GetDiskFreeSpace (szRoot, &dwSecPerCl, &dwBytPerSec, &dwFreeCl, &dwTotCl);
		uliFree.QuadPart = dwSecPerCl * dwBytPerSec * dwFreeCl;
		ConvertULargeInteger (uliFree, buffer, sizeof(buffer));
		ConOutPrintf (_T("   %15s bytes free\n"), buffer);
	}

	if (incline (line, flags) != 0)
		return 1;

	return 0;
}


/*
 * dir_list
 *
 * list the files in the directory
 */
static int
dir_list (int drive, char *directory, char *filespec, int *line,
          DWORD flags)
{
  char pathspec[_MAX_PATH],
   *ext,
    buffer[32];
  WIN32_FIND_DATA file;
  ULARGE_INTEGER bytecount;
  long filecount = 0,
    dircount = 0;
  int count;
  FILETIME   ft;
  SYSTEMTIME dt;
  HANDLE hFile;

  bytecount.QuadPart = 0;

	if (directory[strlen(directory) - 1] == '\\')
		wsprintf(pathspec, "%c:%s%s", drive + '@', directory, filespec);
	else
		wsprintf(pathspec, "%c:%s\\%s", drive + '@', directory, filespec);

	hFile = FindFirstFile (pathspec, &file);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		/* Don't want to print anything if scanning recursively
		 * for a file. RL
		 */
		if ((flags & DIR_RECURSE) == 0)
		{
			error_file_not_found();
			incline(line, flags);
			FindClose (hFile);
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
  if ((flags & DIR_BARE) == 0)
  {
    ConOutPrintf (" Directory of %c:%s\n", drive + '@', directory);
    if (incline(line, flags) != 0)
      return 1;
    ConOutPrintf ("\n");
    if (incline(line, flags) != 0)
      return 1;
  }

  /* For counting columns of output */
  count = 0;

  do
  {
	/* next file, if user doesn't want all files */
	if (!(flags & DIR_ALL) &&
		((file.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
		 (file.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)))
		continue;

	/* begin Rob Lake */
	if (flags & DIR_LWR)
	{
		strlwr(file.cAlternateFileName);
	}

    if (flags & DIR_WIDE && (flags & DIR_BARE) == 0)
    {
      if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
		if (file.cAlternateFileName[0] == '\0')
          wsprintf (buffer, _T("[%s]"), file.cFileName);
		else
          wsprintf (buffer, _T("[%s]"), file.cAlternateFileName);
        dircount++;
      }
      else
      {
		if (file.cAlternateFileName[0] == '\0')
          wsprintf (buffer, _T("%s"), file.cFileName);
		else
          wsprintf (buffer, _T("%s"), file.cAlternateFileName);
        filecount++;
      }
      ConOutPrintf (_T("%-15s"), buffer);
      count++;
      if (count == 5)
      {
        /* output 5 columns */
        ConOutPrintf ("\n");
        if (incline(line, flags) != 0)
          return 1;
        count = 0;
      }

	  /* FIXME: this is buggy - now overflow check */
      bytecount.LowPart += file.nFileSizeLow;
      bytecount.HighPart += file.nFileSizeHigh;

      /* next block 06/17/98 */
    }
    else if (flags & DIR_BARE)
    {
      if (strcmp(file.cFileName, ".") == 0 ||
		  strcmp(file.cFileName, "..") == 0)
        continue;

      if (flags & DIR_RECURSE)
      {
        TCHAR dir[MAX_PATH];
        wsprintf (dir, _T("%c:%s\\"), drive + _T('@'), directory);
        if (flags & DIR_LWR)
          strlwr(dir);
        ConOutPrintf (dir);
      }
      ConOutPrintf (_T("%-13s\n"), file.cFileName);
      if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        dircount++;
      else
        filecount++;
      if (incline(line, flags) != 0)
        return 1;

      /* FIXME: this is buggy - no overflow check */
      bytecount.LowPart += file.nFileSizeLow;
      bytecount.HighPart += file.nFileSizeHigh;
    }
    else
    {
    /* end Rob Lake */
	  if (flags & DIR_NEW)
	  {
		/* print file date and time */
		if (FileTimeToLocalFileTime (&file.ftLastWriteTime, &ft))
		{
		  FileTimeToSystemTime (&ft, &dt);
		  PrintFileDateTime (&dt, flags);
		}

		/* print file size */
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
		  ConOutPrintf ("         <DIR>         ");
		  dircount++;
		}
		else
		{
			ULARGE_INTEGER uliSize;

			uliSize.LowPart = file.nFileSizeLow;
			uliSize.HighPart = file.nFileSizeHigh;

			ConvertULargeInteger (uliSize, buffer, sizeof(buffer));
			ConOutPrintf (_T("   %20s"), buffer);

			bytecount.QuadPart += uliSize.QuadPart;
			filecount++;
		}

		/* print long filename */
		ConOutPrintf (" %s\n", file.cFileName);
	  }
	  else
	  {
        if (file.cFileName[0] == '.')
          ConOutPrintf ("%-13s ", file.cFileName);
	    else if (file.cAlternateFileName[0] == '\0')
		{
          char szShortName[13];
		  strncpy (szShortName, file.cFileName, 13);
		  ext = strchr(szShortName, '.');
          if (!ext)
            ext = "";
          else
            *ext++ = 0;
          ConOutPrintf ("%-8s %-3s  ", szShortName, ext);
		}
        else
		{
		  ext = strchr(file.cAlternateFileName, '.');
          if (!ext)
            ext = "";
          else
            *ext++ = 0;
          ConOutPrintf ("%-8s %-3s  ", file.cAlternateFileName, ext);
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

			uliSize.LowPart = file.nFileSizeLow;
			uliSize.HighPart = file.nFileSizeHigh;

			ConvertULargeInteger (uliSize, buffer, sizeof(buffer));
			ConOutPrintf (_T("   %10s "), buffer);

			bytecount.QuadPart += uliSize.QuadPart;
			filecount++;
		}

		/* print file date and time */
		if (FileTimeToLocalFileTime (&file.ftLastWriteTime, &ft))
		{
		  FileTimeToSystemTime (&ft, &dt);
		  PrintFileDateTime (&dt, flags);
		}

		/* print long filename */
		ConOutPrintf (" %s\n", file.cFileName);
	  }

      if (incline(line, flags) != 0)
        return 1;
    }
  }
  while (FindNextFile (hFile, &file));

  FindClose (hFile);

	/* Rob Lake, need to make clean output */
	/* JPP 07/08/1998 added check for count != 0 */
	if ((flags & DIR_WIDE) && (count != 0))
	{
		ConOutPrintf ("\n");
		if (incline(line, flags) != 0)
			return 1;
	}

	if (filecount || dircount)
	{
		recurse_dir_cnt += dircount;
		recurse_file_cnt += filecount;
		recurse_bytes.QuadPart += bytecount.QuadPart;

		/* The code that was here is now in print_summary */
		if (PrintSummary (drive, filecount, dircount,
						  bytecount, flags, line) != 0)
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
static int
Read_Dir (int drive, char *parent, char *filespec, int *lines,
          DWORD flags)
{
  char fullpath[_MAX_PATH];
  WIN32_FIND_DATA file;
  HANDLE hFile;

  strcpy (fullpath, parent);
  strcat (fullpath, "\\");
  strcat (fullpath, filespec);

  hFile = FindFirstFile (fullpath, &file);
  if (hFile == INVALID_HANDLE_VALUE)
    return 1;

  do
  {
	/* don't list "." and ".." */
    if (strcmp (file.cFileName, ".") == 0 ||
        strcmp (file.cFileName, "..") == 0)
      continue;

	if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (dir_list(drive, file.cFileName, filespec, lines, flags) != 0)
      {
        FindClose (hFile);
        return 1;
      }
      if ((flags & DIR_BARE) == 0)
      {
        ConOutPrintf ("\n");
        if (incline(lines, flags) != 0)
          return 1;
      }
      if (Read_Dir(drive, file.cFileName, filespec, lines, flags) == 1)
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
static int
do_recurse(int drive, char *directory, char *filespec,
           int *line, DWORD flags)
{
  char cur_dir[_MAX_DIR];

  recurse_dir_cnt = recurse_file_cnt = 0L;
  recurse_bytes.QuadPart = 0;

  _chdrive (drive);
  getcwd(cur_dir, sizeof(cur_dir));

  if (chdir(directory) == -1)
    return 1;

	if (!PrintDirectoryHeader (drive, line, flags))
		return 1;

  if (dir_list(drive, directory, filespec, line, flags) != 0)
    return 1;
  if ((flags & DIR_BARE) == 0)
  {
    ConOutPrintf ("\n");
    if (incline(line, flags) != 0)
      return 1;
  }
  if (Read_Dir(drive, directory, filespec, line, flags) != 0)
    return 1;
  if ((flags & DIR_BARE) == 0)
    ConOutPrintf ("Total files listed:\n");
  flags &= ~DIR_RECURSE;

	if (PrintSummary (drive, recurse_file_cnt,
					  recurse_dir_cnt, recurse_bytes, flags, line) != 0)
		return 1;

	if ((flags & DIR_BARE) == 0)
	{
		ConOutPrintf ("\n");
		if (incline(line, flags) != 0)
			return 1;
	}
	chdir(cur_dir);
	return 0;
}


/*
 * dir
 *
 * internal dir command
 */
INT cmd_dir (LPTSTR first, LPTSTR rest)
{
	DWORD dwFlags = DIR_NEW | DIR_FOUR;
  char *param;
	TCHAR dircmd[256];
  int line = 0;
  int drive,
    orig_drive;
  char *directory,
   *filespec,
    orig_dir[_MAX_DIR];


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

	if (strchr(param, '/'))
		param = strtok (param, "/");

	/* save the current directory info */
	orig_drive = _getdrive ();
	getcwd(orig_dir, sizeof(orig_dir));

	/* parse the directory info */
	if (DirParsePathspec (param, &drive, &directory, &filespec) != 0)
	{
		_chdrive (orig_drive);
		chdir(orig_dir);
		return 1;
	}

	if (dwFlags & DIR_RECURSE)
	{
		incline(&line, dwFlags);
		if (do_recurse (drive, directory, filespec, &line, dwFlags) != 0)
			return 1;
		_chdrive (orig_drive);
		chdir (orig_dir);
		return 0;
	}

	/* print the header */
	if (!PrintDirectoryHeader (drive, &line, dwFlags))
		return 1;

	chdir (orig_dir);
	_chdrive (orig_drive);

	if (dir_list (drive, directory, filespec, &line, dwFlags) != 0)
		return 1;

	return 0;
}

#endif
