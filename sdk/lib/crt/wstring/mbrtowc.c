/*
 * msvcrt.dll mbcs functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
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

#include <precomp.h>
#include <locale.h>

/*********************************************************************
 *              mbrtowc(MSVCRT.@)
 */
size_t CDECL mbrtowc(wchar_t *dst, const char *str,
        size_t n, mbstate_t *state)
{
    mbstate_t s = (state ? *state : 0);
    char tmpstr[2];
    int len = 0;

    if(dst)
        *dst = 0;

    if(!n || !str || !*str)
        return 0;

    if(___mb_cur_max_func() == 1) {
        tmpstr[len++] = *str;
    }else if(!s && isleadbyte((unsigned char)*str)) {
        if(n == 1) {
            s = (unsigned char)*str;
            len = -2;
        }else {
            tmpstr[0] = str[0];
            tmpstr[1] = str[1];
            len = 2;
        }
    }else if(!s) {
        tmpstr[len++] = *str;
    }else {
        tmpstr[0] = s;
        tmpstr[1] = *str;
        len = 2;
        s = 0;
    }

    if(len > 0) {
        if(!MultiByteToWideChar(___lc_codepage_func(), 0, tmpstr, len, dst, dst ? 1 : 0))
            len = -1;
    }

    if(state)
        *state = s;
    return len;
}
