/* Copyright (C) 1989, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* As a special exception, if you include this header file into source
   files compiled by GCC, this header file does not by itself cause
   the resulting executable to be covered by the GNU General Public
   License.  This exception does not however invalidate any other
   reasons why the executable file might be covered by the GNU General
   Public License.  */

/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */

#ifndef _STDARG_H
#ifndef _ANSI_STDARG_H_
#ifndef __need___va_list
#define _STDARG_H
#define _ANSI_STDARG_H_
#endif /* not __need___va_list */
#undef __need___va_list

/* Define __msc_va_list.  */

#ifndef __MSC_VA_LIST
#define __MSC_VA_LIST
#ifdef _M_CEE_PURE
typedef System::ArgIterator __msc_va_list;
#else
typedef char * __msc_va_list;
#endif

#ifdef __cplusplus
#define __msc_addressof(v) (&reinterpret_cast<const char &>(v))
#else
#define __msc_addressof(v) (&(v))
#endif

#if defined(_M_IA64) && !defined(_M_CEE_PURE)
#define __msc_va_align 8
#define __msc_slotsizeof(t) ((sizeof(t) + __msc_va_align - 1) & ~(__msc_va_align - 1))

#define __msc_va_struct_align 16
#define __msc_alignof(ap) ((((ap)+ __msc_va_struct_align - 1) & ~(__msc_va_struct_align - 1)) - (ap))
#define __msc_apalign(t,ap) (__alignof(t) > 8 ? __msc_alignof((unsigned __int64)ap) : 0)
#else
#define __msc_slotsizeof(t) (sizeof(t))
#define __msc_apalign(t,ap) (__alignof(t))
#endif

#if defined(_M_CEE)
extern void __cdecl __va_start(__msc_va_list*, ...);
extern void * __cdecl __va_arg(__msc_va_list*, ...);
extern void __cdecl __va_end(__msc_va_list*);
#define __msc_va_start(ap,v) (__va_start(&ap, __msc_addressof(v), __msc_slotsizeof(v), __alignof(v), __msc_addressof(v)))
#define __msc_va_end(ap)     (__va_end(&ap))
#define __msc_va_arg(ap,t)   (*(t *)__va_arg(&ap, __msc_slotsizeof(t), __msc_apalign(t,ap), (t *)0))
#elif defined(_M_IA64)
#error TODO IA64 support
#elif defined(_M_AMD64)
#error TODO IA64 support
#else
#define __msc_intsizeof(n)   ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define __msc_va_start(ap,v) ((void)(ap = (__msc_va_list)__msc_addressof(v) + __msc_intsizeof(v)))
#define __msc_va_end(ap)     ((void)(ap = (__msc_va_list)0))
#define __msc_va_arg(ap,t)   (*(t *)((ap += __msc_intsizeof(t)) - __msc_intsizeof(t)))
#endif

#define __msc_va_copy(d,s) ((void)((d) = (s)))
#endif

/* Define the standard macros for the user,
   if this invocation was from the user program.  */
#ifdef _STDARG_H

#define va_start(v,l)	__msc_va_start(v,l)
#define va_end(v)	__msc_va_end(v)
#define va_arg(v,l)	__msc_va_arg(v,l)
#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L
#define va_copy(d,s)	__msc_va_copy(d,s)
#endif
#define __va_copy(d,s)	__msc_va_copy(d,s)

/* Define va_list, if desired, from __msc_va_list. */

#if !defined(_VA_LIST_) && !defined(_VA_LIST) && !defined(_VA_LIST_DEFINED) && !defined(_VA_LIST_T_H) && !defined(__va_list__)
#define _VA_LIST_
#define _VA_LIST
#define _VA_LIST_DEFINED
#define _VA_LIST_T_H
#define __va_list__
typedef __msc_va_list va_list;
#endif

#endif /* _STDARG_H */

#endif /* not _ANSI_STDARG_H_ */
#endif /* not _STDARG_H */
