/***
*float.h - constants for floating point values
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file contains defines for a number of implementation dependent
*   values which are commonly used by sophisticated numerical (floating
*   point) programs.
*   [ANSI]
*
****/

#ifndef _INC_FLOAT

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#define DBL_DIG     15          /* # of decimal digits of precision */
#define DBL_EPSILON 2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#define DBL_MANT_DIG    53          /* # of bits in mantissa */
#define DBL_MAX     1.7976931348623158e+308 /* max value */
#define DBL_MAX_10_EXP  308         /* max decimal exponent */
#define DBL_MAX_EXP 1024            /* max binary exponent */
#define DBL_MIN     2.2250738585072014e-308 /* min positive value */
#define DBL_MIN_10_EXP  (-307)          /* min decimal exponent */
#define DBL_MIN_EXP (-1021)         /* min binary exponent */
#define _DBL_RADIX  2           /* exponent radix */
#define _DBL_ROUNDS 1           /* addition rounding: near */

#define FLT_DIG     6           /* # of decimal digits of precision */
#define FLT_EPSILON 1.192092896e-07F    /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_GUARD   0
#define FLT_MANT_DIG    24          /* # of bits in mantissa */
#define FLT_MAX     3.402823466e+38F    /* max value */
#define FLT_MAX_10_EXP  38          /* max decimal exponent */
#define FLT_MAX_EXP 128         /* max binary exponent */
#define FLT_MIN     1.175494351e-38F    /* min positive value */
#define FLT_MIN_10_EXP  (-37)           /* min decimal exponent */
#define FLT_MIN_EXP (-125)          /* min binary exponent */
#define FLT_NORMALIZE   0
#define FLT_RADIX   2           /* exponent radix */
#define FLT_ROUNDS  1           /* addition rounding: near */

#define LDBL_DIG    18          /* # of decimal digits of precision */
#define LDBL_EPSILON    1.084202172485504434e-019L /* smallest such that 1.0+LDBL_EPSILON != 1.0 */
#define LDBL_MANT_DIG   64          /* # of bits in mantissa */
#define LDBL_MAX    1.189731495357231765e+4932L /* max value */
#define LDBL_MAX_10_EXP 4932            /* max decimal exponent */
#define LDBL_MAX_EXP    16384           /* max binary exponent */
#define LDBL_MIN    3.3621031431120935063e-4932L /* min positive value */
#define LDBL_MIN_10_EXP (-4931)         /* min decimal exponent */
#define LDBL_MIN_EXP    (-16381)        /* min binary exponent */
#define _LDBL_RADIX 2           /* exponent radix */
#define _LDBL_ROUNDS    1           /* addition rounding: near */


/*
 *  8087/80287 math control information
 */


/* User Control Word Mask and bit definitions.
 * These definitions match the 8087/80287
 */

#define _MCW_EM     0x003f      /* interrupt Exception Masks */
#define _EM_INVALID 0x0001      /*   invalid */
#define _EM_DENORMAL    0x0002      /*   denormal */
#define _EM_ZERODIVIDE  0x0004      /*   zero divide */
#define _EM_OVERFLOW    0x0008      /*   overflow */
#define _EM_UNDERFLOW   0x0010      /*   underflow */
#define _EM_INEXACT 0x0020      /*   inexact (precision) */

#define _MCW_IC     0x1000      /* Infinity Control */
#define _IC_AFFINE  0x1000      /*   affine */
#define _IC_PROJECTIVE  0x0000      /*   projective */

#define _MCW_RC     0x0c00      /* Rounding Control */
#define _RC_CHOP    0x0c00      /*   chop */
#define _RC_UP      0x0800      /*   up */
#define _RC_DOWN    0x0400      /*   down */
#define _RC_NEAR    0x0000      /*   near */

#define _MCW_PC     0x0300      /* Precision Control */
#define _PC_24      0x0000      /*    24 bits */
#define _PC_53      0x0200      /*    53 bits */
#define _PC_64      0x0300      /*    64 bits */


/* initial Control Word value */

#define _CW_DEFAULT ( _IC_AFFINE + _RC_NEAR + _PC_64 + _EM_DENORMAL + _EM_UNDERFLOW + _EM_INEXACT )


/* user Status Word bit definitions */

