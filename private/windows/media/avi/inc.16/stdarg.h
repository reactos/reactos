/***
*stdarg.h - defines ANSI-style macros for variable argument functions
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines ANSI-style macros for accessing arguments
*   of functions which take a variable number of arguments.
*   [ANSI]
*
****/

#ifndef _INC_STDARG

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

#define va_start(ap,v) ap = (va_list)&v + _INTSIZEOF(v)
#define va_arg(ap,t) ( *(t _FARARG_ *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ap = (va_list)0

#ifdef __cplusplus
}
#endif 

#define _INC_STDARG
#endif 
