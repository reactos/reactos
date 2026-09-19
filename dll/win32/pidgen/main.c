/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(pidgen);

int WINAPI PIDGenSimpA(LPCSTR str, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8)
{
    FIXME("%s,%d,%d,%d,%d,%d,%d,%d,%d\n", debugstr_a(str), p1, p2, p3, p4, p5, p6, p7, p8);

    return 0;
}

int WINAPI PIDGenSimpW(const WCHAR* str, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8)
{
    FIXME("%s,%d,%d,%d,%d,%d,%d,%d,%d\n", debugstr_w(str), p1, p2, p3, p4, p5, p6, p7, p8);

    return 0;
}
