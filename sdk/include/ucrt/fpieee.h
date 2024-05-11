//
// fpieee.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file contains constant and type definitions for handling floating point
// exceptions (IEEE 754).
//
#pragma once
#ifndef _INC_FPIEEE // include guard for 3rd party interop
#define _INC_FPIEEE

#ifndef __midl

#ifdef _M_CEE_PURE
    #error ERROR: This file is not supported in the pure mode!
#endif

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#ifndef __assembler

    // Define floating point IEEE compare result values.
    typedef enum
    {
        _FpCompareEqual,
        _FpCompareGreater,
        _FpCompareLess,
        _FpCompareUnordered
    } _FPIEEE_COMPARE_RESULT;

    // Define floating point format and result precision values.
    typedef enum
    {
        _FpFormatFp32,
        _FpFormatFp64,
        _FpFormatFp80,
        _FpFormatFp128,
        _FpFormatI16,
        _FpFormatI32,
        _FpFormatI64,
        _FpFormatU16,
        _FpFormatU32,
        _FpFormatU64,
        _FpFormatBcd80,
        _FpFormatCompare,
        _FpFormatString,
    } _FPIEEE_FORMAT;

    // Define operation code values.
    typedef enum
    {
        _FpCodeUnspecified,
        _FpCodeAdd,
        _FpCodeSubtract,
        _FpCodeMultiply,
        _FpCodeDivide,
        _FpCodeSquareRoot,
        _FpCodeRemainder,
        _FpCodeCompare,
        _FpCodeConvert,
        _FpCodeRound,
        _FpCodeTruncate,
        _FpCodeFloor,
        _FpCodeCeil,
        _FpCodeAcos,
        _FpCodeAsin,
        _FpCodeAtan,
        _FpCodeAtan2,
        _FpCodeCabs,
        _FpCodeCos,
        _FpCodeCosh,
        _FpCodeExp,
        _FpCodeFabs,
        _FpCodeFmod,
        _FpCodeFrexp,
        _FpCodeHypot,
        _FpCodeLdexp,
        _FpCodeLog,
        _FpCodeLog10,
        _FpCodeModf,
        _FpCodePow,
        _FpCodeSin,
        _FpCodeSinh,
        _FpCodeTan,
        _FpCodeTanh,
        _FpCodeY0,
        _FpCodeY1,
        _FpCodeYn,
        _FpCodeLogb,
        _FpCodeNextafter,
        _FpCodeNegate,
        _FpCodeFmin,         // XMMI
        _FpCodeFmax,         // XMMI
        _FpCodeConvertTrunc, // XMMI
        _XMMIAddps,          // XMMI
        _XMMIAddss,
        _XMMISubps,
        _XMMISubss,
        _XMMIMulps,
        _XMMIMulss,
        _XMMIDivps,
        _XMMIDivss,
        _XMMISqrtps,
        _XMMISqrtss,
        _XMMIMaxps,
        _XMMIMaxss,
        _XMMIMinps,
        _XMMIMinss,
        _XMMICmpps,
        _XMMICmpss,
        _XMMIComiss,
        _XMMIUComiss,
        _XMMICvtpi2ps,
        _XMMICvtsi2ss,
        _XMMICvtps2pi,
        _XMMICvtss2si,
        _XMMICvttps2pi,
        _XMMICvttss2si,
        _XMMIAddsubps,       // XMMI for PNI
        _XMMIHaddps,         // XMMI for PNI
        _XMMIHsubps,         // XMMI for PNI
        _XMMIRoundps,        // 66 0F 3A 08
        _XMMIRoundss,        // 66 0F 3A 0A
        _XMMIDpps,           // 66 0F 3A 40
        _XMMI2Addpd,         // XMMI2
        _XMMI2Addsd,
        _XMMI2Subpd,
        _XMMI2Subsd,
        _XMMI2Mulpd,
        _XMMI2Mulsd,
        _XMMI2Divpd,
        _XMMI2Divsd,
        _XMMI2Sqrtpd,
        _XMMI2Sqrtsd,
        _XMMI2Maxpd,
        _XMMI2Maxsd,
        _XMMI2Minpd,
        _XMMI2Minsd,
        _XMMI2Cmppd,
        _XMMI2Cmpsd,
        _XMMI2Comisd,
        _XMMI2UComisd,
        _XMMI2Cvtpd2pi,   // 66 2D
        _XMMI2Cvtsd2si,   // F2
        _XMMI2Cvttpd2pi,  // 66 2C
        _XMMI2Cvttsd2si,  // F2
        _XMMI2Cvtps2pd,   // 0F 5A
        _XMMI2Cvtss2sd,   // F3
        _XMMI2Cvtpd2ps,   // 66
        _XMMI2Cvtsd2ss,   // F2
        _XMMI2Cvtdq2ps,   // 0F 5B
        _XMMI2Cvttps2dq,  // F3
        _XMMI2Cvtps2dq,   // 66
        _XMMI2Cvttpd2dq,  // 66 0F E6
        _XMMI2Cvtpd2dq,   // F2
        _XMMI2Addsubpd,   // 66 0F D0
        _XMMI2Haddpd,     // 66 0F 7C
        _XMMI2Hsubpd,     // 66 0F 7D
        _XMMI2Roundpd,    // 66 0F 3A 09
        _XMMI2Roundsd,    // 66 0F 3A 0B
        _XMMI2Dppd,       // 66 0F 3A 41
    } _FP_OPERATION_CODE;

