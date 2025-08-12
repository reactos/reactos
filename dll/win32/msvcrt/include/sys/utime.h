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
#ifndef __WINE_SYS_UTIME_H
#define __WINE_SYS_UTIME_H

#include <corecrt.h>

#include <pshpack8.h>

#ifndef _UTIMBUF_DEFINED
#define _UTIMBUF_DEFINED
struct _utimbuf
{
    time_t actime;
    time_t modtime;
};
struct __utimbuf32
{
    __time32_t actime;
    __time32_t modtime;
};
struct __utimbuf64
{
    __time64_t actime;
    __time64_t modtime;
};
#endif /* _UTIMBUF_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int __cdecl _futime32(int,struct __utimbuf32*);
_ACRTIMP int __cdecl _futime64(int,struct __utimbuf64*);
_ACRTIMP int __cdecl _utime32(const char*,struct __utimbuf32*);
_ACRTIMP int __cdecl _utime64(const char*,struct __utimbuf64*);
_ACRTIMP int __cdecl _wutime32(const wchar_t*,struct __utimbuf32*);
_ACRTIMP int __cdecl _wutime64(const wchar_t*,struct __utimbuf64*);

#ifdef _USE_32BIT_TIME_T
static inline int _futime(int fd, struct _utimbuf *buf) { return _futime32(fd, (struct __utimbuf32*)buf); }
static inline int _utime(const char *s, struct _utimbuf *buf) { return _utime32(s, (struct __utimbuf32*)buf); }
static inline int _wutime(const wchar_t *s, struct _utimbuf *buf) { return _wutime32(s, (struct __utimbuf32*)buf); }
#else
static inline int _futime(int fd, struct _utimbuf *buf) { return _futime64(fd, (struct __utimbuf64*)buf); }
static inline int _utime(const char *s, struct _utimbuf *buf) { return _utime64(s, (struct __utimbuf64*)buf); }
static inline int _wutime(const wchar_t *s, struct _utimbuf *buf) { return _wutime64(s, (struct __utimbuf64*)buf); }
#endif

#ifdef __cplusplus
}
#endif


#define utimbuf _utimbuf

static inline int utime(const char* path, struct _utimbuf* buf) { return _utime(path, buf); }

#include <poppack.h>

#endif /* __WINE_SYS_UTIME_H */
