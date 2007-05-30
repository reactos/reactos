/*
 *  MISC.C - misc. functions.
 *
 *
 *  History:
 *
 *    07/12/98 (Rob Lake)
 *        started
 *
 *    07/13/98 (Rob Lake)
 *        moved functions in here
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Changed split() to accept quoted arguments.
 *        Removed parse_firstarg().
 *
 *    23-Jan-1999 (Eric Kohl)
 *        Fixed an ugly bug in split(). In rare cases (last character
 *        of the string is a space) it ignored the NULL character and
 *        tried to add the following to the argument list.
 *
 *    28-Jan-1999 (Eric Kohl)
 *        FileGetString() seems to be working now.
 *
 *    06-Nov-1999 (Eric Kohl)
 *        Added PagePrompt() and FilePrompt().
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>
#include "resource.h"


/*
 * get a character out-of-band and honor Ctrl-Break characters
 */
TCHAR cgetchar (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD irBuffer;
	DWORD  dwRead;
/*
	do
	{
		ReadConsoleInput (hInput, &irBuffer, 1, &dwRead);
		if ((irBuffer.EventType == KEY_EVENT) &&
			(irBuffer.Event.KeyEvent.bKeyDown == TRUE))
		{
			if ((irBuffer.Event.KeyEvent.dwControlKeyState &
				 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &
				(irBuffer.Event.KeyEvent.wVirtualKeyCode == 'C'))
				bCtrlBreak = TRUE;

			break;
		}
	}
	while (TRUE);
*/
	do
 	{
 		ReadConsoleInput (hInput, &irBuffer, 1, &dwRead);
 		
		if (irBuffer.EventType == KEY_EVENT)
 		{
			if (irBuffer.Event.KeyEvent.dwControlKeyState &
				 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
			{
				if (irBuffer.Event.KeyEvent.wVirtualKeyCode == 'C')
				{
//					if (irBuffer.Event.KeyEvent.bKeyDown == TRUE)
//					{
						bCtrlBreak = TRUE;
						break;
//					}
				}
			}
			else if ((irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
	       (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
	       (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL))
			{
				;
			}
 
			else
			{
				break;
			}
 		}
 	}
 	while (TRUE);

#ifndef _UNICODE
	return irBuffer.Event.KeyEvent.uChar.AsciiChar;
#else
	return irBuffer.Event.KeyEvent.uChar.UnicodeChar;
#endif /* _UNICODE */
}

/*
 * Takes a path in and returns it with the correct case of the letters
 */
VOID GetPathCase( TCHAR * Path, TCHAR * OutPath)
{
	INT i = 0;
	TCHAR TempPath[MAX_PATH];  
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	_tcscpy(TempPath, _T(""));
	_tcscpy(OutPath, _T(""));

	
	for(i = 0; i < _tcslen(Path); i++)
	{
		if(Path[i] != _T('\\'))
		{
			_tcsncat(TempPath, &Path[i], 1);
			if(i != _tcslen(Path) - 1)
				continue;
		}
		/* Handle the base part of the path different.
		   Because if you put it into findfirstfile, it will
		   return your current folder */
		if(_tcslen(TempPath) == 2 && TempPath[1] == _T(':'))
		{
			_tcscat(OutPath, TempPath);
			_tcscat(OutPath, _T("\\"));
			_tcscat(TempPath, _T("\\"));
		}
		else
		{
			hFind = FindFirstFile(TempPath,&FindFileData);
			if(hFind == INVALID_HANDLE_VALUE)
			{
				_tcscpy(OutPath, Path);
				return;
			}
			_tcscat(TempPath, _T("\\"));
			_tcscat(OutPath, FindFileData.cFileName);
			_tcscat(OutPath, _T("\\"));
			CloseHandle(hFind);
		}
	}
}

/*
 * Check if Ctrl-Break was pressed during the last calls
 */

BOOL CheckCtrlBreak (INT mode)
{
	static BOOL bLeaveAll = FALSE; /* leave all batch files */
	TCHAR c;

	switch (mode)
	{
		case BREAK_OUTOFBATCH:
			bLeaveAll = 0;
			return FALSE;

		case BREAK_BATCHFILE:
			if (bLeaveAll)
				return TRUE;

			if (!bCtrlBreak)
				return FALSE;

			/* we need to be sure the string arrives on the screen! */
			do
				ConOutPuts (_T("\r\nCtrl-Break pressed.  Cancel batch file? (Yes/No/All) "));
			while (!_tcschr (_T("YNA\3"), c = _totupper (cgetchar())) || !c);

			ConOutPuts (_T("\r\n"));

			if (c == _T('N'))
				return bCtrlBreak = FALSE; /* ignore */

			/* leave all batch files */
			bLeaveAll = ((c == _T('A')) || (c == _T('\3')));
			break;

		case BREAK_INPUT:
			if (!bCtrlBreak)
				return FALSE;
			break;
	}

	/* state processed */
	bCtrlBreak = FALSE;
	return TRUE;
}

/* add new entry for new argument */
BOOL add_entry (LPINT ac, LPTSTR **arg, LPCTSTR entry)
{
	LPTSTR q;
	LPTSTR *oldarg;

	q = malloc ((_tcslen(entry) + 1) * sizeof (TCHAR));
	if (NULL == q)
	{
		return FALSE;
	}
	_tcscpy (q, entry);

	oldarg = *arg;
	*arg = realloc (oldarg, (*ac + 2) * sizeof (LPTSTR));
	if (NULL == *arg)
	{
		*arg = oldarg;
		return FALSE;
	}

	/* save new entry */
	(*arg)[*ac] = q;
	(*arg)[++(*ac)] = NULL;

	return TRUE;
}

static BOOL expand (LPINT ac, LPTSTR **arg, LPCTSTR pattern)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindData;
	BOOL ok;
	LPCTSTR pathend;
	LPTSTR dirpart, fullname;

	pathend = _tcsrchr (pattern, _T('\\'));
	if (NULL != pathend)
	{
		dirpart = malloc((pathend - pattern + 2) * sizeof(TCHAR));
		if (NULL == dirpart)
		{
			return FALSE;
		}
		memcpy(dirpart, pattern, pathend - pattern + 1);
		dirpart[pathend - pattern + 1] = _T('\0');
	}
	else
	{
		dirpart = NULL;
	}
	hFind = FindFirstFile (pattern, &FindData);
	if (INVALID_HANDLE_VALUE != hFind)
	{
		do
		{
			if (NULL != dirpart)
			{
				fullname = malloc((_tcslen(dirpart) + _tcslen(FindData.cFileName) + 1) * sizeof(TCHAR));
				if (NULL == fullname)
				{
					ok = FALSE;
				}
				else
				{
					_tcscat (_tcscpy (fullname, dirpart), FindData.cFileName);
					ok = add_entry(ac, arg, fullname);
					free (fullname);
				}
			}
			else
			{
				ok = add_entry(ac, arg, FindData.cFileName);
			}
		} while (FindNextFile (hFind, &FindData) && ok);
		FindClose (hFind);
	}
	else
	{
		ok = add_entry(ac, arg, pattern);
	}

	if (NULL != dirpart)
	{
		free (dirpart);
	}

	return ok;
}

/*
 * split - splits a line up into separate arguments, deliminators
 *         are spaces and slashes ('/').
 */

LPTSTR *split (LPTSTR s, LPINT args, BOOL expand_wildcards)
{
	LPTSTR *arg;
	LPTSTR start;
	LPTSTR q;
	INT  ac;
	INT  len;
	BOOL bQuoted = FALSE;

	arg = malloc (sizeof (LPTSTR));
	if (!arg)
		return NULL;
	*arg = NULL;

	ac = 0;
	while (*s)
	{
		/* skip leading spaces */
		while (*s && (_istspace (*s) || _istcntrl (*s)))
			++s;

		/* if quote (") then set bQuoted */
		if (*s == _T('\"'))
		{
			++s;
			bQuoted = TRUE;
		}

		start = s;

		/* the first character can be '/' */
		if (*s == _T('/'))
			++s;

		/* skip to next word delimiter or start of next option */
		if (bQuoted)
		{
			while (_istprint (*s) && (*s != _T('\"')) && (*s != _T('/')))
				++s;
		}
		else
		{
			while (_istprint (*s) && !_istspace (*s) && (*s != _T('/')))
				++s;
		}

		/* a word was found */
		if (s != start)
		{
			q = malloc (((len = s - start) + 1) * sizeof (TCHAR));
			if (!q)
			{
				return NULL;
			}
			memcpy (q, start, len * sizeof (TCHAR));
			q[len] = _T('\0');
			if (expand_wildcards && _T('/') != *start &&
			    (NULL != _tcschr(q, _T('*')) || NULL != _tcschr(q, _T('?'))))
			{
				if (! expand(&ac, &arg, q))
				{
					free (q);
					freep (arg);
					return NULL;
				}
			}
			else
			{
				if (! add_entry(&ac, &arg, q))
				{
					free (q);
					freep (arg);
					return NULL;
				}
			}
			free (q);
		}

		/* adjust string pointer if quoted (") */
		if (bQuoted)
		{
      /* Check to make sure if there is no ending "
       * we dont run over the null char */
      if(*s == _T('\0'))
      {
        break;
      }
			++s;
			bQuoted = FALSE;
		}
	}

	*args = ac;

	return arg;
}


/*
 * freep -- frees memory used for a call to split
 *
 */
VOID freep (LPTSTR *p)
{
	LPTSTR *q;

	if (!p)
		return;

	q = p;
	while (*q)
		free(*q++);

	free(p);
}


LPTSTR _stpcpy (LPTSTR dest, LPCTSTR src)
{
	_tcscpy (dest, src);
	return (dest + _tcslen (src));
}



/*
 * Checks if a path is valid (accessible)
 */

BOOL IsValidPathName (LPCTSTR pszPath)
{
	TCHAR szOldPath[MAX_PATH];
	BOOL  bResult;

	GetCurrentDirectory (MAX_PATH, szOldPath);
	bResult = SetCurrentDirectory (pszPath);

	SetCurrentDirectory (szOldPath);

	return bResult;
}


/*
 * Checks if a file exists (accessible)
 */

BOOL IsExistingFile (LPCTSTR pszPath)
{
	DWORD attr = GetFileAttributes (pszPath);
	return (attr != 0xFFFFFFFF && (! (attr & FILE_ATTRIBUTE_DIRECTORY)) );
}


BOOL IsExistingDirectory (LPCTSTR pszPath)
{
	DWORD attr = GetFileAttributes (pszPath);
	return (attr != 0xFFFFFFFF && (attr & FILE_ATTRIBUTE_DIRECTORY) );
}


BOOL FileGetString (HANDLE hFile, LPTSTR lpBuffer, INT nBufferLength)
{
	LPSTR lpString;
	CHAR ch;
	DWORD  dwRead;
	INT len;
#ifdef _UNICODE
	lpString = malloc(nBufferLength);
#else
	lpString = lpBuffer;
#endif
	len = 0;
	while ((--nBufferLength >  0) &&
		   ReadFile(hFile, &ch, 1, &dwRead, NULL) && dwRead)
	{
        lpString[len++] = ch;
        if ((ch == _T('\n')) || (ch == _T('\r')))
		{
			/* break at new line*/
			break;
		}
	}

	if (!dwRead && !len)
		return FALSE;

	lpString[len++] = _T('\0');
#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, lpString, len, lpBuffer, len);
	free(lpString);
#endif
	return TRUE;
}

INT PagePrompt (VOID)
{
	INPUT_RECORD ir;

	ConOutResPuts(STRING_MISC_HELP1);

	RemoveBreakHandler ();
	ConInDisable ();

	do
	{
		ConInKey (&ir);
	}
	while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

	AddBreakHandler ();
	ConInEnable ();

	if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
	    ((ir.Event.KeyEvent.wVirtualKeyCode == _T('C')) &&
	     (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
	{
		bCtrlBreak = TRUE;
		return PROMPT_BREAK;
	}

	return PROMPT_YES;
}


INT FilePromptYN (LPTSTR szFormat, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szOut[512];
	va_list arg_ptr;
//	TCHAR cKey = 0;
//	LPTSTR szKeys = _T("yna");

	TCHAR szIn[10];
	LPTSTR p;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	ConOutPrintf (szFormat);

	/* preliminary fix */
	ConInString (szIn, 10);
	ConOutPrintf (_T("\n"));

	_tcsupr (szIn);
	for (p = szIn; _istspace (*p); p++)
		;

	LoadString(CMD_ModuleHandle, STRING_CHOICE_OPTION, szMsg, RC_STRING_MAX_SIZE);

	if (_tcsncmp(p, &szMsg[0], 1) == 0)
		return PROMPT_YES;
	else if (_tcsncmp(p, &szMsg[1], 1) == 0)
		return PROMPT_NO;
#if 0
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif

	return PROMPT_NO;


	/* unfinished sollution */
#if 0
	RemoveBreakHandler ();
	ConInDisable ();

	do
	{
		ConInKey (&ir);
		cKey = _totlower (ir.Event.KeyEvent.uChar.AsciiChar);
		if (_tcschr (szKeys, cKey[0]) == NULL)
			cKey = 0;


	}
	while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

	AddBreakHandler ();
	ConInEnable ();

	if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
	    ((ir.Event.KeyEvent.wVirtualKeyCode == 'C') &&
	     (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
		return PROMPT_BREAK;

	return PROMPT_YES;
#endif
}


INT FilePromptYNA (LPTSTR szFormat, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szOut[512];
	va_list arg_ptr;
//	TCHAR cKey = 0;
//	LPTSTR szKeys = _T("yna");

	TCHAR szIn[10];
	LPTSTR p;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	ConOutPrintf (szFormat);

	/* preliminary fix */
	ConInString (szIn, 10);
	ConOutPrintf (_T("\n"));

	_tcsupr (szIn);
	for (p = szIn; _istspace (*p); p++)
		;

	LoadString( CMD_ModuleHandle, STRING_COPY_OPTION, szMsg, RC_STRING_MAX_SIZE);

	if (_tcsncmp(p, &szMsg[0], 1) == 0)
		return PROMPT_YES;
	else if (_tcsncmp(p, &szMsg[1], 1) == 0)
		return PROMPT_NO;
	else if (_tcsncmp(p, &szMsg[2], 1) == 0)
		return PROMPT_ALL;

#if 0
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif

	return PROMPT_NO;


/* unfinished sollution */
#if 0
	RemoveBreakHandler ();
	ConInDisable ();

	do
	{
		ConInKey (&ir);
		cKey = _totlower (ir.Event.KeyEvent.uChar.AsciiChar);
		if (_tcschr (szKeys, cKey[0]) == NULL)
			cKey = 0;
	}
	while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
	       (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

	AddBreakHandler ();
	ConInEnable ();

	if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
	    ((ir.Event.KeyEvent.wVirtualKeyCode == _T('C')) &&
	     (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
		return PROMPT_BREAK;

	return PROMPT_YES;
#endif
}

/* EOF */
