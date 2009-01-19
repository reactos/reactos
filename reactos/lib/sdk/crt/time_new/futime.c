/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/futime.c
 * PURPOSE:     Implementation of _futime
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <time.h>
#include <sys/utime.h>
#include "bitsfixup.h"

HANDLE fdtoh(int fd);

/******************************************************************************
 * \name _futime
 * \brief Set a files modification time.
 * \param [out] ptimeb Pointer to a structure of type struct _timeb that 
 *        recieves the current time.
 * \sa http://msdn.microsoft.com/en-us/library/95e68951.aspx
 */
int
_futime(int fd, struct _utimbuf *filetime)
{
    HANDLE handle;
    FILETIME at, wt;

    handle = fdtoh(fd);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    if (!filetime)
    {
        time_t currTime;
        _time(&currTime);
        RtlSecondsSince1970ToTime(currTime, (LARGE_INTEGER *)&at);
        wt = at;
    }
    else
    {
        RtlSecondsSince1970ToTime(filetime->actime, (LARGE_INTEGER *)&at);
        if (filetime->actime == filetime->modtime)
        {
            wt = at;
        }
        else
        {
            RtlSecondsSince1970ToTime(filetime->modtime, (LARGE_INTEGER *)&wt);
        }
    }

    if (!SetFileTime(handle, NULL, &at, &wt))
    {
        __set_errno(GetLastError());
        return -1 ;
    }

    return 0;
}
