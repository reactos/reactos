/*
 * PROJECT:         ReactOS CRT library
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            lib/sdk/crt/string/string.c
 * PURPOSE:         string CRT functions
 * PROGRAMMERS:     Wine team
 *                  Ported to ReactOS by Christoph von Wittich (christoph_vw@reactos.org)
 */

/*
 * msvcrt.dll string functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
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


/*********************************************************************
 *      strcat_s (MSVCRT.@)
 */
int CDECL strcat_s( char* dst, size_t elem, const char* src )
{
    size_t i, j;
    if(!dst) return EINVAL;
    if(elem == 0) return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if(dst[i] == '\0')
        {
            for(j = 0; (j + i) < elem; j++)
            {
                if((dst[j + i] = src[j]) == '\0') return 0;
            }
        }
    }
    /* Set the first element to 0, not the first element after the skipped part */
    dst[0] = '\0';
    return ERANGE;
}

/*********************************************************************
 *      strcpy_s (MSVCRT.@)
 */
int CDECL strcpy_s( char* dst, size_t elem, const char* src )
{
    size_t i;
    if(!elem) return EINVAL;
    if(!dst) return EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }

    for(i = 0; i < elem; i++)
    {
        if((dst[i] = src[i]) == '\0') return 0;
    }
    dst[0] = '\0';
    return ERANGE;
}

