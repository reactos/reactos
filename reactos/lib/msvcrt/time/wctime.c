/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* This file has been modified by DJ Delorie.  These modifications are
** Copyright (C) 1995 DJ Delorie, 24 Kirsten Ave, Rochester NH,
** 03867-2954, USA.
*/

/*
 * Copyright (c) 1987, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <msvcrt/fcntl.h>
#include <msvcrt/time.h>
#include <windows.h>
#include "tzfile.h"


wchar_t* _wasctime(const struct tm* timeptr)
{
#ifdef __GNUC__
  static const wchar_t wday_name[DAYSPERWEEK][3] = {
    L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
  };
  static const wchar_t mon_name[MONSPERYEAR][3] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };
#else
  static const wchar_t wday_name[DAYSPERWEEK][4] = {
    L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
  };
  static const wchar_t mon_name[MONSPERYEAR][4] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };
#endif
  static wchar_t result[26];

  (void)swprintf(result, L"%.3s %.3s%3d %02d:%02d:%02d %d\n",
         wday_name[timeptr->tm_wday],
         mon_name[timeptr->tm_mon],
         timeptr->tm_mday, timeptr->tm_hour,
         timeptr->tm_min, timeptr->tm_sec,
         TM_YEAR_BASE + timeptr->tm_year);
  return result;
}


wchar_t* _wctime(const time_t* const timep)
{
    return _wasctime(localtime(timep));
}
