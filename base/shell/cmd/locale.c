/*
 *  LOCALE.C - locale handling.
 *
 *
 *  History:
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Started.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode safe!
 */

#include <precomp.h>
#include "resource.h"


TCHAR cDateSeparator;
TCHAR cTimeSeparator;
TCHAR cThousandSeparator;
TCHAR cDecimalSeparator;
INT   nDateFormat;
INT   nTimeFormat;
INT   nNumberGroups;


VOID InitLocale (VOID)
{
	TCHAR szBuffer[256];

	/* date settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDATE, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
	cDateSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER, (LPTSTR)&nDateFormat, sizeof(nDateFormat) / sizeof(TCHAR));

	/* time settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STIME, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
	cTimeSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_ITIME | LOCALE_RETURN_NUMBER, (LPTSTR)&nTimeFormat, sizeof(nTimeFormat) / sizeof(TCHAR));

	/* number settings */
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
	cThousandSeparator = szBuffer[0];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
	cDecimalSeparator  = szBuffer[0];
        GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
        nNumberGroups = _ttoi(szBuffer);
#if 0
	/* days of week */
	for (i = 0; i < 7; i++)
	{
		GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + i, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
		_tcscpy (aszDayNames[(i+1)%7], szBuffer); /* little hack */
	}
#endif
}


VOID PrintDate (VOID)
{
  TCHAR szDateDay[32];
	TCHAR szDate[32];

  GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, _T("ddd"), szDateDay, sizeof (szDateDay));
               
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL,szDate, sizeof (szDate));
	ConOutPrintf(_T("%s %s"),szDateDay, szDate);
}


VOID PrintTime (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];	
        SYSTEMTIME t;
        GetLocalTime(&t); 
  
	LoadString(CMD_ModuleHandle, STRING_LOCALE_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(_T("%s: %02d%c%02d%c%02d%c%02d\n"), szMsg,  t.wHour, cTimeSeparator,
		             t.wMinute , cTimeSeparator,
		             t.wSecond , cDecimalSeparator, t.wMilliseconds );
}
