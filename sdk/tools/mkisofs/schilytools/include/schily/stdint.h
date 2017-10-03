/* @(#)stdint.h	1.37 15/12/10 Copyright 1997-2015 J. Schilling */
/*
 *	Abstraction from stdint.h
 *
 *	Copyright (c) 1997-2015 J. Schilling
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

#ifndef	_SCHILY_STDINT_H
#define	_SCHILY_STDINT_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * Let us include system defined types too.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

/*
 * Include sys/param.h for NBBY - needed in case that CHAR_BIT is missing
 */
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>	/* Must be before limits.h */
#endif

/*
 * Include limits.h for CHAR_BIT - needed by TYPE_MINVAL(t) and  TYPE_MAXVAL(t)
 */
#ifndef	_SCHILY_LIMITS_H
#include <schily/limits.h>
#endif

#ifndef	CHAR_BIT
#ifdef	NBBY
#define	CHAR_BIT	NBBY
#endif
#endif

/*
 * Last resort: define CHAR_BIT by hand
 */
#ifndef	CHAR_BIT
#define	CHAR_BIT	8
#endif

/*
 * These macros may not work on all platforms but as we depend
 * on two's complement in many places, they do not reduce portability.
 * The macros below work with 2s complement and ones complement machines.
 * Verify with this table...
 *
 *	Bits	1's c.	2's complement.
 * 	100	-3	-4
 * 	101	-2	-3
 * 	110	-1	-2
 * 	111	-0	-1
 * 	000	+0	 0
 * 	001	+1	+1
 * 	010	+2	+2
 * 	011	+3	+3
 *
 * Computing -TYPE_MINVAL(type) will not work on 2's complement machines
 * if 'type' is int or more. Use:
 *		((unsigned type)(-1 * (TYPE_MINVAL(type)+1))) + 1;
 * it works for both 1's complement and 2's complement machines.
 */
#define	TYPE_ISSIGNED(t)	(((t)-1) < ((t)0))
#define	TYPE_ISUNSIGNED(t)	(!TYPE_ISSIGNED(t))
#define	TYPE_MSBVAL(t)		((t)(~((t)0) << (sizeof (t)*CHAR_BIT - 1)))
#define	TYPE_MINVAL(t)		(TYPE_ISSIGNED(t)			\
				    ? TYPE_MSBVAL(t)			\
				    : ((t)0))
#define	TYPE_MAXVAL(t)		((t)(~((t)0) - TYPE_MINVAL(t)))

/*
 * MSVC has size_t in stddef.h
 */
#ifdef HAVE_STDDEF_H
#ifndef	_INCL_STDDEF_H
#include <stddef.h>
#define	_INCL_STDDEF_H
#endif
#endif

/*
 * CHAR_IS_UNSIGNED is needed to define int8_t
 */
#ifdef	__CHAR_UNSIGNED__	/* GNU GCC define (dynamic)	*/
#ifndef CHAR_IS_UNSIGNED
#define	CHAR_IS_UNSIGNED	/* Sing Schily define (static)	*/
#endif
#endif

/*
 * This is a definition for a compiler dependant 64 bit type.
 * There is currently a silently fallback to a long if the compiler does not
 * support it. Check if this is the right way.
 *
 * Be very careful here as MSVC does not implement long long but rather __int64
 * and once someone makes 'long long' 128 bits on a 64 bit machine, we need to
 * check for a MSVC __int128 type.
 */
#ifndef	NO_LONGLONG
#	if	!defined(USE_LONGLONG) && defined(HAVE_LONGLONG)
#		define	USE_LONGLONG
#	endif
#	if	!defined(USE_LONGLONG) && defined(HAVE___INT64)
#		define	USE_LONGLONG
#	endif
#endif

#ifdef	USE_LONGLONG

#	if	defined(HAVE___INT64)

typedef	__int64			Llong;
typedef	unsigned __int64	Ullong;	/* We should avoid this */
typedef	unsigned __int64	ULlong;

#define	SIZEOF_LLONG		SIZEOF___INT64
#define	SIZEOF_ULLONG		SIZEOF_UNSIGNED___INT64

#	else	/* We must have HAVE_LONG_LONG */

typedef	long long		Llong;
typedef	unsigned long long	Ullong;	/* We should avoid this */
typedef	unsigned long long	ULlong;

#define	SIZEOF_LLONG		SIZEOF_LONG_LONG
#define	SIZEOF_ULLONG		SIZEOF_UNSIGNED_LONG_LONG

#	endif	/* HAVE___INT64 / HAVE_LONG_LONG */

#else	/* !USE_LONGLONG */

typedef	long			Llong;
typedef	unsigned long		Ullong;	/* We should avoid this */
typedef	unsigned long		ULlong;

#define	SIZEOF_LLONG		SIZEOF_LONG
#define	SIZEOF_ULLONG		SIZEOF_UNSIGNED_LONG

#endif	/* USE_LONGLONG */

