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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_SYS_TIMEB_H
#define __WINE_SYS_TIMEB_H
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

#ifndef MSVCRT_TIME_T_DEFINED
typedef long MSVCRT(time_t);
#define MSVCRT_TIME_T_DEFINED
#endif

#ifndef MSVCRT_TIMEB_DEFINED
#define MSVCRT_TIMEB_DEFINED
struct _timeb
{
    MSVCRT(time_t) time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};
#endif /* MSVCRT_TIMEB_DEFINED */


#ifdef __cplusplus
extern "C" {
#endif

void        _ftime(struct _timeb*);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define timeb _timeb

static inline void ftime(struct _timeb* ptr) { return _ftime(ptr); }
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_TIMEB_H */
