/*
 *  DATE.C - date internal command.
 *
 *
 *  History:
 *
 *    08 Jul 1998 (John P. Price)
 *        started.
 *
 *    20 Jul 1998 (John P. Price)
 *        corrected number of days for December from 30 to 31.
 *        (Thanx to Steffen Kaiser for bug report)
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    29-Jul-1998 (Rob Lake)
 *        fixed stand-alone mode.
 *        Added Pacific C compatible dos_getdate functions
 *
 *    09-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added locale support
 *
 *    23-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    04-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed date input bug.
 */

#include "config.h"

#ifdef INCLUDE_CMD_DATE

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


static WORD awMonths[2][13] =
{
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static VOID
PrintDate (VOID)
{
#ifdef __REACTOS__
	SYSTEMTIME st;

	GetLocalTime (&st);

	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			ConOutPrintf (_T("Current date is: %s %02d%c%02d%c%04d\n"),
					  aszDayNames[st.wDayOfWeek], st.wMonth, cDateSeparator, st.wDay, cDateSeparator, st.wYear);
			break;

		case 1: /* ddmmyy */
			ConOutPrintf (_T("Current date is: %s %02d%c%02d%c%04d\n"),
					  aszDayNames[st.wDayOfWeek], st.wDay, cDateSeparator, st.wMonth, cDateSeparator, st.wYear);
			break;

		case 2: /* yymmdd */
			ConOutPrintf (_T("Current date is: %s %04d%c%02d%c%02d\n"),
					  aszDayNames[st.wDayOfWeek], st.wYear, cDateSeparator, st.wMonth, cDateSeparator, st.wDay);
			break;
	}
#else
	TCHAR szDate[32];

	GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL,
				   szDate, sizeof (szDate));
	ConOutPrintf (_T("Current date is: %s\n"), szDate);
#endif
}


static VOID
PrintDateString (VOID)
{
	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			ConOutPrintf (_T("Enter new date (mm%cdd%cyyyy): "),
					cDateSeparator, cDateSeparator);
			break;

		case 1: /* ddmmyy */
			ConOutPrintf (_T("Enter new date (dd%cmm%cyyyy): "),
					  cDateSeparator, cDateSeparator);
			break;

		case 2: /* yymmdd */
			ConOutPrintf (_T("Enter new date (yyyy%cmm%cdd): "),
					  cDateSeparator, cDateSeparator);
			break;
	}
}


static BOOL
ReadNumber (LPTSTR *s, LPWORD lpwValue)
{
	if (_istdigit (**s))
	{
		while (_istdigit (**s))
		{
			*lpwValue = *lpwValue * 10 + **s - _T('0');
			(*s)++;
		}
		return TRUE;
	}
	return FALSE;
}


static BOOL
ReadSeparator (LPTSTR *s)
{
	if (**s == _T('/') || **s == _T('-') || **s == cDateSeparator)
	{
		(*s)++;
		return TRUE;
	}
	return FALSE;
}


static BOOL
ParseDate (LPTSTR s)
{
	SYSTEMTIME d;
	unsigned char leap;
	LPTSTR p = s;

	if (!*s)
		return TRUE;

	GetLocalTime (&d);

	d.wYear = 0;
	d.wDay = 0;
	d.wMonth = 0;

	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			break;

		case 1: /* ddmmyy */
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			break;

		case 2: /* yymmdd */
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			break;
	}

    /* if only entered two digits: */
	/*   assume 2000's if value less than 80 */
	/*   assume 1900's if value greater or equal 80 */
	if (d.wYear <= 99)
	{
		if (d.wYear >= 80)
			d.wYear = 1900 + d.wYear;
		else
			d.wYear = 2000 + d.wYear;
	}

	leap = (!(d.wYear % 4) && (d.wYear % 100)) || !(d.wYear % 400);

	if ((d.wMonth >= 1 && d.wMonth <= 12) &&
		(d.wDay >= 1 && d.wDay <= awMonths[leap][d.wMonth]) &&
		(d.wYear >= 1980 && d.wYear <= 2099))
	{
		SetLocalTime (&d);
		return TRUE;
	}

	return FALSE;
}


INT cmd_date (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    argc;
	INT    i;
	BOOL   bPrompt = TRUE;
	INT    nDateString = -1;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or sets the date.\n\n"
				   "DATE [/T][date]\n\n"
				   "  /T    display only\n\n"
				   "Type DATE without parameters to display the current date setting and\n"
				   "a prompt for a new one.  Press ENTER to keep the same date."));
		return 0;
	}

	/* build parameter array */
	arg = split (param, &argc);

	/* check for options */
	for (i = 0; i < argc; i++)
	{
		if (_tcsicmp (arg[i], _T("/t")) == 0)
			bPrompt = FALSE;
		if ((*arg[i] != _T('/')) && (nDateString == -1))
			nDateString = i;
	}

	if (nDateString == -1)
		PrintDate ();

	if (!bPrompt)
	{
		freep (arg);
		return 0;
	}

	if (nDateString == -1)
	{
		while (TRUE)  /* forever loop */
		{
			TCHAR s[40];

			PrintDateString ();
			ConInString (s, 40);
#ifdef _DEBUG
			DebugPrintf ("\'%s\'\n", s);
#endif
			while (*s && s[_tcslen (s) - 1] < ' ')
				s[_tcslen (s) - 1] = '\0';
			if (ParseDate (s))
			{
				freep (arg);
				return 0;
			}
			ConErrPuts ("Invalid date.");
		}
	}
	else
	{
		if (ParseDate (arg[nDateString]))
		{
			freep (arg);
			return 0;
		}
		ConErrPuts ("Invalid date.");
	}

	freep (arg);

	return 0;
}
#endif

