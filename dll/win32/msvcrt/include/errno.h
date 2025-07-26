/*
 * Copyright 2001 Jon Griffiths
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

#ifndef __WINE_ERRNO_H
#define __WINE_ERRNO_H

#include <corecrt.h>

#  define EPERM   1
#  define ENOENT  2
#  define ESRCH   3
#  define EINTR   4
#  define EIO     5
#  define ENXIO   6
#  define E2BIG   7
#  define ENOEXEC 8
#  define EBADF   9
#  define ECHILD  10
#  define EAGAIN  11
#  define ENOMEM  12
#  define EACCES  13
#  define EFAULT  14
#  define EBUSY   16
#  define EEXIST  17
#  define EXDEV   18
#  define ENODEV  19
#  define ENOTDIR 20
#  define EISDIR  21
#  define EINVAL  22
#  define ENFILE  23
#  define EMFILE  24
#  define ENOTTY  25
#  define EFBIG   27
#  define ENOSPC  28
#  define ESPIPE  29
#  define EROFS   30
#  define EMLINK  31
#  define EPIPE   32
#  define EDOM    33
#  define ERANGE  34
#  define EDEADLK 36
#  define EDEADLOCK EDEADLK
#  define ENAMETOOLONG 38
#  define ENOLCK  39
#  define ENOSYS  40
#  define ENOTEMPTY 41
#  define EILSEQ    42

#  define STRUNCATE 80

#ifndef _CRT_NO_POSIX_ERROR_CODES
#  define EADDRINUSE 100
#  define EADDRNOTAVAIL 101
#  define EAFNOSUPPORT 102
#  define EALREADY 103
#  define EBADMSG 104
#  define ECANCELED 105
#  define ECONNABORTED 106
#  define ECONNREFUSED 107
#  define ECONNRESET 108
#  define EDESTADDRREQ 109
#  define EHOSTUNREACH 110
#  define EIDRM 111
#  define EINPROGRESS 112
#  define EISCONN 113
#  define ELOOP 114
#  define EMSGSIZE 115
#  define ENETDOWN 116
#  define ENETRESET 117
#  define ENETUNREACH 118
#  define ENOBUFS 119
#  define ENODATA 120
#  define ENOLINK 121
#  define ENOMSG 122
#  define ENOPROTOOPT 123
#  define ENOSR 124
#  define ENOSTR 125
#  define ENOTCONN 126
#  define ENOTRECOVERABLE 127
#  define ENOTSOCK 128
#  define ENOTSUP 129
#  define EOPNOTSUPP 130
#  define EOTHER 131
#  define EOVERFLOW 132
#  define EOWNERDEAD 133
#  define EPROTO 134
#  define EPROTONOSUPPORT 135
#  define EPROTOTYPE 136
#  define ETIME 137
#  define ETIMEDOUT 138
#  define ETXTBSY 139
#  define EWOULDBLOCK 140
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int* __cdecl _errno(void);

#ifdef __cplusplus
}
#endif

#define errno        (*_errno())

#endif  /* __WINE_ERRNO_H */
