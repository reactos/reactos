/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#pragma once
#include <vcruntime.h>

#ifndef _INC_LIMITS
#define _INC_LIMITS

/*
* File system limits
*
* TODO: NAME_MAX and OPEN_MAX are file system limits or not? Are they the
*       same as FILENAME_MAX and FOPEN_MAX from stdio.h?
* NOTE: Apparently the actual size of PATH_MAX is 260, but a space is
*       required for the NUL. TODO: Test?
*/
#define PATH_MAX	(259)

#define CHAR_BIT 8
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 0xff

#ifdef _CHAR_UNSIGNED
 #define CHAR_MIN 0
 #define CHAR_MAX UCHAR_MAX
#else
 #define CHAR_MIN SCHAR_MIN
 #define CHAR_MAX SCHAR_MAX
#endif /* _CHAR_UNSIGNED */

#define MB_LEN_MAX 5
#define SHRT_MIN (-32768)
#define SHRT_MAX 32767
#define USHRT_MAX 0xffff
#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define UINT_MAX 0xffffffff
#define LONG_MIN (-2147483647L - 1)
#define LONG_MAX 2147483647L
#define ULONG_MAX 0xffffffffUL
#define LLONG_MAX 9223372036854775807LL
#define LLONG_MIN (-9223372036854775807LL - 1)
#define ULLONG_MAX 0xffffffffffffffffULL

#define _I8_MIN ((signed char)(-127 - 1))
#define _I8_MAX ((signed char)127)
#define _UI8_MAX ((unsigned char)0xff)

#define _I16_MIN ((short)(-32767 - 1))
#define _I16_MAX ((short)32767)
#define _UI16_MAX ((unsigned short)0xffffU)

#define _I32_MIN (-2147483647 - 1)
#define _I32_MAX 2147483647
#define _UI32_MAX 0xffffffffu

#define _I64_MIN (-9223372036854775807LL - 1)
#define _I64_MAX 9223372036854775807LL
#define _UI64_MAX 0xffffffffffffffffULL

#if defined(_MSC_VER) && (_INTEGRAL_MAX_BITS >= 128)
#define _I128_MIN (-170141183460469231731687303715884105727i128 - 1)
#define _I128_MAX 170141183460469231731687303715884105727i128
#define _UI128_MAX 0xffffffffffffffffffffffffffffffffui128
#endif

#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX _UI64_MAX
#else
#define SIZE_MAX UINT_MAX
#endif
#endif /* SIZE_MAX */

#if __STDC_WANT_SECURE_LIB__
#ifndef RSIZE_MAX
#define RSIZE_MAX SIZE_MAX
#endif /* RSIZE_MAX */
#endif /* __STDC_WANT_SECURE_LIB__ */

#endif /* _INC_LIMITS */
