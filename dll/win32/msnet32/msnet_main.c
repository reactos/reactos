/*
 * Microsoft Network API implementation
 *
 * Copyright 2003 Rein Klazes
 * Copyright 2006 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msnet);


/***********************************************************************
 *		@  (MSNET32.57)
 */
LONG WINAPI MSNET32_57( LONG a1, LONG a2, LPVOID a3, LONG a4, LPVOID a5 )
{
    FIXME("(0x%04lx 0x%04lx %p 0x%04lx %p): stub\n", a1, a2, a3, a4, a5);
    return -1; /* FAILURE */
}
