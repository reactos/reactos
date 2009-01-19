/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/utime.c
 * PURPOSE:     Implementation of utime, _wutime
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <tchar.h>
#include <sys/utime.h>
#include "bitsfixup.h"

int 
_tutime(const _TCHAR* path, struct _utimbuf *t)
{
    int fd = _topen(path, _O_WRONLY | _O_BINARY);

    if (fd > 0)
    {
        int retVal = _futime(fd, t);
        _close(fd);
        return retVal;
    }
    return -1;
}
