/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/time.h
 * PURPOSE:      Time declarations used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_NTOS_TIME_H
#define __INCLUDE_NTOS_TIME_H

#include <ntos/types.h>

typedef struct _SYSTEMTIME
{
   WORD wYear;
   WORD wMonth;
   WORD wDayOfWeek;
   WORD wDay;
   WORD wHour;
   WORD wMinute;
   WORD wSecond;
   WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION
{
   LONG Bias;
   WCHAR StandardName[32];
   SYSTEMTIME StandardDate;
   LONG StandardBias;
   WCHAR DaylightName[32];
   SYSTEMTIME DaylightDate;
   LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#endif /* __INCLUDE_NTOS_TIME_H */