#endif // __assembler

#ifdef _CORECRT_BUILD
    #ifndef __assembler

        #define OP_UNSPEC    _FpCodeUnspecified
        #define OP_ADD       _FpCodeAdd
        #define OP_SUB       _FpCodeSubtract
        #define OP_MUL       _FpCodeMultiply
        #define OP_DIV       _FpCodeDivide
        #define OP_REM       _FpCodeRemainder
        #define OP_COMP      _FpCodeCompare
        #define OP_CVT       _FpCodeConvert
        #define OP_RND       _FpCodeRound
        #define OP_TRUNC     _FpCodeTruncate

        #define OP_EXP       _FpCodeExp

        #define OP_POW       _FpCodePow
        #define OP_LOG       _FpCodeLog
        #define OP_LOG10     _FpCodeLog10
        #define OP_SINH      _FpCodeSinh
        #define OP_COSH      _FpCodeCosh
        #define OP_TANH      _FpCodeTanh
        #define OP_ASIN      _FpCodeAsin
        #define OP_ACOS      _FpCodeAcos
        #define OP_ATAN      _FpCodeAtan
        #define OP_ATAN2     _FpCodeAtan2
        #define OP_SQRT      _FpCodeSquareRoot
        #define OP_SIN       _FpCodeSin
        #define OP_COS       _FpCodeCos
        #define OP_TAN       _FpCodeTan
        #define OP_CEIL      _FpCodeCeil
        #define OP_FLOOR     _FpCodeFloor
        #define OP_ABS       _FpCodeFabs
        #define OP_MODF      _FpCodeModf
        #define OP_LDEXP     _FpCodeLdexp
        #define OP_CABS      _FpCodeCabs
        #define OP_HYPOT     _FpCodeHypot
        #define OP_FMOD      _FpCodeFmod
        #define OP_FREXP     _FpCodeFrexp
        #define OP_Y0        _FpCodeY0
        #define OP_Y1        _FpCodeY1
        #define OP_YN        _FpCodeYn

        #define OP_LOGB       _FpCodeLogb
        #define OP_NEXTAFTER  _FpCodeNextafter

        // XMMI
        #define OP_ADDPS     _XMMIAddps
        #define OP_ADDSS     _XMMIAddss
        #define OP_SUBPS     _XMMISubps
        #define OP_SUBSS     _XMMISubss
        #define OP_MULPS     _XMMIMulps
        #define OP_MULSS     _XMMIMulss
        #define OP_DIVPS     _XMMIDivps
        #define OP_DIVSS     _XMMIDivss
        #define OP_SQRTPS    _XMMISqrtps
        #define OP_SQRTSS    _XMMISqrtss
        #define OP_MAXPS     _XMMIMaxps
        #define OP_MAXSS     _XMMIMaxss
        #define OP_MINPS     _XMMIMinps
        #define OP_MINSS     _XMMIMinss
        #define OP_CMPPS     _XMMICmpps
        #define OP_CMPSS     _XMMICmpss
        #define OP_COMISS    _XMMIComiss
        #define OP_UCOMISS   _XMMIUComiss
        #define OP_CVTPI2PS  _XMMICvtpi2ps
        #define OP_CVTSI2SS  _XMMICvtsi2ss
        #define OP_CVTPS2PI  _XMMICvtps2pi
        #define OP_CVTSS2SI  _XMMICvtss2si
        #define OP_CVTTPS2PI _XMMICvttps2pi
        #define OP_CVTTSS2SI _XMMICvttss2si
        #define OP_ADDSUBPS  _XMMIAddsubps
        #define OP_HADDPS    _XMMIHaddps
        #define OP_HSUBPS    _XMMIHsubps
        #define OP_ROUNDPS   _XMMIRoundps
        #define OP_ROUNDSS   _XMMIRoundss
        #define OP_DPPS      _XMMIDpps
        // XMMI

        // XMMI2
        #define OP_ADDPD     _XMMI2Addpd      // XMMI2
        #define OP_ADDSD     _XMMI2Addsd
        #define OP_SUBPD     _XMMI2Subpd
        #define OP_SUBSD     _XMMI2Subsd
        #define OP_MULPD     _XMMI2Mulpd
        #define OP_MULSD     _XMMI2Mulsd
        #define OP_DIVPD     _XMMI2Divpd
        #define OP_DIVSD     _XMMI2Divsd
        #define OP_SQRTPD    _XMMI2Sqrtpd
        #define OP_SQRTSD    _XMMI2Sqrtsd
        #define OP_MAXPD     _XMMI2Maxpd
        #define OP_MAXSD     _XMMI2Maxsd
        #define OP_MINPD     _XMMI2Minpd
        #define OP_MINSD     _XMMI2Minsd
        #define OP_CMPPD     _XMMI2Cmppd
        #define OP_CMPSD     _XMMI2Cmpsd
        #define OP_COMISD    _XMMI2Comisd
        #define OP_UCOMISD   _XMMI2UComisd
        #define OP_CVTPD2PI  _XMMI2Cvtpd2pi   // 66 2D
        #define OP_CVTSD2SI  _XMMI2Cvtsd2si   // F2
        #define OP_CVTTPD2PI _XMMI2Cvttpd2pi  // 66 2C
        #define OP_CVTTSD2SI _XMMI2Cvttsd2si  // F2
        #define OP_CVTPS2PD  _XMMI2Cvtps2pd   // 0F 5A
        #define OP_CVTSS2SD  _XMMI2Cvtss2sd   // F3
        #define OP_CVTPD2PS  _XMMI2Cvtpd2ps   // 66
        #define OP_CVTSD2SS  _XMMI2Cvtsd2ss   // F2
        #define OP_CVTDQ2PS  _XMMI2Cvtdq2ps   // 0F 5B
        #define OP_CVTTPS2DQ _XMMI2Cvttps2dq  // F3
        #define OP_CVTPS2DQ  _XMMI2Cvtps2dq   // 66
        #define OP_CVTTPD2DQ _XMMI2Cvttpd2dq  // 66 0F E6
        #define OP_CVTPD2DQ  _XMMI2Cvtpd2dq   // F2
        #define OP_ADDSUBPD  _XMMI2Addsubpd   // 66 0F D0
        #define OP_HADDPD    _XMMI2Haddpd     // 66 0F 7C
        #define OP_HSUBPD    _XMMI2Hsubpd     // 66 0F 7D
        #define OP_ROUNDPD   _XMMI2Roundpd    // 66 0F 3A 09
        #define OP_ROUNDSD   _XMMI2Roundsd    // 66 0F 3A 0B
        #define OP_DPPD      _XMMI2Dppd       // 66 0F 3A 41
        // XMMI2

    #else // __assembler

        // This must be the same as the enumerator _FP_OPERATION_CODE
        #define OP_UNSPEC    0
        #define OP_ADD       1
        #define OP_SUB       2
        #define OP_MUL       3
        #define OP_DIV       4
        #define OP_SQRT      5
        #define OP_REM       6
        #define OP_COMP      7
        #define OP_CVT       8
        #define OP_RND       9
        #define OP_TRUNC     10
        #define OP_FLOOR     11
        #define OP_CEIL      12
        #define OP_ACOS      13
        #define OP_ASIN      14
        #define OP_ATAN      15
        #define OP_ATAN2     16
        #define OP_CABS      17
        #define OP_COS       18
        #define OP_COSH      19
        #define OP_EXP       20
        #define OP_ABS       21         // same as OP_FABS
        #define OP_FABS      21         // same as OP_ABS
        #define OP_FMOD      22
        #define OP_FREXP     23
        #define OP_HYPOT     24
        #define OP_LDEXP     25
        #define OP_LOG       26
        #define OP_LOG10     27
        #define OP_MODF      28
        #define OP_POW       29
        #define OP_SIN       30
        #define OP_SINH      31
        #define OP_TAN       32
        #define OP_TANH      33
        #define OP_Y0        34
        #define OP_Y1        35
        #define OP_YN        36
        #define OP_LOGB      37
        #define OP_NEXTAFTER 38
        #define OP_NEG       39

    #endif // __assembler
