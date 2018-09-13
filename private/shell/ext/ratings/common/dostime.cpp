/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/
/* :ts=4 */

/***	dostime.cpp - map between DOS & NET time formats
 */

#include "npcommon.h"
#include <convtime.h>


#define YR_MASK		0xFE00
#define LEAPYR_MASK	0x0600
#define YR_BITS		7
#define MON_MASK	0x01E0
#define MON_BITS	4
#define DAY_MASK	0x001F
#define DAY_BITS	5

#define HOUR_MASK	0xF800
#define HOUR_BITS	5
#define MIN_MASK	0x07E0
#define MIN_BITS	6
#define SEC2_MASK	0x001F
#define SEC2_BITS	5

void
NetToDosDate(
DWORD time,
dos_time *pinfo)			// ptr for return data:
{
	UINT secs, days;
	WORD r;

    time = (time - _70_to_80_bias) / 2;	// # of 2 second periods since 1980
	secs = time % SEC2S_IN_DAY;			// 2 second period into day
	days = time / SEC2S_IN_DAY;			// days since Jan 1 1980

	r = secs % 30;					// # of 2 second steps
	secs /= 30;
	r |= (secs % 60) << SEC2_BITS;	// # of minutes
        r |= (secs / 60) << (SEC2_BITS+MIN_BITS);         // # of hours
	pinfo->dt_time = r;

	r = days / FOURYEARS;			// (r) = four year period past 1980
	days %= FOURYEARS;				// (days) = days into four year period
	r *= 4;							// (r) = years since 1980 (within 3)

	if (days == 31+28) {
		//* Special case for FEB 29th
		r = (r<<(MON_BITS+DAY_BITS)) + (2<<DAY_BITS) + 29;
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
		r <<= MON_BITS;
		r += (unsigned short) secs;
		r <<= DAY_BITS;
		r += (unsigned short) days+1;
	}
	pinfo->dt_date = r;
}


DWORD
DosToNetDate(dos_time dt)
{
    UINT days, secs2;

        days = dt.dt_date >> (MON_BITS + DAY_BITS);
	days = days*365 + days/4;			// # of years in days
	days += (dt.dt_date & DAY_MASK) + MonTotal[(dt.dt_date&MON_MASK) >> DAY_BITS];
	if ((dt.dt_date&LEAPYR_MASK) == 0
				&& (dt.dt_date&MON_MASK) <= (2<<DAY_BITS))
		--days;						// adjust days for early in leap year

        secs2 = ( ((dt.dt_time&HOUR_MASK) >> (MIN_BITS+SEC2_BITS)) * 60
				+ ((dt.dt_time&MIN_MASK) >> SEC2_BITS) ) * 30
				+ (dt.dt_time&SEC2_MASK);
	return (DWORD)days*SECS_IN_DAY + _70_to_80_bias + (DWORD)secs2*2;
}
