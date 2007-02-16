/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/strtime.c
 * PURPOSE:     Fills a buffer with a formatted time representation
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <precomp.h>

/*
 * @implemented
 */
wchar_t* _wstrtime(wchar_t* buf)
{
    time_t t;
    struct tm* d;
    wchar_t* dt = (wchar_t*)buf;

    if ( buf == NULL ) {
        __set_errno(EINVAL);
        return NULL;
    }
    t = time(NULL);
    d = localtime(&t);
    swprintf(dt,L"%d:%d:%d",d->tm_hour,d->tm_min,d->tm_sec);
    return dt;
}
