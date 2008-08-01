/*
 * msvcrt20 implementation
 *
 * Copyright 2002 Alexandre Julliard
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

extern void CDECL __getmainargs(int *argc, char** *argv, char** *envp,
                                int expand_wildcards, int *new_mode);
extern void CDECL __wgetmainargs(int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                                 int expand_wildcards, int *new_mode);

/*********************************************************************
 *		__getmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void CDECL MSVCRT20__getmainargs( int *argc, char** *argv, char** *envp,
                                  int expand_wildcards, int new_mode )
{
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void CDECL MSVCRT20__wgetmainargs( int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                                   int expand_wildcards, int new_mode )
{
    __wgetmainargs( argc, wargv, wenvp, expand_wildcards, &new_mode );
}
