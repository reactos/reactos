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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

extern void __getmainargs(int *argc, char** *argv, char** *envp,
                          int expand_wildcards, int *new_mode);
extern void __wgetmainargs(int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                           int expand_wildcards, int *new_mode);

/*********************************************************************
 *		__getmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void MSVCRT20__getmainargs( int *argc, char** *argv, char** *envp,
                            int expand_wildcards, int new_mode )
{
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void MSVCRT20__wgetmainargs( int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                             int expand_wildcards, int new_mode )
{
    __wgetmainargs( argc, wargv, wenvp, expand_wildcards, &new_mode );
}
