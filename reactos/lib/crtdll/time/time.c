/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef _TM_DEFINED
struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  char *tm_zone;
  int tm_gmtoff;
};
#define _TM_DEFINED
#endif

#include <time.h>
#include <windows.h>


time_t
time(time_t *t)
{
	SYSTEMTIME  SystemTime;
	GetLocalTime(&SystemTime);
	
	
}
