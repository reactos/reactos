/*
 * Porting wine comtrl32 to ReactOS comctrl32 support functions
 *
 * Copyright 2002 Robert Dickenson
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include "porting.h"


static int interlocked_mutex;
void _lwp_mutex_lock(int* interlocked_mutex) {}
void _lwp_mutex_unlock(int* interlocked_mutex) {}

long interlocked_xchg_add( long *dest, long incr )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest += incr;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

