//-----------------------------------------------------------------------------
//  ldouble.h
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//      define stuff needed for long double in win32
//
//  Revision History:
//
//  []      06-Apr-1993 Dans    Created
//
//-----------------------------------------------------------------------------
#if !defined(_ldouble_h)
#define _ldouble_h

#include <cvinfo.h> // for FLOAT10
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif

#pragma pack(push, 2)
// little endian, 10byte float, same as FLOAT10, but easier to parse!
typedef struct F10_REP {
    unsigned long   ulManLo;
    unsigned long   ulManHi;
    unsigned short  usExp : 15;
    unsigned short  bSign : 1;
} F10_REP;
#pragma pack(pop)

// special bits
#define bNan        0x40000000
#define bInd        0xc0000000
#define bExpSp      0x7fff
#define bSign       0x8000
#define bMsbShort   0x8000
#define bMsbLong    0x80000000

// get a string rep of long doubles
// always scientific notation:
// [+-]x.yyyyyyyyyyyyyyyyyye[+-]zzzz, LDBL_DIG digits of precision
//
char *  SzFromLd ( char * pchBuf, size_t cb, FLOAT10 f10 );

// get a float10 rep of a string
//  (basically _strtold from 16-bit runtimes)
//
FLOAT10 LdFromSz ( char * szFloat, char ** ppchEnd );

// compare a 10-byte float to zero
//
#define PF10(x) ((F10_REP*)&x)
__inline int
FCmpLdZero (
    FLOAT10 f10
    )
{
    return !(PF10(f10)->ulManLo || PF10(f10)->ulManHi || PF10(f10)->usExp);
}


// give the real values for 10-byte reals in place of the CRT in float.h
//  values were retrieved from 16-bit version of float.h.
//
#undef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP     4932

#undef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP     -4931

#undef LDBL_DIG
#define LDBL_DIG            18

#ifdef __cplusplus
} // extern "C" {
#endif

#endif
