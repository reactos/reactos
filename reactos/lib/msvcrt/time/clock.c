/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/clock.c
 * PURPOSE:     Get elapsed time
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


clock_t clock ( void )
{
  FILETIME CreationTime;
  FILETIME ExitTime;
  FILETIME KernelTime;
  FILETIME UserTime;
  DWORD Remainder;

  if (!GetProcessTimes(GetCurrentProcess(),&CreationTime,&ExitTime,&KernelTime,&UserTime))
    return -1;

  return FileTimeToUnixTime(&KernelTime,&Remainder) + FileTimeToUnixTime(&UserTime,&Remainder);
}