#endif // _CORECRT_BUILD

// Define rounding modes.
#ifndef __assembler

    typedef enum
    {
        _FpRoundNearest,
        _FpRoundMinusInfinity,
        _FpRoundPlusInfinity,
        _FpRoundChopped
    } _FPIEEE_ROUNDING_MODE;

    typedef enum
    {
        _FpPrecisionFull,
        _FpPrecision53,
        _FpPrecision24,
    } _FPIEEE_PRECISION;


    // Define floating point context record
    typedef float          _FP32;
    typedef double         _FP64;
    typedef short          _I16;
    typedef int            _I32;
    typedef unsigned short _U16;
    typedef unsigned int   _U32;
    typedef __int64        _Q64;

    #ifdef _CORECRT_BUILD
    typedef struct
    {
        unsigned long W[4];
    } _U32ARRAY;
    #endif

    typedef struct
    {
        unsigned short W[5];
    } _FP80;

    typedef struct _CRT_ALIGN(16)
    {
        unsigned long W[4];
    } _FP128;

    typedef struct _CRT_ALIGN(8)
    {
        unsigned long W[2];
    } _I64;

    typedef struct _CRT_ALIGN(8)
    {
        unsigned long W[2];
    } _U64;

    typedef struct
    {
        unsigned short W[5];
    } _BCD80;

    typedef struct _CRT_ALIGN(16)
    {
        _Q64 W[2];
    } _FPQ64;

    typedef struct
    {
        union
        {
            _FP32        Fp32Value;
            _FP64        Fp64Value;
            _FP80        Fp80Value;
            _FP128       Fp128Value;
            _I16         I16Value;
            _I32         I32Value;
            _I64         I64Value;
            _U16         U16Value;
            _U32         U32Value;
            _U64         U64Value;
            _BCD80       Bcd80Value;
            char         *StringValue;
            int          CompareValue;
            #ifdef _CORECRT_BUILD
            _U32ARRAY    U32ArrayValue;
            #endif
            _Q64         Q64Value;
            _FPQ64       Fpq64Value;
        } Value;

        unsigned int OperandValid : 1;
        unsigned int Format       : 4;

    } _FPIEEE_VALUE;

    typedef struct
    {
        unsigned int Inexact          : 1;
        unsigned int Underflow        : 1;
        unsigned int Overflow         : 1;
        unsigned int ZeroDivide       : 1;
        unsigned int InvalidOperation : 1;
    } _FPIEEE_EXCEPTION_FLAGS;


    typedef struct
    {
        unsigned int RoundingMode :  2;
        unsigned int Precision    :  3;
        unsigned int Operation    : 12;
        _FPIEEE_EXCEPTION_FLAGS Cause;
        _FPIEEE_EXCEPTION_FLAGS Enable;
        _FPIEEE_EXCEPTION_FLAGS Status;
        _FPIEEE_VALUE Operand1;
        _FPIEEE_VALUE Operand2;
        _FPIEEE_VALUE Result;
    } _FPIEEE_RECORD, *_PFPIEEE_RECORD;


    struct _EXCEPTION_POINTERS;

    typedef int (__cdecl* _FpieeFltHandlerType)(_FPIEEE_RECORD*);

    // Floating point IEEE exception filter routine
    _ACRTIMP int __cdecl _fpieee_flt(
        _In_ unsigned long               _ExceptionCode,
        _In_ struct _EXCEPTION_POINTERS* _PtExceptionPtr,
        _In_ _FpieeFltHandlerType        _Handler
        );

#endif // __assembler

_CRT_END_C_HEADER

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // __midl
#endif // _INC_FPIEEE
