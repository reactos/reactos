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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_SYS_TYPES_H
#define __WINE_SYS_TYPES_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_DEV_T_DEFINED
typedef unsigned int   _dev_t;
#define MSVCRT_DEV_T_DEFINED
#endif

#ifndef MSVCRT_INO_T_DEFINED
typedef unsigned short _ino_t;
#define MSVCRT_INO_T_DEFINED
#endif

#ifndef MSVCRT_MODE_T_DEFINED
typedef unsigned short _mode_t;
#define MSVCRT_MODE_T_DEFINED
#endif

#ifndef MSVCRT_OFF_T_DEFINED
typedef int MSVCRT(_off_t);
#define MSVCRT_OFF_T_DEFINED
#endif

#ifndef MSVCRT_TIME_T_DEFINED
typedef long MSVCRT(time_t);
#define MSVCRT_TIME_T_DEFINED
#endif

#ifndef USE_MSVCRT_PREFIX
#ifndef MSVCRT_BSD_TYPES_DEFINED
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int  u_int;
typedef unsigned long u_long;
#define MSVCRT_BSD_TYPES_DEFINED
#endif

#define dev_t _dev_t
#define ino_t _ino_t
#define mode_t _mode_t
#define off_t _off_t
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_TYPES_H */
