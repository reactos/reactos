/* @(#)varargs.h	1.8 14/01/06 Copyright 1998-2014 J. Schilling */
/*
 *	Generic header for users of var args ...
 *
 *	Includes a default definition for va_copy()
 *	and some magic know how about the SVr4 Power PC var args ABI
 *	to create a __va_arg_list() macro.
 *
 *	The __va_arg_list() macro is needed to fetch a va_list type argument
 *	from a va_list. This is needed to implement a recursive "%r" printf.
 *
 *	Copyright (c) 1998-2014 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_VARARGS_H
#define	_SCHILY_VARARGS_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	PROTOTYPES
/*
 * For ANSI C-compilers prefer stdarg.h
 */
#	ifdef	HAVE_STDARG_H
#		ifndef	_INCL_STDARG_H
#		include <stdarg.h>
#		define	_INCL_STDARG_H
#		endif
#	else
#		ifndef	_INCL_VARARGS_H
#		include <varargs.h>
#		define	_INCL_VARARGS_H
#		endif
#	endif
#else
/*
 * For K&R C-compilers prefer varargs.h
 */
#	ifdef	HAVE_VARARGS_H
#		ifndef	_INCL_VARARGS_H
#		include <varargs.h>
#		define	_INCL_VARARGS_H
#		endif
#	else
#		ifndef	_INCL_STDARG_H
#		include <stdarg.h>
#		define	_INCL_STDARG_H
#		endif
#	endif
#endif

#if (defined(__linux__) || defined(__linux) || defined(sun)) && \
		(defined(__ppc) || defined(__PPC) || \
		defined(powerpc) || defined(__powerpc__))

#	ifndef	VA_LIST_IS_ARRAY
#	define	VA_LIST_IS_ARRAY
#	endif
#endif


/*
 * __va_copy() is used by GCC 2.8 or newer until va_copy() becomes
 * a final ISO standard.
 */
#if !defined(va_copy) && !defined(HAVE_VA_COPY)
#	if	defined(__va_copy)
#		define	va_copy(to, from)	__va_copy(to, from)
#	endif
#endif

/*
 * va_copy() is a Solaris extension to provide a portable way to perform a
 * variable argument list "bookmarking" function.
 * If it is not available via stdarg.h, use a simple assignement for backward
 * compatibility.
 */
#if !defined(va_copy) && !defined(HAVE_VA_COPY)
#ifdef	VA_LIST_IS_ARRAY
#	define	va_copy(to, from)	((to)[0] = (from)[0])
#else
#	define	va_copy(to, from)	((to) = (from))
#endif
#endif

/*
 * I don't know any portable way to get an arbitrary
 * C object from a var arg list so I use a
 * system-specific routine __va_arg_list() that knows
 * if 'va_list' is an array. You will not be able to
 * assign the value of __va_arg_list() but it works
 * to be used as an argument of a function.
 * It is a requirement for recursive printf to be able
 * to use this function argument. If your system
 * defines va_list to be an array you need to know this
 * via autoconf or another mechanism.
 * It would be nice to have something like
 * __va_arg_list() in stdarg.h
 */

#ifdef	VA_LIST_IS_ARRAY
#	define	__va_arg_list(list)	va_arg(list, void *)
#else
#	define	__va_arg_list(list)	va_arg(list, va_list)
#endif

/*
 * This structure allows to work around the C limitation that a variable of
 * type array cannot appear at the left side of an assignement operator.
 * By putting va_list inside a struture, the assignement will work even in case
 * that va_list is an array2.
 */
typedef struct {
	va_list	ap;
} va_lists_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_VARARGS_H */
