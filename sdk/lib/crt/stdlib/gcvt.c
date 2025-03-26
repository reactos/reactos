/*
 * msvcrt.dll math functions
 *
 * Copyright 2003 Alexandre Julliard <julliard@winehq.org>
 * Copyright 2010 Piotr Caban <piotr@codeweavers.com>
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
 */

#include <precomp.h>

/***********************************************************************
 *		_gcvt  (MSVCRT.@)
 */
char* CDECL _gcvt(double number, int ndigit, char* buff)
{
    if (!buff) {
        *_errno() = EINVAL;
        return NULL;
    }

    if (ndigit < 0) {
        *_errno() = ERANGE;
        return NULL;
    }

    sprintf(buff, "%.*g", ndigit, number);
    return buff;
}

/***********************************************************************
 *              _gcvt_s  (MSVCRT.@)
 */
int CDECL _gcvt_s(char* buff, size_t size, double number, int digits)
{
    int len;

    if (!buff) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if (digits < 0 || digits >= size) {
        if (size)
            buff[0] = '\0';

        *_errno() = ERANGE;
        return ERANGE;
    }

    len = _scprintf("%.*g", digits, number);
    if (len > size) {
        buff[0] = '\0';
        *_errno() = ERANGE;
        return ERANGE;
    }

    sprintf(buff, "%.*g", digits, number);
    return 0;
}