#ifndef	LLONG_MIN
#define	LLONG_MIN	TYPE_MINVAL(Llong)
#endif
#ifndef	LLONG_MAX
#define	LLONG_MAX	TYPE_MAXVAL(Llong)
#endif
#ifndef	ULLONG_MAX
#define	ULLONG_MAX	TYPE_MAXVAL(Ullong)
#endif

/*
 * Start inttypes.h emulation.
 *
 * Thanks to Solaris 2.4 and even recent 1999 Linux versions, we
 * cannot use the official UNIX-98 names here. Old Solaris versions
 * define parts of the types in some exotic include files.
 * Linux even defines incompatible types in <sys/types.h>.
 */

#if defined(HAVE_INTTYPES_H) || defined(HAVE_STDINT_H)
#if defined(HAVE_INTTYPES_H)
#	ifndef	_INCL_INTTYPES_H
#	include <inttypes.h>
#	define	_INCL_INTTYPES_H
#	endif
#else
#if defined(HAVE_STDINT_H)
#	ifndef	_INCL_STDINT_H
#	include <stdint.h>
#	define	_INCL_STDINT_H
#	endif
#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * On VMS on VAX, these types are present but non-scalar.
 * Thus we may not be able to use them
 */
#ifdef	HAVE_LONGLONG
#	define	HAVE_INT64_T
#	define	HAVE_UINT64_T
#endif

#define	Int8_t			int8_t
#define	Int16_t			int16_t
#define	Int32_t			int32_t
#ifdef	HAVE_LONGLONG
#define	Int64_t			int64_t
#endif
#define	Intmax_t		intmax_t
#define	UInt8_t			uint8_t
#define	UInt16_t		uint16_t
#define	UInt32_t		uint32_t
#ifdef	HAVE_LONGLONG
#define	UInt64_t		uint64_t
#endif
#define	UIntmax_t		uintmax_t

#define	Intptr_t		intptr_t
#define	UIntptr_t		uintptr_t

/*
 * If we only have a UNIX-98 inttypes.h but no SUSv3
 *
 * Beware not to use int64_t / uint64_t as VMS on a VAX defines
 * them as non-scalar (structure) based types.
 */
#ifndef	HAVE_TYPE_INTMAX_T
#define	intmax_t	Llong
#endif
#ifndef	HAVE_TYPE_UINTMAX_T
#define	uintmax_t	ULlong
#endif

#ifdef	__cplusplus
}
#endif

#else	/* !HAVE_INTTYPES_H */

#ifdef	__cplusplus
extern "C" {
#endif

#if SIZEOF_CHAR != 1 || SIZEOF_UNSIGNED_CHAR != 1
/*
 * #error will not work for all compilers (e.g. sunos4)
 * The following line will abort compilation on all compilers
 * if the above is true. And that's what we want.
 */
error  Sizeof char is not equal 1

#include <schily/err_char.h>	/* Avoid "unknown directive" with K&R */
#endif

#if	defined(__STDC__) || defined(CHAR_IS_UNSIGNED)
	typedef	signed char		Int8_t;
#else
	typedef	char			Int8_t;
#endif

#if SIZEOF_SHORT_INT == 2
	typedef	short			Int16_t;
#else
	error		No int16_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif

#if SIZEOF_INT == 4
#if defined(_MSC_VER) && SIZEOF_LONG_INT == 4
	typedef	long			Int32_t;
#else
	typedef	int			Int32_t;
#endif
#else
	error		No int32_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif

#if SIZEOF_LONG_INT == 8
	typedef		long		Int64_t;
#	define	HAVE_INT64_T
#else
#if SIZEOF_LONG_LONG == 8
	typedef		long long	Int64_t;
#	define	HAVE_INT64_T
#else
#if SIZEOF___INT64 == 8
	typedef		__int64		Int64_t;
#	define	HAVE_INT64_T
#else
/*
 * Tolerate platforms without 64-Bit support.
 */
/*	error		No int64_t found */
#endif
#endif
#endif

#if SIZEOF_CHAR_P == SIZEOF_INT
	typedef		int		Intptr_t;
#else
#if SIZEOF_CHAR_P == SIZEOF_LONG_INT
	typedef		long		Intptr_t;
#else
#if SIZEOF_CHAR_P == SIZEOF_LLONG
	typedef		Llong		Intptr_t;
#else
	error		No intptr_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif
#endif
#endif

typedef	unsigned char		UInt8_t;

#if SIZEOF_UNSIGNED_SHORT_INT == 2
	typedef	unsigned short		UInt16_t;
#else
	error		No uint16_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif

#if SIZEOF_UNSIGNED_INT == 4
#if defined(_MSC_VER) && SIZEOF_UNSIGNED_LONG_INT == 4
	typedef	unsigned long		UInt32_t;
#else
	typedef	unsigned int		UInt32_t;
#endif
#else
	error		No int32_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif

#if SIZEOF_UNSIGNED_LONG_INT == 8
	typedef	unsigned long		UInt64_t;
#	define	HAVE_UINT64_T
#else
#if SIZEOF_UNSIGNED_LONG_LONG == 8
	typedef	unsigned long long	UInt64_t;
#	define	HAVE_UINT64_T
#else
#if SIZEOF_UNSIGNED___INT64 == 8
	typedef	unsigned __int64	UInt64_t;
#	define	HAVE_UINT64_T
#else
/*
 * Tolerate platforms without 64-Bit support.
 */
/*	error		No uint64_t found */
#endif
#endif
#endif

#define	Intmax_t	Llong
#define	UIntmax_t	Ullong

#if SIZEOF_CHAR_P == SIZEOF_UNSIGNED_INT
	typedef		unsigned int	UIntptr_t;
#else
#if SIZEOF_CHAR_P == SIZEOF_UNSIGNED_LONG_INT
	typedef		unsigned long	UIntptr_t;
#else
#if SIZEOF_CHAR_P == SIZEOF_ULLONG
	typedef		ULlong		UIntptr_t;
#else
	error		No uintptr_t found

#include <schily/err_type.h>	/* Avoid "unknown directive" with K&R */
#endif
#endif
#endif

#ifdef	_MSC_VER
/*
 * All recent platforms define the POSIX/C-99 compliant types from inttypes.h
 * except Microsoft. With these #defines, we may also use official types on a
 * Microsoft environment.
 *
 * Warning: Linux-2.2 and before do not have inttypes.h and define some of the
 * types in an incmpatible way.
 */
#undef	int8_t
#define	int8_t			Int8_t
#undef	int16_t
#define	int16_t			Int16_t
#undef	int32_t
#define	int32_t			Int32_t
#undef	int64_t
#define	int64_t			Int64_t
#undef	intmax_t
#define	intmax_t		Intmax_t
#undef	uint8_t
#define	uint8_t			UInt8_t
#undef	uint16_t
#define	uint16_t		UInt16_t
#undef	uint32_t
#define	uint32_t		UInt32_t
#undef	uint64_t
#define	uint64_t		UInt64_t
#undef	uintmax_t
#define	uintmax_t		UIntmax_t

#undef	intptr_t
#define	intptr_t		Intptr_t
#undef	uintptr_t
#define	uintptr_t		UIntptr_t
#endif	/* _MSC_VER */

#ifdef	__cplusplus
}
#endif

