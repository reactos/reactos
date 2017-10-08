/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/utime.c
 * PURPOSE:     Implementation of utime, _wutime
 * PROGRAMERS:  Wine team
 */

/*
 * msvcrt.dll file functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Eric Pouech
 * Copyright 2004 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO
 * Use the file flag hints O_SEQUENTIAL, O_RANDOM, O_SHORT_LIVED
 */

#include <precomp.h>
#include <tchar.h>
#define RC_INVOKED 1 // to prevent inline functions
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
