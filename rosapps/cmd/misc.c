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
 *    18-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Changed split() to accept quoted arguments.
 *        Removed parse_firstarg().
 *
 *    23-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed an ugly bug in split(). In rare cases (last character
 *        of the string is a space) it ignored the NULL character and
 *        tried to add the following to the argument list.
 *
 *    28-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        FileGetString() seems to be working now.
 *
 *    06-Nov-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added PagePrompt() and FilePrompt().
 */

#include "config.h"

#include <windows.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "cmd.h"


/*
 * get a character out-of-band and honor Ctrl-Break characters
 */
TCHAR cgetchar (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD irBuffer;
	DWORD  dwRead;

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

#ifndef _UNICODE
	return irBuffer.Event.KeyEvent.uChar.AsciiChar;
#else
	return irBuffer.Event.KeyEvent.uChar.UnicodeChar;
#endif /* _UNICODE */
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
			while (!_tcschr ("YNA\3", c = _totupper (cgetchar())) || !c);

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
static BOOL add_entry (LPINT ac, LPTSTR **arg, LPTSTR entry)
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

static BOOL expand (LPINT ac, LPTSTR **arg, LPTSTR pattern)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindData;
	BOOL ok;

	hFind = FindFirstFile (pattern, &FindData);
	if (INVALID_HANDLE_VALUE != hFind)
	{
		do
		{
			ok = add_entry(ac, arg, FindData.cFileName);
		} while (FindNextFile (hFind, &FindData) && ok);
		FindClose (hFind);
	}
	else
	{
		ok = add_entry(ac, arg, pattern);
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


LPTSTR stpcpy (LPTSTR dest, LPTSTR src)
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

BOOL IsValidFileName (LPCTSTR pszPath)
{
	return (GetFileAttributes (pszPath) != 0xFFFFFFFF);
}


BOOL IsValidDirectory (LPCTSTR pszPath)
{
	return (GetFileAttributes (pszPath) & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL FileGetString (HANDLE hFile, LPTSTR lpBuffer, INT nBufferLength)
{
	LPTSTR lpString;
	TCHAR  ch;
	DWORD  dwRead;

	lpString = lpBuffer;

	while ((--nBufferLength >  0) &&
		   ReadFile(hFile, &ch, 1, &dwRead, NULL) && dwRead)
	{
		if (ch == _T('\r'))
		{
			/* overread '\n' */
			ReadFile (hFile, &ch, 1, &dwRead, NULL);
			break;
		}
		*lpString++ = ch;
	}

	if (!dwRead && lpString == lpBuffer)
		return FALSE;

	*lpString++ = _T('\0');

	return TRUE;
}

#ifndef __REACTOS__
/*
 * GetConsoleWindow - returns the handle to the current console window
 */
HWND GetConsoleWindow (VOID)
{
	TCHAR original[256];
	TCHAR temp[256];
	HWND h=0;

	if(h)
		return h;

	GetConsoleTitle (original, sizeof(original));

	_tcscpy (temp, original);
	_tcscat (temp, _T("-xxx   "));

	if (FindWindow (0, temp) == NULL )
	{
		SetConsoleTitle (temp);
		Sleep (0);

		while(!(h = FindWindow (0, temp)))
			;

		SetConsoleTitle (original);
	}

	return h;
}
#endif


INT PagePrompt (VOID)
{
	INPUT_RECORD ir;

	ConOutPrintf ("Press a key to continue...\n");

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
	    ((ir.Event.KeyEvent.wVirtualKeyCode == 'C') &&
	     (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
		return PROMPT_BREAK;

	return PROMPT_YES;
}


INT FilePromptYN (LPTSTR szFormat, ...)
{
        TCHAR szOut[512];
	va_list arg_ptr;
//        TCHAR cKey = 0;
//        LPTSTR szKeys = _T("yna");

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

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
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
        TCHAR szOut[512];
	va_list arg_ptr;
//        TCHAR cKey = 0;
//        LPTSTR szKeys = _T("yna");

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

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
		return PROMPT_NO;
	if (*p == _T('A'))
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
	    ((ir.Event.KeyEvent.wVirtualKeyCode == 'C') &&
	     (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
		return PROMPT_BREAK;

	return PROMPT_YES;
#endif
}

/* EOF */