#define _SW_INVALID 0x0001  /* invalid */
#define _SW_DENORMAL    0x0002  /* denormal */
#define _SW_ZERODIVIDE  0x0004  /* zero divide */
#define _SW_OVERFLOW    0x0008  /* overflow */
#define _SW_UNDERFLOW   0x0010  /* underflow */
#define _SW_INEXACT 0x0020  /* inexact (precision) */


/* invalid subconditions (_SW_INVALID also set) */

#define _SW_UNEMULATED      0x0040  /* unemulated instruction */
#define _SW_SQRTNEG     0x0080  /* square root of a neg number */
#define _SW_STACKOVERFLOW   0x0200  /* FP stack overflow */
#define _SW_STACKUNDERFLOW  0x0400  /* FP stack underflow */


/*  Floating point error signals and return codes */

#define _FPE_INVALID        0x81
#define _FPE_DENORMAL       0x82
#define _FPE_ZERODIVIDE     0x83
#define _FPE_OVERFLOW       0x84
#define _FPE_UNDERFLOW      0x85
#define _FPE_INEXACT        0x86

#define _FPE_UNEMULATED     0x87
#define _FPE_SQRTNEG        0x88
#define _FPE_STACKOVERFLOW  0x8a
#define _FPE_STACKUNDERFLOW 0x8b

#define _FPE_EXPLICITGEN    0x8c    /* raise( SIGFPE ); */


/* function prototypes */

unsigned int __cdecl _clear87(void);
unsigned int __cdecl _control87(unsigned int, unsigned int);
void __cdecl _fpreset(void);
unsigned int __cdecl _status87(void);


#ifndef __STDC__
/* Non-ANSI names for compatibility */

#define DBL_RADIX       _DBL_RADIX
#define DBL_ROUNDS      _DBL_ROUNDS

#define LDBL_RADIX      _LDBL_RADIX
#define LDBL_ROUNDS     _LDBL_ROUNDS

#define MCW_EM          _MCW_EM
#define EM_INVALID      _EM_INVALID
#define EM_DENORMAL     _EM_DENORMAL
#define EM_ZERODIVIDE       _EM_ZERODIVIDE
#define EM_OVERFLOW     _EM_OVERFLOW
#define EM_UNDERFLOW        _EM_UNDERFLOW
#define EM_INEXACT      _EM_INEXACT

#define MCW_IC          _MCW_IC
#define IC_AFFINE       _IC_AFFINE
#define IC_PROJECTIVE       _IC_PROJECTIVE

#define MCW_RC          _MCW_RC
#define RC_CHOP         _RC_CHOP
#define RC_UP           _RC_UP
#define RC_DOWN         _RC_DOWN
#define RC_NEAR         _RC_NEAR

#define MCW_PC          _MCW_PC
#define PC_24           _PC_24
#define PC_53           _PC_53
#define PC_64           _PC_64

#define CW_DEFAULT      _CW_DEFAULT

#define SW_INVALID      _SW_INVALID
#define SW_DENORMAL     _SW_DENORMAL
#define SW_ZERODIVIDE       _SW_ZERODIVIDE
#define SW_OVERFLOW     _SW_OVERFLOW
#define SW_UNDERFLOW        _SW_UNDERFLOW
#define SW_INEXACT      _SW_INEXACT

#define SW_UNEMULATED       _SW_UNEMULATED
#define SW_SQRTNEG      _SW_SQRTNEG
#define SW_STACKOVERFLOW    _SW_STACKOVERFLOW
#define SW_STACKUNDERFLOW   _SW_STACKUNDERFLOW

#define FPE_INVALID     _FPE_INVALID
#define FPE_DENORMAL        _FPE_DENORMAL
#define FPE_ZERODIVIDE      _FPE_ZERODIVIDE
#define FPE_OVERFLOW        _FPE_OVERFLOW
#define FPE_UNDERFLOW       _FPE_UNDERFLOW
#define FPE_INEXACT     _FPE_INEXACT

#define FPE_UNEMULATED      _FPE_UNEMULATED
#define FPE_SQRTNEG     _FPE_SQRTNEG
#define FPE_STACKOVERFLOW   _FPE_STACKOVERFLOW
#define FPE_STACKUNDERFLOW  _FPE_STACKUNDERFLOW

#define FPE_EXPLICITGEN     _FPE_EXPLICITGEN

#endif 


#ifdef __cplusplus
}
#endif 

#define _INC_FLOAT
#endif 
