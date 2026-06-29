/*
 * Variable argument definitions
 *
 * Copyright 2022 Jacek Caban
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

#ifndef _INC_VADEFS
#define _INC_VADEFS

#include <corecrt.h>

#ifdef __cplusplus
#define _ADDRESSOF(v) (&reinterpret_cast<const char &>(v))
#else
#define _ADDRESSOF(v) (&(v))
#endif

#if defined (__GNUC__) && defined(__x86_64__)

typedef __builtin_ms_va_list va_list;
#define _crt_va_start(v,l)  __builtin_ms_va_start(v,l)
#define _crt_va_arg(v,l)    __builtin_va_arg(v,l)
#define _crt_va_end(v)      __builtin_ms_va_end(v)
#define _crt_va_copy(d,s)   __builtin_ms_va_copy(d,s)

#elif defined(__GNUC__) || defined(__clang__)

typedef __builtin_va_list va_list;
#define _crt_va_start(v,l)  __builtin_va_start(v,l)
#define _crt_va_arg(v,l)    __builtin_va_arg(v,l)
#define _crt_va_end(v)      __builtin_va_end(v)
#define _crt_va_copy(d,s)   __builtin_va_copy(d,s)

#else

typedef char *va_list;

#if defined(__i386__)
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define _crt_va_start(v,l)  ((v) = (va_list)_ADDRESSOF(l) + _INTSIZEOF(l))
#define _crt_va_arg(v,l)    (*(l *)(((v) += _INTSIZEOF(l)) - _INTSIZEOF(l)))
#define _crt_va_end(v)      ((v) = (va_list)0)
#define _crt_va_copy(d,s)   ((d) = (s))
#endif

#endif

#endif /* _INC_VADEFS */
