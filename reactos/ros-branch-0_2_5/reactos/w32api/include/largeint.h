/*
  largeint.h

  Header for 64 bit integer arithmetics library

 */
#ifndef _LARGEINT_H
#define _LARGEINT_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _HAVE_INT64
#define _toi (__int64)
#define _toui (unsigned __int64)
#else
#error "64 bit integers not supported"
#endif

/*
  We don't let the compiler see the prototypes if we are compiling the
  library because if it does it will choke on conflicting types in the
  prototypes.
*/

#if defined(LARGEINT_PROTOS) || defined(__COMPILING_LARGEINT)

#ifndef __COMPILING_LARGEINT
/* addition/subtraction */
LARGE_INTEGER WINAPI LargeIntegerAdd (LARGE_INTEGER, LARGE_INTEGER);
LARGE_INTEGER WINAPI LargeIntegerSubtract (LARGE_INTEGER, LARGE_INTEGER);

/* bit operations */
LARGE_INTEGER WINAPI LargeIntegerArithmeticShift (LARGE_INTEGER, int);
LARGE_INTEGER WINAPI LargeIntegerShiftLeft (LARGE_INTEGER, int);
LARGE_INTEGER WINAPI LargeIntegerShiftRight (LARGE_INTEGER, int);
LARGE_INTEGER WINAPI LargeIntegerNegate (LARGE_INTEGER);

/* conversion */
LARGE_INTEGER WINAPI ConvertLongToLargeInteger (LONG);
LARGE_INTEGER WINAPI ConvertUlongToLargeInteger (ULONG);

/* multiplication */
LARGE_INTEGER WINAPI EnlargedIntegerMultiply (LONG, LONG);
LARGE_INTEGER WINAPI EnlargedUnsignedMultiply (ULONG, ULONG);
LARGE_INTEGER WINAPI ExtendedIntegerMultiply (LARGE_INTEGER, LONG);
/* FIXME: is this not part of largeint? */
LARGE_INTEGER WINAPI LargeIntegerMultiply (LARGE_INTEGER, LARGE_INTEGER);
#endif /* __COMPILING_LARGEINT */

#else

#define LargeIntegerAdd(a,b) (LARGE_INTEGER)(_toi(a) + _toi(b))
#define LargeIntegerSubtract(a,b) (LARGE_INTEGER)(_toi(a) - _toi(b))
#define LargeIntegerRightShift(i,n) (LARGE_INTEGER)(_toi(i) >> (n))
#define LargeIntegerArithmeticShift LargeIntegerRightShift
#define LargeIntegerLeftShift(i,n) (LARGE_INTEGER)(_toi(i) << (n))
#define LargeIntegerNegate(i) (LARGE_INTEGER)(- _toi(i))
#define EnlargedIntegerMultiply(a,b) (LARGE_INTEGER)(_toi(a) * _toi(b))
#define EnlargedUnsignedMultiply(a,b) (LARGE_INTEGER)(_toui(a) * _toui(b))
#define ExtendedIntegerMultiply(a,b) (LARGE_INTEGER)(_toi(a) * _toi(b))
/* FIXME: should this exist */
#define LargeIntegerMultiply(a,b) (LARGE_INTEGER)(_toi(a) * _toi(b))
#define ConvertLongToLargeInteger(l) (LARGE_INTEGER)(_toi(l))
#define ConvertUlongToLargeInteger(ul) (LARGE_INTEGER)(_toui(ul))

#endif /* LARGEINT_PROTOS || __COMPILING_LARGEINT */

#ifndef __COMPILING_LARGEINT
/* division; no macros of these because of multiple expansion */
LARGE_INTEGER WINAPI LargeIntegerDivide (LARGE_INTEGER, LARGE_INTEGER, PLARGE_INTEGER);
ULONG WINAPI EnlargedUnsignedDivide (ULARGE_INTEGER, ULONG, PULONG);
LARGE_INTEGER WINAPI ExtendedLargeIntegerDivide (LARGE_INTEGER, ULONG, PULONG);
LARGE_INTEGER WINAPI ExtendedMagicDivide (LARGE_INTEGER, LARGE_INTEGER, int);
#endif /* __COMPILING_LARGEINT */

#define LargeIntegerAnd(dest, src, m) \
{ \
  dest._STRUCT_NAME(u.)LowPart = s._STRUCT_NAME(u.)LowPart & m._STRUCT_NAME(u.)LowPart; \
  dest._STRUCT_NAME(u.)HighPart = s._STRUCT_NAME(u.)HighPart & m._STRUCT_NAME(u.)HighPart; \
}

/* comparision */
#define LargeIntegerGreaterThan(a,b) (_toi(a) > _toi(b))
#define LargeIntegerGreaterThanOrEqual(a,b) (_toi(a) >= _toi(b))
#define LargeIntegerEqualTo(a,b) (_toi(a) == _toi(b))
#define LargeIntegerNotEqualTo(a,b) (_toi(a) != _toi(b))
#define LargeIntegerLessThan(a,b) (_toi(a) < _toi(b))
#define LargeIntegerLessThanOrEqualTo(a,b) (_toi(a) <= _toi(b))
#define LargeIntegerGreaterThanZero(a) (_toi(a) > 0)
#define LargeIntegerGreaterOrEqualToZero(a) ((a)._STRUCT_NAME(u.)HighPart > 0)
#define LargeIntegerEqualToZero(a) !((a)._STRUCT_NAME(u.)LowPart | (a)._STRUCT_NAME(u.)HighPart)
#define LargeIntegerNotEqualToZero(a) ((a)._STRUCT_NAME(u.)LowPart | (a)._STRUCT_NAME(u.)HighPart)
#define LargeIntegerLessThanZero(a) ((a)._STRUCT_NAME(u.)HighPart < 0)
#define LargeIntegerLessOrEqualToZero(a) (_toi(a) <= 0)

#ifndef __COMPILING_LARGEINT
#undef _toi
#undef _toui
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LARGEINT_H */