#endif	/* HAVE_INTTYPES_H */

#ifndef	INT8_MIN
#define	INT8_MIN	TYPE_MINVAL(Int8_t)
#endif
#ifndef	INT8_MAX
#define	INT8_MAX	TYPE_MAXVAL(Int8_t)
#endif
#ifndef	UINT8_MAX
#define	UINT8_MAX	TYPE_MAXVAL(UInt8_t)
#endif

#ifndef	INT16_MIN
#define	INT16_MIN	TYPE_MINVAL(Int16_t)
#endif
#ifndef	INT16_MAX
#define	INT16_MAX	TYPE_MAXVAL(Int16_t)
#endif
#ifndef	UINT16_MAX
#define	UINT16_MAX	TYPE_MAXVAL(UInt16_t)
#endif

#ifndef	INT32_MIN
#define	INT32_MIN	TYPE_MINVAL(Int32_t)
#endif
#ifndef	INT32_MAX
#define	INT32_MAX	TYPE_MAXVAL(Int32_t)
#endif
#ifndef	UINT32_MAX
#define	UINT32_MAX	TYPE_MAXVAL(UInt32_t)
#endif

#ifdef	HAVE_INT64_T
#ifndef	INT64_MIN
#define	INT64_MIN	TYPE_MINVAL(Int64_t)
#endif
#ifndef	INT64_MAX
#define	INT64_MAX	TYPE_MAXVAL(Int64_t)
#endif
#endif
#ifdef	HAVE_UINT64_T
#ifndef	UINT64_MAX
#define	UINT64_MAX	TYPE_MAXVAL(UInt64_t)
#endif
#endif

#ifndef	INTMAX_MIN
#define	INTMAX_MIN	TYPE_MINVAL(Intmax_t)
#endif
#ifndef	INTMAX_MAX
#define	INTMAX_MAX	TYPE_MAXVAL(Intmax_t)
#endif
#ifndef	UINTMAX_MAX
#define	UINTMAX_MAX	TYPE_MAXVAL(UIntmax_t)
#endif

#define	SIZE_T_MIN	TYPE_MINVAL(size_t)
#ifdef	SIZE_T_MAX
#undef	SIZE_T_MAX			/* FreeBSD has a similar #define */
#endif
#define	SIZE_T_MAX	TYPE_MAXVAL(size_t)

#define	SSIZE_T_MIN	TYPE_MINVAL(ssize_t)
#define	SSIZE_T_MAX	TYPE_MAXVAL(ssize_t)

#endif	/* _SCHILY_STDINT_H */
