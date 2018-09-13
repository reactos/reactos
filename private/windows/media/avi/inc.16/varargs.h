/***
*varargs.h - XENIX style macros for variable argument functions
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines XENIX style macros for accessing arguments of a
*   function which takes a variable number of arguments.
*   [System V]
*
****/

#ifdef __cplusplus
extern "C" {
#endif 

#ifdef _WINDLL
#define _FARARG_ __far
#else 
#define _FARARG_
#endif 

#if (_MSC_VER <= 600)
#define __far       _far
#endif 

#ifdef __STDC__
#error varargs.h incompatible with ANSI (use stdarg.h)
#endif 

#ifndef _VA_LIST_DEFINED
typedef char _FARARG_ *va_list;
#define _VA_LIST_DEFINED
#endif 

/*
 * define a macro to compute the size of a type, variable or expression,
 * rounded up to the nearest multiple of sizeof(int). This number is its
 * size as function argument (Intel architecture). Note that the macro
 * depends on sizeof(int) being a power of 2!
 */

#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_dcl va_list va_alist;
#define va_start(ap) ap = (va_list)&va_alist
#define va_arg(ap,t) ( *(t _FARARG_ *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ap = (va_list)0

#ifdef __cplusplus
}
#endif 
