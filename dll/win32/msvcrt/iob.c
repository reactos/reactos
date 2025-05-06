/*
 * acrt function needed for compatibility with mingw
 *
 * Copyright 2019 Alexandre Julliard
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

/* this function is part of the import lib for compatibility with ucrt runtime */
#if 0
#pragma makedep implib
#endif

#include <stdio.h>
#include <wine/asm.h>

#undef __iob_func
extern FILE * __cdecl __iob_func(void);

/*********************************************************************
 *		__acrt_iob_func(UCRTBASE.@)
 */
FILE * __cdecl __acrt_iob_func(unsigned idx)
{
    return __iob_func() + idx;
}
__ASM_GLOBAL_IMPORT(__acrt_iob_func)
