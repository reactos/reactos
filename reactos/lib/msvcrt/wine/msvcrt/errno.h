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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_ERRNO_H
#define __WINE_ERRNO_H
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

#ifdef USE_MSVCRT_PREFIX

#  define MSVCRT_EPERM   1
#  define MSVCRT_ENOENT  2
#  define MSVCRT_ESRCH   3
#  define MSVCRT_EINTR   4
#  define MSVCRT_EIO     5
#  define MSVCRT_ENXIO   6
#  define MSVCRT_E2BIG   7
#  define MSVCRT_ENOEXEC 8
#  define MSVCRT_EBADF   9
#  define MSVCRT_ECHILD  10
#  define MSVCRT_EAGAIN  11
#  define MSVCRT_ENOMEM  12
#  define MSVCRT_EACCES  13
#  define MSVCRT_EFAULT  14
#  define MSVCRT_EBUSY   16
#  define MSVCRT_EEXIST  17
#  define MSVCRT_EXDEV   18
#  define MSVCRT_ENODEV  19
#  define MSVCRT_ENOTDIR 20
#  define MSVCRT_EISDIR  21
#  define MSVCRT_EINVAL  22
#  define MSVCRT_ENFILE  23
#  define MSVCRT_EMFILE  24
#  define MSVCRT_ENOTTY  25
#  define MSVCRT_EFBIG   27
#  define MSVCRT_ENOSPC  28
#  define MSVCRT_ESPIPE  29
#  define MSVCRT_EROFS   30
#  define MSVCRT_EMLINK  31
#  define MSVCRT_EPIPE   32
#  define MSVCRT_EDOM    33
#  define MSVCRT_ERANGE  34
#  define MSVCRT_EDEADLK 36
#  define MSVCRT_EDEADLOCK MSVCRT_EDEADLK
#  define MSVCRT_ENAMETOOLONG 38
#  define MSVCRT_ENOLCK  39
#  define MSVCRT_ENOSYS  40
#  define MSVCRT_ENOTEMPTY 41

#else /* USE_MSVCRT_PREFIX */

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

#endif /* USE_MSVCRT_PREFIX */

#ifdef __cplusplus
extern "C" {
#endif

extern int* MSVCRT(_errno)(void);

#ifdef __cplusplus
}
#endif

#ifndef USE_MSVCRT_PREFIX
# define errno        (*_errno())
#else
# define MSVCRT_errno (*MSVCRT__errno())
#endif

#endif  /* __WINE_ERRNO_H */
