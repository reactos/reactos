/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */


#include <time.h>
#include <windows.h>


time_t
time(time_t *t)
{
	SYSTEMTIME  SystemTime;
	GetLocalTime(&SystemTime);
		
}
