/*
 * Path and directory definitions
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
#ifndef __WINE_SYS_TIMEB_H
#define __WINE_SYS_TIMEB_H

#include <corecrt.h>

#include <pshpack8.h>

#ifndef _TIMEB_DEFINED
#define _TIMEB_DEFINED
struct _timeb
{
    time_t time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};
struct __timeb32
{
    __time32_t     time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};
struct __timeb64
{
    __time64_t     time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};
#endif /* _TIMEB_DEFINED */


#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP void __cdecl _ftime32(struct __timeb32*);
_ACRTIMP void __cdecl _ftime64(struct __timeb64*);

#ifdef __cplusplus
}
#endif

#ifdef _USE_32BIT_TIME_T
static inline void __cdecl _ftime(struct _timeb *tb) { return _ftime32((struct __timeb32*)tb); }
#else
static inline void __cdecl _ftime(struct _timeb *tb) { return _ftime64((struct __timeb64*)tb); }
#endif

#define timeb _timeb

static inline void ftime(struct _timeb* ptr) { return _ftime(ptr); }

#include <poppack.h>

#endif /* __WINE_SYS_TIMEB_H */
