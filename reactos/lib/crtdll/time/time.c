/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/time.c
 * PURPOSE:     Get system time
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <windows.h>
#include <time.h>

// should be replace by a call to RtltimeToSecondsSince70
// and moved to a header

time_t FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder ); 

time_t
time(time_t *t)
{
	FILETIME  SystemTime;
	DWORD Remainder;
	GetSystemTimeAsFileTime(&SystemTime);
	return FileTimeToUnixTime( &SystemTime,&Remainder ); 
}

