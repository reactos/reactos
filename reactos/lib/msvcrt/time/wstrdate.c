/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/strtime.c
 * PURPOSE:     Fills a buffer with a formatted date representation
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrt/time.h>
#include <msvcrt/stdio.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
wchar_t* _wstrdate(const wchar_t* datestr)
{
    time_t t;
    struct tm* d;
    wchar_t* dt = (wchar_t*)datestr;

    if (datestr == NULL) {
        __set_errno(EINVAL);
        return NULL;
    }
    t = time(NULL);
    d = localtime(&t);
    swprintf(dt,L"%d/%d/%d",d->tm_mday,d->tm_mon+1,d->tm_year);
    return dt;
}
