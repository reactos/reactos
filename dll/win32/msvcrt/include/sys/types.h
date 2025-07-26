/*
 * _stat() definitions
 *
 * Copyright 2000 Francois Gouget.
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
#ifndef __WINE_SYS_TYPES_H
#define __WINE_SYS_TYPES_H

#include <corecrt.h>

#ifndef _DEV_T_DEFINED
# ifdef _CRTDLL
typedef unsigned short _dev_t;
# else
typedef unsigned int   _dev_t;
# endif
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#define _INO_T_DEFINED
#endif

#ifndef _MODE_T_DEFINED
typedef unsigned short _mode_t;
#define _MODE_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef int _off_t;
#define _OFF_T_DEFINED
#endif

#ifndef _BSDTYPES_DEFINED
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int  u_int;
typedef __msvcrt_ulong u_long;
#define _BSDTYPES_DEFINED
#endif

#define dev_t _dev_t
#define ino_t _ino_t
#define mode_t _mode_t
#define off_t _off_t

#ifndef _PID_T_DEFINED
typedef int pid_t;
#define _PID_T_DEFINED
#endif

#ifndef _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif

#endif /* __WINE_SYS_TYPES_H */
