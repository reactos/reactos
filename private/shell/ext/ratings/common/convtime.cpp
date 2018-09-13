/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/
/* :ts=4 */

/***	convtime.cpp - map between SYSTEM & NET time formats
 */

#include "npcommon.h"
#include <convtime.h>


void
NetToSystemDate(
DWORD time,
LPSYSTEMTIME pinfo)			// ptr for return data:
{
	UINT secs, days;
	WORD r;

	// Base the time on 1980, not 1970, to make leap year calculation
	// easier -- 1980 is a leap year, but 1970 isn't.  This code is being
	// written in 1996, so we aren't going to be dealing with dates before
	// 1980 anyway.

    time -= _70_to_80_bias;				// # of seconds since 1980
	secs = time % SECS_IN_DAY;			// seconds into day
	days = time / SECS_IN_DAY;			// days since Jan 1 1980
	pinfo->wDayOfWeek = (days + 2) % 7;	// Jan 1 1980 was a Tuesday, hence "+2"

	pinfo->wMilliseconds = 0;
	pinfo->wSecond = secs % 60;					// # of seconds
	secs /= 60;
	pinfo->wMinute = secs % 60;					// # of minutes
	pinfo->wHour = secs / 60;					// # of hours

	r = days / FOURYEARS;			// (r) = four year period past 1980
	days %= FOURYEARS;				// (days) = days into four year period
	r *= 4;							// (r) = years since 1980 (within 3)

	if (days == 31+28) {			// this many days into a 4-year period is feb 29
		//* Special case for FEB 29th
		pinfo->wDay = 29;
		pinfo->wMonth = 2;
	} else {
		if (days > 31+28)
			--days;						// compensate for leap year
		while (days >= 365) {
			++r;
			days -= 365;
		}

		for (secs = 1; days >= MonTotal[secs+1] ; ++secs)
			;
		days -= MonTotal[secs];

		pinfo->wDay = days + 1;
		pinfo->wMonth = (unsigned short) secs;
	}

	pinfo->wYear = r + 1980;
}


DWORD
SystemToNetDate(LPSYSTEMTIME pinfo)
{
    UINT days, secs;

	days = pinfo->wYear - 1980;
	days = days*365 + days/4;			// # of years in days
	days += pinfo->wDay + MonTotal[pinfo->wMonth];
	if (!(pinfo->wYear % 4)
		&& pinfo->wMonth <= 2)
		--days;						// adjust days for early in leap year

	secs = (((pinfo->wHour * 60) + pinfo->wMinute) * 60) + pinfo->wSecond;
	return days*SECS_IN_DAY + _70_to_80_bias + secs;
}


DWORD
GetCurrentNetDate(void)
{
	SYSTEMTIME st;

	GetSystemTime(&st);

	return SystemToNetDate(&st);
}


DWORD
GetLocalNetDate(void)
{
	SYSTEMTIME st;

	GetLocalTime(&st);

	return SystemToNetDate(&st);
}
