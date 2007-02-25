/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/ftime.c
 * PURPOSE:     Deprecated BSD library call
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              07/15/03 -- Created
 */

#include <precomp.h>
#include <sys/timeb.h>

/* ftime (3) -- Obsolete BSD library function included in the SUS for copat.
 * Also present in msvcrt.dll as _ftime
 * See: http://www.opengroup.org/onlinepubs/007904957/functions/ftime.html */
/*
 * @implemented
 */
void _ftime( struct _timeb *tm )
{
  int ret = 0;
  SYSTEMTIME syst;

  GetSystemTime( &syst );

  if( ret == 0 ) {
    time( &tm->time );
    tm->millitm = syst.wMilliseconds;
//    tm->_timezone = 0; /* According to the open group, timezone and dstflag */
    tm->dstflag = 0;  /* exist for compatibility, but are unspecified. */
  }
}
