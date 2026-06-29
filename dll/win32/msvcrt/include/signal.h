/*
 * Signal definitions
 *
 * Copyright 2005 Juan Lang
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
#ifndef _WINE_SIGNAL_H
#define _WINE_SIGNAL_H

#include <corecrt.h>

#define SIGINT   2
#define SIGILL   4
#define SIGFPE   8
#define SIGSEGV  11
#define SIGTERM  15
#define SIGBREAK 21
#define SIGABRT  22

#define NSIG     (SIGABRT + 1)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (__cdecl *__sighandler_t)(int);

#define SIG_DFL ((__sighandler_t)0)
#define SIG_IGN ((__sighandler_t)1)
#define SIG_ERR ((__sighandler_t)-1)

_ACRTIMP void** __cdecl __pxcptinfoptrs(void);
_ACRTIMP __sighandler_t __cdecl signal(int sig, __sighandler_t func);
_ACRTIMP int __cdecl raise(int sig);

#ifdef __cplusplus
}
#endif

#endif /* _WINE_SIGNAL_H */
