/* 
 * limits.h
 *
 * Defines constants for the sizes of integral types.
 *
 * NOTE: GCC should supply a version of this header and it should be safe to
 *       use that version instead of this one (maybe safer).
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1 $
 * $Author: ariadne $
 * $Date: 1999/03/22 20:48:08 $
 *
 */

#ifndef _LIMITS_H_
#define _LIMITS_H_

/*
 * File system limits
 *
 * TODO: NAME_MAX and OPEN_MAX are file system limits or not? Are they the
 *       same as FILENAME_MAX and FOPEN_MAX from stdio.h?
 * NOTE: Apparently the actual size of PATH_MAX is 260, but a space is
 *       required for the NUL. TODO: Test?
 */
#define PATH_MAX	(259)

/*
 * Characteristics of the char data type.
 *
 * TODO: Is MB_LEN_MAX correct?
 */
#define CHAR_BIT	8
#define MB_LEN_MAX	2

#define SCHAR_MIN	(-128)
#define SCHAR_MAX	127

#define UCHAR_MAX	255

/* TODO: Is this safe? I think it might just be testing the preprocessor,
 *       not the compiler itself... */
#if	('\x80' < 0)
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX
#else
#define CHAR_MIN	0
#define CHAR_MAX	UCHAR_MAX
#endif

/*
 * Maximum and minimum values for ints.
 */
#define INT_MAX		2147483647
#define INT_MIN		(-INT_MAX-1)

#define UINT_MAX	(2U * INT_MAX + 1)

/*
 * Maximum and minimum values for shorts.
 */
#define SHRT_MAX	32767
#define SHRT_MIN	(-SHRT_MAX-1)

#define USHRT_MAX	((unsigned short) (2U * SHRT_MAX + 1))

/*
 * Maximum and minimum values for longs and unsigned longs.
 *
 * TODO: This is not correct for Alphas, which have 64 bit longs.
 */
#define LONG_MAX	2147483647L

#define LONG_MIN	(-LONG_MAX-1)

#define ULONG_MAX	(2UL * LONG_MAX + 1)


/*
 * The GNU C compiler also allows 'long long int'
 */
#if	!defined(__STRICT_ANSI__) && defined(__GNUC__)

#define LONG_LONG_MAX	9223372036854775807LL
#define LONG_LONG_MIN	(-LONG_LONG_MAX-1)

#define ULONG_LONG_MAX	(2ULL * LONG_LONG_MAX + 1)

#endif /* Not Strict ANSI and GNU C compiler */


#endif /* not _LIMITS_H_ */
