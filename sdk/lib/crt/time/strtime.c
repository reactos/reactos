/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/time/strtime.c
 * PURPOSE:     Fills a buffer with a formatted time representation
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <precomp.h>

/*
 * @implemented
 */
char* _strtime(char* time)
{
   static const char format[] = "HH':'mm':'ss";

   GetTimeFormatA(LOCALE_NEUTRAL, 0, NULL, format, time, 9);

   return time;
}

int CDECL _strtime_s(char* time, size_t size)
{
    if(time && size)
        time[0] = '\0';

    if(!time) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if(size < 9) {
        *_errno() = ERANGE;
        return ERANGE;
    }

    _strtime(time);
    return 0;
}
