/*
 * Protocol-level socket functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
 * Copyright (C) 2005 Marcus Meissner
 * Copyright (C) 2006-2008 Kai Blin
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
 */

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

#define TRACE(...)

/***********************************************************************
 *      inet_addr   (ws2_32.11)
 */
u_long WINAPI inet_addr( const char *str )
{
    unsigned long a[4] = { 0 };
    const char *s = str;
    unsigned char *d;
    unsigned int i;
    u_long addr;
    char *z;

    TRACE( "str %s.\n", debugstr_a(str) );

    if (!s)
    {
        SetLastError( WSAEFAULT );
        return INADDR_NONE;
    }

    d = (unsigned char *)&addr;

    if (s[0] == ' ' && !s[1]) return 0;

    for (i = 0; i < 4; ++i)
    {
        a[i] = strtoul( s, &z, 0 );
        if (z == s || !isdigit( *s )) return INADDR_NONE;
        if (!*z || isspace(*z)) break;
        if (*z != '.') return INADDR_NONE;
        s = z + 1;
    }

    if (i == 4) return INADDR_NONE;

    switch (i)
    {
        case 0:
            a[1] = a[0] & 0xffffff;
            a[0] >>= 24;
            /* fallthrough */
        case 1:
            a[2] = a[1] & 0xffff;
            a[1] >>= 16;
            /* fallthrough */
        case 2:
            a[3] = a[2] & 0xff;
            a[2] >>= 8;
    }
    for (i = 0; i < 4; ++i)
    {
        if (a[i] > 255) return INADDR_NONE;
        d[i] = a[i];
    }
    return addr;
}
