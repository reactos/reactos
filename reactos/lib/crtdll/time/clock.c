/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/time/clock.c
 * PURPOSE:     Get elapsed time
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <windows.h>
#include <time.h>

// should be replace by a call to RtltimeToSecondsSince70
time_t FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder ); 

clock_t clock ( void )
{
	FILETIME  CreationTime;
	FILETIME  ExitTime;
	FILETIME  KernelTime;
	FILETIME  UserTime;
	
	FILETIME  SystemTime;
	DWORD Remainder;

	if ( !GetProcessTimes(-1,&CreationTime,&ExitTime,&KernelTime,&UserTime ) )
		return -1;

	if ( !GetSystemTimeAsFileTime(&SystemTime) )
		return -1;

	return FileTimeToUnixTime( &SystemTime,&Remainder ) - FileTimeToUnixTime( &CreationTime,&Remainder ); 
}