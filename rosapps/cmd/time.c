/*
 *  TIME.C - time internal command.
 *
 *
 *  History:
 *
 *    07/08/1998 (John P. Price)
 *        started.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    09-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added locale support.
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *        Added "/t" option.
 *
 *    04-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed time input bug.
 */

#include "config.h"

#ifdef INCLUDE_CMD_TIME

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


static BOOL ParseTime (LPTSTR s)
{
	SYSTEMTIME t;
	LPTSTR p = s;

	if (!*s)
		return TRUE;

	GetLocalTime (&t);
	t.wHour = 0;
	t.wMinute = 0;
	t.wSecond = 0;
	t.wMilliseconds = 0;

	// first get hour
	if (_istdigit(*p))
	{
		while (_istdigit(*p))
		{
			t.wHour = t.wHour * 10 + *p - _T('0');
			p++;
		}
	}
	else
		return FALSE;

	// get time separator
	if (*p != cTimeSeparator)
		return FALSE;
	p++;

	// now get minutes
	if (_istdigit(*p))
	{
		while (_istdigit(*p))
		{
			t.wMinute = t.wMinute * 10 + *p - _T('0');
			p++;
		}
	}
	else
		return FALSE;

	// get time separator
	if (*p != cTimeSeparator)
		return FALSE;
	p++;

	// now get seconds
	if (_istdigit(*p))
	{
		while (_istdigit(*p))
		{
			t.wSecond = t.wSecond * 10 + *p - _T('0');
			p++;
		}
	}
	else
		return FALSE;

	// get decimal separator
	if (*p == cDecimalSeparator)
	{
		p++;

		// now get hundreths
		if (_istdigit(*p))
		{
			while (_istdigit(*p))
			{
//				t.wMilliseconds = t.wMilliseconds * 10 + *p - _T('0');
				p++;
			}
//			t.wMilliseconds *= 10;
		}
	}

	/* special case: 12 hour format */
	if (nTimeFormat == 0)
	{
		if (_totupper(*s) == _T('P'))
		{
			t.wHour += 12;
		}

		if ((_totupper(*s) == _T('A')) && (t.wHour == 12))
		{
			t.wHour = 0;
		}
	}

	if (t.wHour > 23 || t.wMinute > 60 || t.wSecond > 60 || t.wMilliseconds > 999)
		return FALSE;

	SetLocalTime (&t);

	return TRUE;
}


INT cmd_time (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    argc;
	INT    i;
	BOOL   bPrompt = TRUE;
	INT    nTimeString = -1;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or sets the system time.\n\n"
					   "TIME [/T][time]\n\n"
					   "  /T    display only\n\n"
					   "Type TIME with no parameters to display the current time setting and a prompt\n"
					   "for a new one.  Press ENTER to keep the same time."));
		return 0;
	}

	/* build parameter array */
	arg = split (param, &argc);

	/* check for options */
	for (i = 0; i < argc; i++)
	{
		if (_tcsicmp (arg[i], _T("/t")) == 0)
			bPrompt = FALSE;
		if ((*arg[i] != _T('/')) && (nTimeString == -1))
			nTimeString = i;
	}

	if (nTimeString == -1)
		PrintTime ();

	if (!bPrompt)
	{
		freep (arg);
		return 0;
	}

	while (1)
	{
		if (nTimeString == -1)
		{
			TCHAR  s[40];

			ConOutPrintf (_T("Enter new time: "));

			ConInString (s, 40);

#ifdef _DEBUG
			DebugPrintf ("\'%s\'\n", s);
#endif

			while (*s && s[_tcslen (s) - 1] < _T(' '))
				s[_tcslen(s) - 1] = _T('\0');

			if (ParseTime (s))
			{
				freep (arg);
				return 0;
			}
		}
		else
		{
			if (ParseTime (arg[nTimeString]))
			{
				freep (arg);
				return 0;
			}

                        /* force input the next time around. */
                        nTimeString = -1;
		}
		ConErrPuts (_T("Invalid time."));
	}

	freep (arg);

	return 0;
}

#endif
