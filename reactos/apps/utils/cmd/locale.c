/*
 *  LOCALE.C - locale handling.
 *
 *
 *  History:
 *
 *    09-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode safe!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


TCHAR cDateSeparator;
TCHAR cTimeSeparator;
TCHAR cThousandSeparator;
TCHAR cDecimalSeparator;
INT   nDateFormat;
INT   nTimeFormat;
TCHAR aszDayNames[7][8];
INT   nNumberGroups;


VOID InitLocale (VOID)
{
#ifdef LOCALE_WINDOWS
	TCHAR szBuffer[256];
	INT i;

	/* date settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDATE, szBuffer, 256);
	CharToOem (szBuffer, szBuffer);
	cDateSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_IDATE, szBuffer, 256);
	nDateFormat = _ttoi (szBuffer);

	/* time settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STIME, szBuffer, 256);
	CharToOem (szBuffer, szBuffer);
	cTimeSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_ITIME, szBuffer, 256);
	nTimeFormat = _ttoi (szBuffer);

	/* number settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szBuffer, 256);
	CharToOem (szBuffer, szBuffer);
	cThousandSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szBuffer, 256);
	CharToOem (szBuffer, szBuffer);
	cDecimalSeparator  = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szBuffer, 256);
	nNumberGroups = _ttoi (szBuffer);

	/* days of week */
	for (i = 0; i < 7; i++)
	{
		GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + i, szBuffer, 256);
		CharToOem (szBuffer, szBuffer);
		_tcscpy (aszDayNames[(i+1)%7], szBuffer); /* little hack */
	}
#endif

#ifdef LOCALE_GERMAN
	LPTSTR names [7] = {_T("So"), _T("Mo"), _T("Di"), _T("Mi"), _T("Do"), _T("Fr"), _T("Sa")};
	INT i;

	/* date settings */
	cDateSeparator = '.';
	nDateFormat    = 1;			/* ddmmyy */

	/* time settings */
	cTimeSeparator = ':';
	nTimeFormat    = 1;			/* 24 hour */

	/* number settings */
	cThousandSeparator = '.';
	cDecimalSeparator  = ',';
	nNumberGroups      = 3;

	/* days of week */
	for (i = 0; i < 7; i++)
		_tcscpy (aszDayNames[i], names[i]);
#endif

#ifdef LOCALE_DEFAULT
	LPTSTR names [7] = {_T("Son"), _T("Mon"), _T("Tue"), _T("Wed"), _T("Thu"), _T("Fri"), _T("Sat")};
	INT i;

	/* date settings */
	cDateSeparator = '-';
	nDateFormat = 0;		/* mmddyy */

	/* time settings */
	cTimeSeparator = ':';
	nTimeFormat = 0;		/* 12 hour */

	/* number settings */
	cThousandSeparator = ',';
	cDecimalSeparator  = '.';
	nNumberGroups      = 3;

	/* days of week */
	for (i = 0; i < 7; i++)
		_tcscpy (aszDayNames[i], names[i]);
#endif
}
