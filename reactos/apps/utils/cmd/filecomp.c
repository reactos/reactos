/*
 *  FILECOMP.C - handles filename completion.
 *
 *
 *  Comments:
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *       moved from command.c file
 *       made second TAB display list of filename matches
 *       made filename be lower case if last character typed is lower case
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *       Cleanup. Unicode safe!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "cmd.h"


#ifdef FEATURE_UNIX_FILENAME_COMPLETION

VOID CompleteFilename (LPTSTR str, INT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	INT   curplace = 0;
	INT   start;
	INT   count;
	BOOL  found_dot = FALSE;
	BOOL  perfectmatch = TRUE;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR maxmatch[MAX_PATH] = _T("");
	TCHAR directory[MAX_PATH];

	/* expand current file name */
	count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find front of word */
	while (count > 0 && str[count] != _T(' '))
		count--;

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	/* extract directory from word */
	_tcscpy (directory, &str[start]);
	curplace = _tcslen (directory) - 1;
	while (curplace >= 0 && directory[curplace] != _T('\\') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	_tcscpy (path, &str[start]);

	/* look for a '.' in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* find anything */
		do
		{
			/* ignore "." and ".." */
			if (!_tcscmp (file.cFileName, _T(".")) || 
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			_tcscpy (fname, file.cFileName);

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_tcscat (fname, _T("\\"));
			else
				_tcscat (fname, _T(" "));

			if (!maxmatch[0] && perfectmatch)
			{
				_tcscpy(maxmatch, fname);
			}
			else
			{
				for (count = 0; maxmatch[count] && fname[count]; count++)
				{
					if (tolower(maxmatch[count]) != tolower(fname[count]))
					{
						perfectmatch = FALSE;
						maxmatch[count] = 0;
						break;
					}
				}
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		_tcscpy (&str[start], directory);
		_tcscat (&str[start], maxmatch);

		if (!perfectmatch)
#ifdef __REACTOS__
			Beep (440, 50);
#else
			MessageBeep (-1);
#endif
	}
	else
	{
		/* no match found */
#ifdef __REACTOS__
		Beep (440, 50);
#else
		MessageBeep (-1);
#endif
	}
}


/*
 * returns 1 if at least one match, else returns 0
 */

BOOL ShowCompletionMatches (LPTSTR str, INT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	BOOL  found_dot = FALSE;
	INT   curplace = 0;
	INT   start;
	INT   count;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR directory[MAX_PATH];

	/* expand current file name */
	count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find front of word */
	while (count > 0 && str[count] != _T(' '))
		count--;

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	/* extract directory from word */
	_tcscpy (directory, &str[start]);
	curplace = _tcslen (directory) - 1;
	while (curplace >= 0 &&
		   directory[curplace] != _T('\\') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	_tcscpy (path, &str[start]);

	/* look for a . in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* find anything */
		ConOutChar (_T('\n'));
		count = 0;
		do
		{
			/* ignore . and .. */
			if (!_tcscmp (file.cFileName, _T(".")) || 
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_stprintf (fname, _T("[%s]"), file.cFileName);
			else
				_tcscpy (fname, file.cFileName);

			ConOutPrintf (_T("%-14s"), fname);
			if (++count == 5)
			{
				ConOutChar (_T('\n'));
				count = 0;
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		if (count)
			ConOutChar (_T('\n'));
	}
	else
	{
		/* no match found */
#ifdef __REACTOS__
		Beep (440, 50);
#else
		MessageBeep (-1);
#endif
		return FALSE;
	}

	return TRUE;
}
#endif

#ifdef FEATURE_4NT_FILENAME_COMPLETION

//static VOID BuildFilenameMatchList (...)

// VOID CompleteFilenameNext (LPTSTR, INT)
// VOID CompleteFilenamePrev (LPTSTR, INT)

// VOID RemoveFilenameMatchList (VOID)

#endif
